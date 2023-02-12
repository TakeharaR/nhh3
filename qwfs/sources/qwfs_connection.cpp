#include <string>
#include <random>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cassert>

#include "qwfs_connection.h"

namespace qwfs
{
    Connection::Connection(QwfsId id, const char* host, const char* port, const QwfsCallbacks& callbacks, DebugOutputCallback debugOutput) :
        _id(id)
        , _status(QwfsStatus::Wait)
        , _phase(Phase::Uninitialize)
        , _hostName(host)
        , _port(port)
        , _sock(INVALID_SOCKET)
        , _peerAddInfo(nullptr)
        , _localAddInfo(nullptr)
        , _callbacks(callbacks)
        , _debugOutput(debugOutput)
        , _quicheConfig(nullptr)
        , _quicheH3Config(nullptr)
        , _quicheConnection(nullptr)
        , _quicheH3Connection(nullptr)
        , _updateFunc(nullptr)
        , _callRetry(false)
        , _callAbort(false)
        , _totalWriteSize(0U)
    {
#ifdef _WINDOWS
        WSADATA    wsaData;
        WSAStartup(MAKEWORD(2, 0), &wsaData);
#endif
        if (nullptr != _debugOutput)
        {
            std::stringstream ss;
            ss << "[qwfs info][" << _hostName << "]Create qwfs connection" << std::endl;
            DebugOutput(ss.str().c_str());
        }

        SetPhase(Phase::CreateSocket);
    }

    Connection::~Connection()
    {
        // quiche_conn_close ���Ă�ł���
        if (nullptr != _quicheH3Connection)
        {
            quiche_conn_close(_quicheConnection, false, 0, nullptr, 0);
        }

        _h3StreamsErrorStock.clear();
        _h3StreamsStock.clear();
        _h3Streams.clear();
        _h3StreamsMap.clear();

        if (INVALID_SOCKET != _sock)
        {
            closesocket(_sock);
        }

        if (nullptr != _localAddInfo)
        {
            _localAddInfo = nullptr;
        }

        if (nullptr != _peerAddInfo)
        {
            freeaddrinfo(_peerAddInfo);
            _peerAddInfo = nullptr;
        }

        ClearQuicheConnection(this);

        // todo : �I�������҂�

        ClearConfig();

#ifdef _WINDOWS
        WSACleanup();
#endif

#if 0
        // Unity Editor ���� Uninitizalie ���Ă΂��ۂɊ��� DebugOutput �̎Q�Ɛ悪�����Ă���ꍇ������̂Ŋ�{�R�����g�A�E�g
        // �f�o�b�O�ɂ��������ȂǂɗL�������邱��
        if (nullptr != _debugOutput)
        {
            std::stringstream ss;
            ss << "[qwfs info][" << _hostName << "]Destroy qwfs connection" << std::endl;
            DebugOutput(ss.str().c_str());
        }
#endif
    }

    QwfsResult Connection::SetOptions(const QwfsOptions& options)
    {
        // �R�l�N�V����������̕ύX�͐ڑ�������ɂ��ꍇ�ɐؒf���������܂Ȃ���΂Ȃ炸�ʓ|�Ȃ̂łƂ肠�����������O�����Ɍ���
        if (Phase::Initialize != _phase)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        _options = options;
        
        // �b��B�I�v�V�����̐ݒ�̎d���͍čl�̗]�n������
        if (nullptr != options._caCertsList)
        {
            _caCertsList = options._caCertsList;
        }
        if (nullptr != options._qlogPath)
        {
            _qlogPath = options._qlogPath;
        }

        return QwfsResult::Ok;
    }

    QwfsResult Connection::PublishRequest(QwfsId externalId, const char* path, const char* saveFilePath, QwfsRequestHeaders* headers, uint64_t headersSize)
    {
        // �G���[�����ς߂邪�X�e�[�^�X�͕ς��Ȃ�(�ʐM�����̓��g���C���Ȃ��ƊJ�n���Ȃ�)
        if (QwfsStatus::Wait <= _status)
        {
            if (QwfsStatus::Wait == _status)
            {
                // Wait �̎��͍ăX�^�[�g�Ƃ��ăv���O���X������������
                _totalWriteSize = 0U;
            }
            SetStatus(QwfsStatus::Connecting);
        }

        auto stream = std::make_unique<Stream>(externalId, saveFilePath, _hostName.c_str(), path, headers, headersSize, _callbacks, _debugOutput);
        // �X�g�b�N�ɒ��߂Ă����ď������ł�����ʐM�J�n����
        _h3StreamsStock.push_back(std::move(stream));

        return QwfsResult::Ok;
    }

    QwfsResult Connection::Update()
    {
        // ����M�͕K���Ă� (�\�P�b�g�������̍ۂ͓����ŉ������Ȃ�)
        if (QwfsResult::Ok != Send())
        {
            return QwfsResult::Ok;
        }
        if (QwfsResult::Ok != Receive())
        {
            return QwfsResult::Ok;
        }

        if (nullptr == _updateFunc)
        {
            // �J�n�҂��̎��͉������������Ȃ�
            return ConvertStatusToResult(_status);
        }

        // �t�F�C�Y�ŗL�̏��������{
        auto result = _updateFunc(this);
        if (QwfsResult::Ok > result)
        {
            return result;
        }

        // �R�l�N�V�����̃N���[�Y�m�F
        CheckConnectionClosed();

        // �R�l�N�V�����̃^�C���A�E�g�m�F
        CheckConnectionTimeout();

        return QwfsResult::Ok;
    }

    void Connection::IssueCallbacks()
    {
        // �I���m�F
        auto beginItr = _h3Streams.begin();
        auto endItr = _h3Streams.end();
        auto newEndItr = remove_if(beginItr, endItr, [&](std::unique_ptr<Stream>& stream) -> bool
        {
            // ���ۊm�F
            auto status = stream->GetStatus();

            if (QwfsStatus::Completed == status)
            {
                stream->CallSuccessCallback(_id);
                return true;
            }
            else if (QwfsStatus::Wait > status)
            {
                stream->CallErrorCallback(_id);
                if (QwfsStatus::CriticalError >= status)
                {
                    // �i�s�s�\�ȃG���[�̍ۂ̓V���b�g�_�E����
                    SetPhase(Phase::StartShutdown);
                    SetStatus(status, stream->GetErrorDetail());
                    // ���g���C�������̂Ń��X�g����̍폜�͂��Ȃ�
                }
                else
                {
                    // ���������ŉ��Ƃ��Ȃ�l�b�g���[�N�G���[�n�̏ꍇ�͎������g���C
                    stream->Retry();
                }
            }

            return false;
        });
        _h3Streams.erase(newEndItr, endItr);
    }

    void Connection::ClearConfig()
    {
        if (nullptr != _quicheH3Config)
        {
            quiche_h3_config_free(_quicheH3Config);
            _quicheH3Config = nullptr;
        }

        if (nullptr != _quicheConfig)
        {
            quiche_config_free(_quicheConfig);
            _quicheConfig = nullptr;
        }
    }

    void Connection::ClearQuicheConnection(Connection* connection) const
    {
        if (nullptr != connection->_quicheH3Connection)
        {
            quiche_h3_conn_free(connection->_quicheH3Connection);
            connection->_quicheH3Connection = nullptr;
        }

        if (nullptr != connection->_quicheConnection)
        {
            quiche_conn_free(connection->_quicheConnection);
            connection->_quicheConnection = nullptr;
        }
    }

    QwfsResult Connection::SetOptionsInternal()
    {
        ClearConfig();
        _quicheConfig = CreateQuicheConfig(_options);
        if (nullptr == _quicheConfig)
        {
            return SetStatus(QwfsStatus::CriticalErrorQuiche, "Failed to create quiche config.");
        }

        _quicheH3Config = CreateQuicheHttp3Config(_options);
        if (nullptr == _quicheH3Config)
        {
            return SetStatus(QwfsStatus::CriticalErrorQuiche, "Failed to create quiche h3 config.");
        }

        return QwfsResult::Ok;
    }

    // QUIC �ŗL�̐ݒ���s��
    quiche_config* Connection::CreateQuicheConfig(const QwfsOptions& options) const
    {
        // �����ɂ� QUIC �̃o�[�W������n��
        // �o�[�W�����l�S�V�G�[�V������������������ 0xbabababa ��n������
        quiche_config* config = quiche_config_new(QUICHE_PROTOCOL_VERSION);
        if (config == nullptr)
        {
            return nullptr;
        }

        // quiche �� HTTP/3 �� ALPN token ��ݒ肷�� -> quiche.h �ɒ�`����Ă��� QUICHE_H3_APPLICATION_PROTOCOL ��n���΂���
        quiche_config_set_application_protos(config, (uint8_t*)QUICHE_H3_APPLICATION_PROTOCOL, sizeof(QUICHE_H3_APPLICATION_PROTOCOL) - 1); // �����G���[�ȊO�Ԃ�Ȃ��̂Ŗ���

        quiche_config_verify_peer(config, options._verifyPeer);                                                                 // �ؖ����̌��؂� on/off �B�I���I���ؖ������g���ۂɂ� false �ɂ���
        if (options._quicOprtions._enableEarlyData)                                                                             // 0-RTT ���󂯓����ꍇ�͌Ăяo��
        {
            quiche_config_enable_early_data(config);
        }
        quiche_config_set_cc_algorithm(config, static_cast<quiche_cc_algorithm>(options._quicOprtions._ccType));

        quiche_config_set_max_idle_timeout(config, options._quicOprtions._maxIdleTimeout);                                      // max_idle_timeout �̐ݒ�(�~���b)�B 0 ���w�肷�鎖�Ń^�C���A�E�g���������ɂȂ�
        quiche_config_set_max_recv_udp_payload_size(config, options._quicOprtions._maxUdpPayloadSize);                          // max_udp_payload_size (��M��)�̐ݒ�B PMTU �̎������ӂ݂Đݒ���s������(�u14. Packet Size�v�Q��)
        quiche_config_set_max_send_udp_payload_size(config, options._quicOprtions._maxUdpPayloadSize);                          // max_udp_payload_size (���M��)�̐ݒ�B PMTU �̎������ӂ݂Đݒ���s������(�u14. Packet Size�v�Q��)
        quiche_config_set_initial_max_data(config, options._quicOprtions._initialMaxData);                                      // initial_max_data �̐ݒ�(�R�l�N�V�����ɑ΂������T�C�Y)
        quiche_config_set_initial_max_stream_data_bidi_local(config, options._quicOprtions._initialMaxStreamDataBidiLocal);     // initial_max_stream_data_bidi_local �̐ݒ�(���[�J���n���̑o�����X�g���[���̏����t���[����l)
        quiche_config_set_initial_max_stream_data_bidi_remote(config, options._quicOprtions._initialMaxStreamDataBidiRemote);   // initial_max_stream_data_bidi_remote �̐ݒ�(�s�A�n���̑o�����X�g���[���̏����t���[����l)
        quiche_config_set_initial_max_stream_data_uni(config, options._quicOprtions._initialMaxStreamDataUni);                  // initial_max_stream_data_uni �̐ݒ�(�P�����X�g���[���̏����t���[����l)
        quiche_config_set_initial_max_streams_bidi(config, options._quicOprtions._initialMaxStreamsBidi);                       // initial_max_streams_bidi �̐ݒ�(�쐬�\�ȑo�����X�g���[���̍ő�l)
        quiche_config_set_initial_max_streams_uni(config, options._quicOprtions._initialMaxStreamsUni);                         // initial_max_streams_uni �̐ݒ�(�쐬�\�ȒP�����X�g���[���̍ő�l)
        quiche_config_set_disable_active_migration(config, options._quicOprtions._disableActiveMigration);                      // disable_active_migration �̐ݒ�(�R�l�N�V�����}�C�O���[�V�����ɑΉ����Ă��Ȃ��ꍇ�� false �ɂ���)
        quiche_config_set_max_connection_window(config, options._quicOprtions._maxConnectionWindowSize);                        // �R�l�N�V�����ɗp����ő�E�B���h�E�T�C�Y
        quiche_config_set_max_stream_window(config, options._quicOprtions._maxStreamWindowSize);                                // �X�g���[���ɗp����ő�E�B���h�E�T�C�Y
        quiche_config_set_active_connection_id_limit(config, options._quicOprtions._activeConnectionIdLimit);                   // �A�N�e�B�u�ȃR�l�N�V���� ID �̏���l
        
        // �X�e�[�g���X���Z�b�g�p�g�[�N���̐ݒ�B���󖢑Ή�
        // quiche_config_set_stateless_reset_token

        // �ȉ��� qwfs �̗p�r�ł͊�{�I�ɂ̓T�[�o���̐ݒ�Ȃ̂ŃX���[
        // quiche_config_set_max_ack_delay
        // quiche_config_set_ack_delay_exponent
        // quiche_config_load_priv_key_from_pem_file
        // quiche_config_set_cc_algorithm
        // quiche_config_enable_hystart
        // quiche_config_enable_pacing

        // DATAGRAM �Ή� : ���󖢑Ή�
        // quiche_config_enable_dgram

        // wireshark ���ŃL���v�`���������ꍇ
        // quiche_config_log_keys

        return config;
    }

    // HTTP/3 �p�̃R���t�B�O���쐬����
    quiche_h3_config* Connection::CreateQuicheHttp3Config(const QwfsOptions& options) const
    {
        quiche_h3_config* config = quiche_h3_config_new();
        if (config == nullptr)
        {
            return nullptr;
        }

        quiche_h3_config_set_max_field_section_size(config, options._h3Oprtions._maxHeaderListSize);            // SETTINGS_MAX_HEADER_LIST_SIZE �̐ݒ�B�w�b�_���X�g�ɓo�^�ł���w�b�_�̍ő吔
        quiche_h3_config_set_qpack_max_table_capacity(config, options._h3Oprtions._qpackMaxTableCapacity);      // SETTINGS_QPACK_MAX_TABLE_CAPACITY �̐ݒ�BQPACK �̓��I�e�[�u���̍ő�l
        quiche_h3_config_set_qpack_blocked_streams(config, options._h3Oprtions._qpackBlockedStreams);           // SETTINGS_QPACK_BLOCKED_STREAMS �̐ݒ�B�u���b�N�����\���̂���X�g���[����
        quiche_h3_config_enable_extended_connect(config, options._h3Oprtions._settingsEnableConnectProtocol);   // SETTINGS_ENABLE_CONNECT_PROTOCOL �̐ݒ�B�g�� CONNECT �v���g�R���̐ݒ�BHTTP/3 �p���C�u�����Ȃ̂Ŋ�{�� false �z��

        return config;
    }

    // quiche �̃R�l�N�V�����𐶐�����֐�
    quiche_conn* Connection::CreateQuicheConnection(const char* host, quiche_config* config) const
    {
        // Initial Packet �Ŏg�p���� SCID �𗐐����琶������
        // SCID �� QUIC �o�[�W���� 1 �܂ł� 20 �o�C�g�ȓ��ɗ}����K�v������(�b��� quiche �� example �̐ݒ�l�ɏ���)
        uint8_t scid[16] = {};
        std::random_device rd;
        std::mt19937_64 mt(rd());
        for (auto& id : scid)
        {
            id = mt() % 256;
        }

        socklen_t localAddrLen = sizeof(struct sockaddr_in6);
        auto errorMessage = GetLastError();
        auto result = getsockname(_sock, (struct sockaddr*)&_localAddInfo, &localAddrLen);
        if (0 != result)
        {
            errorMessage = GetLastError();
            return nullptr;
        };

        // quiche �̃R�l�N�V�����n���h�����쐬����B���̒i�K�ł͂܂��ʐM�͎��{����Ȃ�
        return quiche_connect(host, (const uint8_t*)scid, sizeof(scid),
            (struct sockaddr*)&_localAddInfo, localAddrLen,
            _peerAddInfo->ai_addr, _peerAddInfo->ai_addrlen,
            config);
    }

    // �񓯊� UDP �\�P�b�g�̐����֐�
    // todo : �Ƃ肠�����u���b�N�ŏ������Ă��邪��Ő���
    QwfsResult Connection::CreateUdpSocket()
    {
        if (INVALID_SOCKET != _sock)
        {
            // �o�H�ɂ���Ă͂����ɓ���̂ŉ�����Ă�����
            closesocket(_sock);
            _sock = INVALID_SOCKET;
        }

        const struct addrinfo hints =
        {
            .ai_family = PF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDP
        };

        if (nullptr != _peerAddInfo)
        {
            freeaddrinfo(_peerAddInfo);
        }

        int err = getaddrinfo(_hostName.c_str(), _port.c_str(), &hints, &_peerAddInfo);
        if (err != 0)
        {
            return SetStatus(QwfsStatus::ErrorResolveHost, "Failed to resolve host.");
        }

        _sock = socket(_peerAddInfo->ai_family, SOCK_DGRAM, 0);
        if (INVALID_SOCKET == _sock)
        {
            freeaddrinfo(_peerAddInfo);
            _peerAddInfo = nullptr;
            return SetStatus(QwfsStatus::ErrorSocket, "Create socket is failed.");
        }

        u_long val = 1;
        ioctlsocket(_sock, FIONBIO, &val);

        if (connect(_sock, _peerAddInfo->ai_addr, (int)_peerAddInfo->ai_addrlen) < 0)
        {
            closesocket(_sock);
            _sock = INVALID_SOCKET;
            freeaddrinfo(_peerAddInfo);
            _peerAddInfo = nullptr;
            return SetStatus(QwfsStatus::ErrorSocket, "Failed to connect socket.");
        }

        if (nullptr != _debugOutput)
        {
            std::stringstream ss;
            ss << "[qwfs info][" << _hostName << "]Successed to create socket" << std::endl;
            DebugOutput(ss.str().c_str());
        }

        return QwfsResult::Ok;
    }

    QwfsResult Connection::SetQlogSettings()
    {
        if (_qlogPath.empty())
        {
            return QwfsResult::Ok;
        }

        std::time_t timeResult = time(nullptr);
        const tm* localTime = localtime(&timeResult);
        std::stringstream log;
        std::stringstream ss;
        ss << _qlogPath;
        ss << "/qwfs_";
        ss << _hostName << "_";
        ss << std::setw(2) << std::setfill('0') << localTime->tm_year - 100;
        ss << std::setw(2) << std::setfill('0') << localTime->tm_mon + 1;
        ss << std::setw(2) << std::setfill('0') << localTime->tm_mday;
        ss << std::setw(2) << std::setfill('0') << localTime->tm_hour;
        ss << std::setw(2) << std::setfill('0') << localTime->tm_min;
        ss << std::setw(2) << std::setfill('0') << localTime->tm_sec;
        ss << ".qlog";
        if (!quiche_conn_set_qlog_path(_quicheConnection, ss.str().c_str(), "qwfs log", ""))
        {
            log << "Failed to create qlog file -> " << ss.str().c_str() << std::endl;
            return SetStatus(QwfsStatus::ErrorFileIO, log.str().c_str());
        }

        if (nullptr != _debugOutput)
        {
            log << "[qwfs info][" << _hostName << "]Successed to create qlog file ->" << ss.str().c_str() << std::endl;
            DebugOutput(log.str().c_str());
        }

        return QwfsResult::Ok;
    }

    QwfsResult Connection::Send()
    {
        if ((INVALID_SOCKET == _sock) || (nullptr == _quicheConnection))
        {
            // �������܂��Ȃ����ŃG���[�ł͂Ȃ�
            return QwfsResult::Ok;
        }

        uint8_t sendBuffer[MAX_DATAGRAM_SIZE] = { 0 };

        // quiche �ɂ���đ��M�f�[�^��������
        // quiche_conn_send ���ĂԂ����ŁA�����̃X�e�[�^�X(�R�l�N�V������X�g���[���̏�)�ɉ����ēK�؂ȃf�[�^�𐶐����Ă����
        // ���ő���؂�Ȃ��ꍇ������̂Ń��[�v�Ŏ��؂�K�v������
        quiche_send_info sendInfo;
        while (1)
        {
            ssize_t writeSize = quiche_conn_send(_quicheConnection, sendBuffer, sizeof(sendBuffer), &sendInfo);
            if (QUICHE_ERR_DONE == writeSize)
            {
                break;
            }
            else if (QUICHE_ERR_DONE > writeSize)
            {
                std::stringstream ss;
                ss << "Failed to create packet(quiche_error is " << writeSize << ")." << std::endl;
                return SetStatus(ConvertQuicheErrorToStatus(static_cast<quiche_error>(writeSize), _status), ss.str().c_str());
            }

            // quiche �����������f�[�^�� UDP �\�P�b�g�ő��M����
            auto sendSize = sendto(_sock, (const char*)sendBuffer, (int)writeSize, 0, (struct sockaddr*)&sendInfo.to, sendInfo.to_len);
            if (sendSize != (int)writeSize)
            {
                // �ؒf �� �\�P�b�g�̍쐬�����蒼���K�v������
                // todo : �V���b�g�_�E�������܂������Ȃ���������Ȃ��ꍇ�̃P�A
                // todo : �\�P�b�g�G���[�̓��e�ɍ��킹���n���h�����O
                SetPhase(Phase::StartShutdown);
                std::stringstream ss;
                ss << "Failed to send packet(Error is " << WSAGetLastError() << ")." << std::endl;
                return SetStatus(QwfsStatus::ErrorSocket, ss.str().c_str());
            }
        }

        return QwfsResult::Ok;
    }

    QwfsResult Connection::Receive()
    {
        if (INVALID_SOCKET == _sock)
        {
            // �������܂��Ȃ����ŃG���[�ł͂Ȃ�
            return QwfsResult::Ok;
        }

        char receiveBuffer[MAX_DATAGRAM_SIZE] = { 0 };

        // quiche_conn_send ���Ŏ��؂�Ȃ��ꍇ������̂Ń��[�v�Ŏ��؂�K�v������
        // todo : ���������������ꍇ�̈�U���f
        while (1)
        {
            // UDP �p�P�b�g����M
            struct sockaddr_storage recvPeerAddr;
            socklen_t recvPeerAddrLen = sizeof(struct sockaddr_in6);
            memset(&recvPeerAddr, 0, recvPeerAddrLen);
            ssize_t recvSize = recvfrom(_sock, receiveBuffer, sizeof(receiveBuffer), 0, (struct sockaddr*)&recvPeerAddr, &recvPeerAddrLen);
            if (SOCKET_ERROR >= recvSize)
            {
                auto error = WSAGetLastError();
                if (WSAEWOULDBLOCK != error)
                {
                    // �ؒf �� �\�P�b�g�̍쐬�����蒼���K�v������
                    // todo : �V���b�g�_�E�������܂������Ȃ���������Ȃ��ꍇ�̃P�A
                    // todo : �\�P�b�g�G���[�̓��e�ɍ��킹���n���h�����O
                    SetPhase(Phase::StartShutdown);
                    std::stringstream ss;
                    ss << "Failed to recv packet(Erroris " << error << ")." << std::endl;
                    return SetStatus(QwfsStatus::ErrorSocket, ss.str().c_str());
                }
                break;
            }

            // ��M�����p�P�b�g�� quiche �ɓn��
            quiche_recv_info recv_info =
            {
                (struct sockaddr*)&recvPeerAddr,
                recvPeerAddrLen,
                (struct sockaddr*)&_localAddInfo,
                sizeof(struct sockaddr_in6),
            };
            ssize_t readSize = quiche_conn_recv(_quicheConnection, reinterpret_cast<uint8_t*>(receiveBuffer), recvSize, &recv_info);
            if (0 > readSize)
            {
                // ��Ɏ����~�X���������s��
                std::stringstream ss;
                ss << "Failed to process packet(quiche_error is " << readSize << ")." << std::endl;
                auto result = SetStatus(ConvertQuicheErrorToStatus(static_cast<quiche_error>(readSize), _status), ss.str().c_str());
                if (QwfsStatus::ErrorTls == _status)
                {
                    SetPhase(Phase::StartShutdown);
                }
                else
                {
                    // todo : �v���I�G���[�̏���. ConvertQuicheErrorToStatus ���ł������y����
                    SetPhase(Phase::Wait);
                }
                return result;
            }
        }

        return QwfsResult::Ok;
    }

    void Connection::CheckConnectionClosed()
    {
        if (
            (nullptr == _quicheConnection)  ||
            (Phase::Wait > _phase)          ||
            (_status < QwfsStatus::Wait)    ||
            (_status == QwfsStatus::Aborting)
            )
        {
            // ���������n���h�V�F�C�N���I����ĂȂ��ꍇ�A�N���[�Y�������J�n���Ă���ꍇ�͊m�F�s�v
            return;
        }

        if (quiche_conn_is_closed(_quicheConnection))
        {
            // �T�[�o����ؒf���ꂽ���ɓ���
            // �����炩��� close ���Ă�����K�v������
            SetStatus(QwfsStatus::ErrorConnectionClose, "Connection close from server");
            SetPhase(Phase::StartShutdown);
        }
    }

    void Connection::CheckConnectionTimeout()
    {
        // todo : ������Ƃ��������̗���
        if (
            (nullptr == _quicheConnection)  ||
            (_phase < Phase::Handshake)     ||
            (_status < QwfsStatus::Wait)
            )
        {
            // �N���[�Y�������J�n���Ă���ꍇ�͊m�F�s�v
            return;
        }

        auto timeout = quiche_conn_timeout_as_millis(_quicheConnection);
        if (0U == timeout)
        {
            // �^�C���A�E�g�C�x���g���Ă�
            quiche_conn_on_timeout(_quicheConnection);

            if (!(_status == QwfsStatus::Aborting) && (quiche_conn_is_closed(_quicheConnection)))
            {
                // �N���[�Y���Ă�����
                SetPhase(Phase::StartShutdown);
                SetStatus(QwfsStatus::ErrorTimeout, "Connection close");    // �\�P�b�g�G���[���Ƃ��ɂ�����̂Ń��b�Z�[�W�� timeout �Ɍ��y���Ȃ�
            }
        }
    }

    QwfsResult Connection::StartStream()
    {
        // ���ݏ������̃��N�G�X�g�ɋ󂫂�������Δ�����
        if ((_h3Streams.size() >= _options._maxConcurrentStreams) || (_h3StreamsStock.size() == 0))
        {
            return QwfsResult::Ok;
        }

        // Wait �܂��� Connecting �̎������V�K�J�n�ł��Ȃ�
        if ((QwfsStatus::Wait != _status) && (QwfsStatus::Connecting != _status))
        {
            return QwfsResult::Ok;
        }

        // �����܂ł���΃��N�G�X�g�̏��������� OK
        uint64_t num = 0;
        auto canRequestNum = _options._maxConcurrentStreams - _h3Streams.size();
        for (; num < canRequestNum; ++num)
        {
            auto stream = _h3StreamsStock.begin();      // ���� _h3StreamsStock ���� _h3StreamsStock �͍č\�z�����̂Ŗ��� begin �Ŋm�F
            if (_h3StreamsStock.end() == stream)
            {
                break;
            }
            auto streamId = (*stream)->Start(_quicheConnection, _quicheH3Connection);
            if (0 > streamId)
            {
                if (-12 == streamId)
                {
                    // �X�g���[���̍쐬�̌��E�𒴂��Ă���ꍇ�� -12 ���Ԃ��Ă���
                    // quiche ���̎��� peer �� initial_max_streams_bidi �̒l�� MAX_STREAMS �ɂ��X�V�����͂��Ȃ̂ŁA���΂炭�܂��ă��g���C����Ή�������͂�(���󂵂Ȃ��ꍇ������܂��c�c)
                    break;
                }
                SetPhase(Phase::Wait);
                return SetStatus(QwfsStatus::CriticalErrorQuiche, "Failed to create stream");
            }
            _h3StreamsMap.emplace(streamId, stream->get());  // stream id �͌ŗL�l�Ȃ̂� find �m�F�͂��Ȃ�
            _h3Streams.push_back(std::move(*stream));
            _h3StreamsStock.pop_front();
        }

        if ((0 < num) && (QwfsStatus::Wait == _status))
        {
            // wait ��Ԃł���ΐV�K�J�n�Ƃ��� Connecting �ɂ��ăv���O���X�� 0 ����n�ߒ���
            SetStatus(QwfsStatus::Connecting);
            _totalWriteSize = 0U;
        }


        return QwfsResult::Ok;
    }

    void Connection::ShutdownStream()
    {
        for (auto& stream : _h3Streams)
        {
            stream->Shutdown(_status, _errorDetail.c_str());
        }
    }

    void Connection::GetProgress(uint64_t* progress, uint64_t *totalWriteSize)
    {
        for (auto& stream : _h3Streams)
        {
            stream->AddProgress(progress, totalWriteSize);
        }
        *totalWriteSize += _totalWriteSize;
    }

    void Connection::Retry()
    {
        _callRetry = true;
    }

    void Connection::Abort()
    {
        // todo ; �N���[�Y�̗��R�������Ɠ����
        quiche_conn_close(_quicheConnection, false, 0, nullptr, 0);
        _callAbort = true;

        // ���X�g�̑S�N���A�B���̃^�C�~���O�ł��Ȃ��� IssueCallbacks �Ő����������̂������������Ă��܂�
        _h3StreamsMap.clear();
        _h3Streams.clear();
        _h3StreamsErrorStock.clear();
        _h3StreamsStock.clear();

        SetPhase(Phase::StartShutdown);
        SetStatus(QwfsStatus::Aborting);
    }

    // �p�P�b�g�̏������̎��{
    QwfsResult Connection::UpdatePhaseCreateSocket(Connection* connection) const
    {
        auto result = connection->CreateUdpSocket();
        if (QwfsResult::Ok != result)
        {
            // �\�P�b�g����̍Đ�����҂�(�G���[�J�ڂ� CreateUdpSocket ���Ŏ��{)
            return result;
        }

        connection->SetPhase(Phase::Initialize);
        return QwfsResult::Ok;
    }

    QwfsResult Connection::UpdatePhaseInitialize(Connection* connection) const
    {
        connection->SetOptionsInternal();

        ClearQuicheConnection(connection);

        connection->_quicheConnection = connection->CreateQuicheConnection(connection->_hostName.c_str(), connection->_quicheConfig);
        if (nullptr == connection->_quicheConnection)
        {
            connection->SetPhase(Phase::Wait);
            connection->SetStatus(QwfsStatus::CriticalErrorQuiche, "Failed to create quiche connection.");
            return QwfsResult::CriticalError;
        }

        auto result = connection->SetQlogSettings();
        if (QwfsResult::Ok != result)
        {
            return result;
        }

        connection->SetPhase(Phase::Handshake);
        return QwfsResult::Ok;
    }

    QwfsResult Connection::UpdatePhaseHandshake(Connection* connection) const
    {
        if (quiche_conn_is_established(connection->_quicheConnection))
        {
            // �m�������R�l�N�V�����̃o�[�W�������m�F�������ꍇ�̏���
            const uint8_t* app_proto;
            size_t app_proto_len;
            quiche_conn_application_proto(connection->_quicheConnection, &app_proto, &app_proto_len);
            if (nullptr != _debugOutput)
            {
                std::stringstream ss;
                std::string protocol(reinterpret_cast<const char*>(app_proto), app_proto_len);
                ss << "[qwfs info][" << _hostName << "]Connection established: " << protocol;
                ss << std::endl;
                connection->DebugOutput(ss.str().c_str());
            }

            // HTTP/3 �R�l�N�V�����̍쐬(���̃^�C�~���O�ł͂܂��ʐM���Ȃ�)
            if (nullptr != connection->_quicheH3Connection)
            {
                quiche_h3_conn_free(connection->_quicheH3Connection);
            }
            connection->_quicheH3Connection = quiche_h3_conn_new_with_transport(connection->_quicheConnection, connection->_quicheH3Config);
            if (nullptr == connection->_quicheH3Connection)
            {
                connection->SetPhase(Phase::StartShutdown);
                return connection->SetStatus(QwfsStatus::CriticalErrorQuiche, "Failed to quiche_h3_conn_new_with_transport");
            }

            connection->SetPhase(Phase::Wait);
        }

        return QwfsResult::Ok;
    }

    // ���N�G�X�g�J�n�҂�
    QwfsResult Connection::UpdatePhaseWait(Connection* connection) const
    {
        // ���g���C�v�������Ă��邩�m�F
        if (connection->_callRetry)
        {
            connection->DebugOutput("[qwfs info]Start retry");

            // �G���[�̃��X�g��߂�
            while (!connection->_h3StreamsErrorStock.empty())
            {
                auto itr = connection->_h3StreamsErrorStock.begin();
                connection->_h3StreamsStock.push_back(std::move(*itr));
                connection->_h3StreamsErrorStock.pop_front();
            }
            if (QwfsStatus::Wait > _status)
            {
                // ������ԓ����N���A���Ȃ��Ƃ����Ȃ��̂� quiche_conn ����蒼���Ȃ��Ƃ����Ȃ�
                connection->SetPhase(Phase::Initialize);
                connection->SetStatus(QwfsStatus::Wait);
            }
            connection->_callRetry = false;
            return QwfsResult::Ok;
        }

        // �G���[��Ԃ̍ۂ� retry �ɂ��ĊJ���Ȃ�����͏������Ȃ�
        if (QwfsStatus::Wait > connection->_status)
        {
            return QwfsResult::Ok;
        }

        // �ʐM����
        // quiche ���� HTTP �̃C�x���g�����Ă��邩�`�F�b�N
        // �C�x���g�����؂�̂� while �ŉ� �� stream id �P�ʂŉ��x���C�x���g������
        while (1)
        {
            quiche_h3_event* event;
            auto streamId = quiche_h3_conn_poll(connection->_quicheH3Connection, connection->_quicheConnection, &event);
            if (0 > streamId)
            {
                break;
            }
            // todo : �߂�l�m�F
            switch (quiche_h3_event_type(event))
            {
            case QUICHE_H3_EVENT_HEADERS:
                connection->_h3StreamsMap[streamId]->EventHeaders(event);
                break;
            case QUICHE_H3_EVENT_DATA:
                connection->_h3StreamsMap[streamId]->EventData();
                break;
            case QUICHE_H3_EVENT_FINISHED:
                connection->_h3StreamsMap[streamId]->EventFinished();
                connection->_totalWriteSize += connection->_h3StreamsMap[streamId]->GetWriteSize();
                break;
            }
            quiche_h3_event_free(event);
        }

        // �ǉ����N�G�X�g�Ή�
        auto result = connection->StartStream();
        if (QwfsResult::Ok != result)
        {
            // �G���[�����͓����ł���Ă�̂ŕԂ�����
            return result;
        }

        // �ʐM�����`�F�b�N
        if (connection->_h3Streams.empty())
        {
            connection->SetStatus(QwfsStatus::Wait);
        }

        return QwfsResult::Ok;
    }

    QwfsResult Connection::UpdatePhaseStartShutdown(Connection* connection) const
    {
        // HTTP/3 shutdown
        connection->ShutdownStream();

        connection->SetPhase(Phase::WaitShutdown);
        return QwfsResult::Ok;
    }

    QwfsResult Connection::UpdatePhaseWaitShutdown(Connection* connection) const
    {
        if (!quiche_conn_is_established(connection->_quicheConnection) || quiche_conn_is_closed(connection->_quicheConnection))
        {
            // close socket
            // todo : closesocket �֘A�̂����Ƃ�������
            closesocket(connection->_sock);
            connection->_sock = INVALID_SOCKET;

            if (connection->_callAbort)
            {
                connection->DebugOutput("[qwfs info]Start abort");
                connection->_callAbort = false;
                connection->SetPhase(Phase::CreateSocket);
                // �X�e�[�^�X�� wait �ɖ߂��Ă����āA���N�G�X�g���ς܂ꂽ��ĊJ�ł���悤�ɂ���
                connection->SetStatus(QwfsStatus::Wait);
            }
            else
            {
                // �G���[���̓��N�G�X�g��ۑ����Ă����� Retry ���Ɏ����ōĔ��s�ł���悤�ɂ��Ă���
                for (auto& stream : connection->_h3Streams)
                {
                    connection->_h3StreamsStock.emplace_front(std::move(stream));
                }
                connection->_h3Streams.clear();
                connection->_h3StreamsMap.clear();
                connection->_h3StreamsErrorStock.swap(connection->_h3StreamsStock);
                // �������� wait �ɖ߂��ă��g���C�J�n�� Abort ��҂�
                connection->SetPhase(Phase::Wait);
            }
        }

        return QwfsResult::Ok;
    }

    QwfsResult Connection::SetStatus(QwfsStatus status, const char* message)
    {
        _status = status;
        if (nullptr != message)
        {
            std::stringstream ss;
            ss << "[qwfs error][" << _hostName << "]" << message << std::endl;
            _errorDetail = ss.str().c_str();
            DebugOutput(_errorDetail.c_str());
        }
        else
        {
            _errorDetail.clear();
        }

        return ConvertStatusToResult(_status);
    }

    void Connection::SetPhase(Phase phase)
    {
        if (nullptr != _debugOutput)
        {
            std::stringstream ss;
            ss << "[qwfs info][" << _hostName << "]Change qwfs phase : " << PhaseStr[static_cast<int>(_phase)] << " -> " << PhaseStr[static_cast<int>(phase)] << std::endl;
            DebugOutput(ss.str().c_str());
        }

        switch (phase)
        {
            case Phase::CreateSocket:
                _updateFunc = [&](Connection* connection) { return UpdatePhaseCreateSocket(connection); };
                break;
            case Phase::Initialize:
                _updateFunc = [&](Connection* connection) { return UpdatePhaseInitialize(connection); };
                break;
            case Phase::Handshake:
                _updateFunc = [&](Connection* connection) { return UpdatePhaseHandshake(connection); };
                break;
            case Phase::Wait:
                _updateFunc = [&](Connection* connection) { return UpdatePhaseWait(connection); };
                break;
            case Phase::StartShutdown:
                _updateFunc = [&](Connection* connection) { return UpdatePhaseStartShutdown(connection); };
                break;
            case Phase::WaitShutdown:
                _updateFunc = [&](Connection* connection) { return UpdatePhaseWaitShutdown(connection); };
                break;
            default:
                _updateFunc = nullptr;
                break;
        }
        _phase = phase;
    }

    QwfsResult Connection::ConvertStatusToResult(QwfsStatus status) const
    {
        switch (status)
        {
        default:
        case QwfsStatus::CriticalError:
        case QwfsStatus::CriticalErrorQuiche:
            return QwfsResult::CriticalError;

        case QwfsStatus::Error:
        case QwfsStatus::ErrorSocket:
        case QwfsStatus::ErrorResolveHost:
        case QwfsStatus::ErrorTimeout:
        case QwfsStatus::ErrorConnectionClose:
        case QwfsStatus::ErrorInvalidResponse:
        case QwfsStatus::ErrorNetwork:
            return QwfsResult::Error;

        case QwfsStatus::ErrorFileIO:
            return QwfsResult::ErrorFileIO;

        case QwfsStatus::Wait:
        case QwfsStatus::Connecting:
        case QwfsStatus::Completed:
            return QwfsResult::Ok;
        }
    }

    QwfsStatus Connection::ConvertQuicheErrorToStatus(quiche_error error, QwfsStatus nowStatus) const
    {
        // �ǂ��Ή����Ă����̂��d�l�����L����Ă��炸������Ȃ����̂��������A�ꕔ�G���[�ɂ���Ƃ܂������̂�����̂ŃR���o�[�g����
        switch (error)
        {
            // �����I�ɒʐM���s�Œ��肻���Ȃ���
            case QUICHE_ERR_DONE:
            case QUICHE_ERR_FLOW_CONTROL:
            case QUICHE_ERR_FINAL_SIZE:
            case QUICHE_ERR_CONGESTION_CONTROL:
                return nowStatus;

            // ���g���C�Œ��肻���Ȃ���
            case QUICHE_ERR_STREAM_RESET:
                return QwfsStatus::Error;   // �K���ȃX�e�[�^�X��Ԃ��� Connection ���Ń��g���C���Ă��炤

            // ������̎����~�X�Ŕ����������
            case QUICHE_ERR_BUFFER_TOO_SHORT:
            case QUICHE_ERR_STREAM_LIMIT:
                return QwfsStatus::CriticalError;

            // �ؖ����̐ݒ�Ŕ������� TLS �֘A�̃G���[
            case QUICHE_ERR_TLS_FAIL:
                // �\�P�b�g�N���[�Y�����蒼���K�v������
                return QwfsStatus::ErrorTlsInvalidCert;

            // �ؖ����̐ݒ�ȊO�� TLS �֘A�̃G���[
            case QUICHE_ERR_CRYPTO_FAIL:
                return QwfsStatus::ErrorTls;

            // �T�[�o����̃f�[�^�������������̓����G���[���ۂ����́B�T�[�o�̖��Ȃ̂Ōq�������Ă��_��
            case QUICHE_ERR_UNKNOWN_VERSION:
            case QUICHE_ERR_INVALID_FRAME:
            case QUICHE_ERR_INVALID_PACKET:
            case QUICHE_ERR_INVALID_STATE:
            case QUICHE_ERR_INVALID_STREAM_STATE:
            case QUICHE_ERR_INVALID_TRANSPORT_PARAM:
                return QwfsStatus::ErrorInvalidServer;

            default:
                return QwfsStatus::CriticalErrorQuiche;
        }
    }

    void Connection::DebugOutput(const char* message)
    {
        if (nullptr != _debugOutput)
        {
            _debugOutput(message);
        }
    }
}