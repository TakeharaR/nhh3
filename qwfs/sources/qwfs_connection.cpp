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
        // quiche_conn_close を呼んでおく
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

        // todo : 終了処理待ち

        ClearConfig();

#ifdef _WINDOWS
        WSACleanup();
#endif

#if 0
        // Unity Editor から Uninitizalie が呼ばれる際に既に DebugOutput の参照先が消えている場合があるので基本コメントアウト
        // デバッグにしたい時などに有効化すること
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
        // コネクション生成後の変更は接続完了後にやる場合に切断処理を挟まなければならず面倒なのでとりあえず初期化前だけに限定
        if (Phase::Initialize != _phase)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        _options = options;
        
        // 暫定。オプションの設定の仕方は再考の余地がある
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
        // エラー中も積めるがステータスは変わらない(通信処理はリトライしないと開始しない)
        if (QwfsStatus::Wait <= _status)
        {
            if (QwfsStatus::Wait == _status)
            {
                // Wait の時は再スタートとしてプログレスを初期化する
                _totalWriteSize = 0U;
            }
            SetStatus(QwfsStatus::Connecting);
        }

        auto stream = std::make_unique<Stream>(externalId, saveFilePath, _hostName.c_str(), path, headers, headersSize, _callbacks, _debugOutput);
        // ストックに貯めておいて準備ができたら通信開始する
        _h3StreamsStock.push_back(std::move(stream));

        return QwfsResult::Ok;
    }

    QwfsResult Connection::Update()
    {
        // 送受信は必ず呼ぶ (ソケットが無効の際は内部で何もしない)
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
            // 開始待ちの時は何も処理をしない
            return ConvertStatusToResult(_status);
        }

        // フェイズ固有の処理を実施
        auto result = _updateFunc(this);
        if (QwfsResult::Ok > result)
        {
            return result;
        }

        // コネクションのクローズ確認
        CheckConnectionClosed();

        // コネクションのタイムアウト確認
        CheckConnectionTimeout();

        return QwfsResult::Ok;
    }

    void Connection::IssueCallbacks()
    {
        // 終了確認
        auto beginItr = _h3Streams.begin();
        auto endItr = _h3Streams.end();
        auto newEndItr = remove_if(beginItr, endItr, [&](std::unique_ptr<Stream>& stream) -> bool
        {
            // 成否確認
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
                    // 進行不能なエラーの際はシャットダウンへ
                    SetPhase(Phase::StartShutdown);
                    SetStatus(status, stream->GetErrorDetail());
                    // リトライしたいのでリストからの削除はしない
                }
                else
                {
                    // 投げ直しで何とかなるネットワークエラー系の場合は自動リトライ
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

    // QUIC 固有の設定を行う
    quiche_config* Connection::CreateQuicheConfig(const QwfsOptions& options) const
    {
        // 引数には QUIC のバージョンを渡す
        // バージョンネゴシエーションを試したい時は 0xbabababa を渡すこと
        quiche_config* config = quiche_config_new(QUICHE_PROTOCOL_VERSION);
        if (config == nullptr)
        {
            return nullptr;
        }

        // quiche に HTTP/3 の ALPN token を設定する -> quiche.h に定義されている QUICHE_H3_APPLICATION_PROTOCOL を渡せばいい
        quiche_config_set_application_protos(config, (uint8_t*)QUICHE_H3_APPLICATION_PROTOCOL, sizeof(QUICHE_H3_APPLICATION_PROTOCOL) - 1); // 引数エラー以外返らないので無視

        quiche_config_verify_peer(config, options._verifyPeer);                                                                 // 証明書の検証の on/off 。オレオレ証明書を使う際には false にする
        if (options._quicOprtions._enableEarlyData)                                                                             // 0-RTT を受け入れる場合は呼び出す
        {
            quiche_config_enable_early_data(config);
        }
        quiche_config_set_cc_algorithm(config, static_cast<quiche_cc_algorithm>(options._quicOprtions._ccType));

        quiche_config_set_max_idle_timeout(config, options._quicOprtions._maxIdleTimeout);                                      // max_idle_timeout の設定(ミリ秒)。 0 を指定する事でタイムアウトが無制限になる
        quiche_config_set_max_recv_udp_payload_size(config, options._quicOprtions._maxUdpPayloadSize);                          // max_udp_payload_size (受信時)の設定。 PMTU の実装を鑑みて設定を行うこと(「14. Packet Size」参照)
        quiche_config_set_max_send_udp_payload_size(config, options._quicOprtions._maxUdpPayloadSize);                          // max_udp_payload_size (送信時)の設定。 PMTU の実装を鑑みて設定を行うこと(「14. Packet Size」参照)
        quiche_config_set_initial_max_data(config, options._quicOprtions._initialMaxData);                                      // initial_max_data の設定(コネクションに対する上限サイズ)
        quiche_config_set_initial_max_stream_data_bidi_local(config, options._quicOprtions._initialMaxStreamDataBidiLocal);     // initial_max_stream_data_bidi_local の設定(ローカル始動の双方向ストリームの初期フロー制御値)
        quiche_config_set_initial_max_stream_data_bidi_remote(config, options._quicOprtions._initialMaxStreamDataBidiRemote);   // initial_max_stream_data_bidi_remote の設定(ピア始動の双方向ストリームの初期フロー制御値)
        quiche_config_set_initial_max_stream_data_uni(config, options._quicOprtions._initialMaxStreamDataUni);                  // initial_max_stream_data_uni の設定(単方向ストリームの初期フロー制御値)
        quiche_config_set_initial_max_streams_bidi(config, options._quicOprtions._initialMaxStreamsBidi);                       // initial_max_streams_bidi の設定(作成可能な双方向ストリームの最大値)
        quiche_config_set_initial_max_streams_uni(config, options._quicOprtions._initialMaxStreamsUni);                         // initial_max_streams_uni の設定(作成可能な単方向ストリームの最大値)
        quiche_config_set_disable_active_migration(config, options._quicOprtions._disableActiveMigration);                      // disable_active_migration の設定(コネクションマイグレーションに対応していない場合は false にする)
        quiche_config_set_max_connection_window(config, options._quicOprtions._maxConnectionWindowSize);                        // コネクションに用いる最大ウィンドウサイズ
        quiche_config_set_max_stream_window(config, options._quicOprtions._maxStreamWindowSize);                                // ストリームに用いる最大ウィンドウサイズ
        quiche_config_set_active_connection_id_limit(config, options._quicOprtions._activeConnectionIdLimit);                   // アクティブなコネクション ID の上限値
        
        // ステートレスリセット用トークンの設定。現状未対応
        // quiche_config_set_stateless_reset_token

        // 以下は qwfs の用途では基本的にはサーバ側の設定なのでスルー
        // quiche_config_set_max_ack_delay
        // quiche_config_set_ack_delay_exponent
        // quiche_config_load_priv_key_from_pem_file
        // quiche_config_set_cc_algorithm
        // quiche_config_enable_hystart
        // quiche_config_enable_pacing

        // DATAGRAM 対応 : 現状未対応
        // quiche_config_enable_dgram

        // wireshark 等でキャプチャしたい場合
        // quiche_config_log_keys

        return config;
    }

    // HTTP/3 用のコンフィグを作成する
    quiche_h3_config* Connection::CreateQuicheHttp3Config(const QwfsOptions& options) const
    {
        quiche_h3_config* config = quiche_h3_config_new();
        if (config == nullptr)
        {
            return nullptr;
        }

        quiche_h3_config_set_max_field_section_size(config, options._h3Oprtions._maxHeaderListSize);            // SETTINGS_MAX_HEADER_LIST_SIZE の設定。ヘッダリストに登録できるヘッダの最大数
        quiche_h3_config_set_qpack_max_table_capacity(config, options._h3Oprtions._qpackMaxTableCapacity);      // SETTINGS_QPACK_MAX_TABLE_CAPACITY の設定。QPACK の動的テーブルの最大値
        quiche_h3_config_set_qpack_blocked_streams(config, options._h3Oprtions._qpackBlockedStreams);           // SETTINGS_QPACK_BLOCKED_STREAMS の設定。ブロックされる可能性のあるストリーム数
        quiche_h3_config_enable_extended_connect(config, options._h3Oprtions._settingsEnableConnectProtocol);   // SETTINGS_ENABLE_CONNECT_PROTOCOL の設定。拡張 CONNECT プロトコルの設定。HTTP/3 用ライブラリなので基本は false 想定

        return config;
    }

    // quiche のコネクションを生成する関数
    quiche_conn* Connection::CreateQuicheConnection(const char* host, quiche_config* config) const
    {
        // Initial Packet で使用する SCID を乱数から生成する
        // SCID は QUIC バージョン 1 までは 20 バイト以内に抑える必要がある(暫定で quiche の example の設定値に準拠)
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

        // quiche のコネクションハンドルを作成する。この段階ではまだ通信は実施されない
        return quiche_connect(host, (const uint8_t*)scid, sizeof(scid),
            (struct sockaddr*)&_localAddInfo, localAddrLen,
            _peerAddInfo->ai_addr, _peerAddInfo->ai_addrlen,
            config);
    }

    // 非同期 UDP ソケットの生成関数
    // todo : とりあえずブロックで処理しているが後で整理
    QwfsResult Connection::CreateUdpSocket()
    {
        if (INVALID_SOCKET != _sock)
        {
            // 経路によってはここに入るので解放してあげる
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
            // 準備がまだなだけでエラーではない
            return QwfsResult::Ok;
        }

        uint8_t sendBuffer[MAX_DATAGRAM_SIZE] = { 0 };

        // quiche によって送信データ生成する
        // quiche_conn_send を呼ぶだけで、内部のステータス(コネクションやストリームの状況)に応じて適切なデータを生成してくれる
        // 一回で送り切れない場合があるのでループで取り切る必要がある
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

            // quiche が生成したデータを UDP ソケットで送信する
            auto sendSize = sendto(_sock, (const char*)sendBuffer, (int)writeSize, 0, (struct sockaddr*)&sendInfo.to, sendInfo.to_len);
            if (sendSize != (int)writeSize)
            {
                // 切断 → ソケットの作成からやり直す必要がある
                // todo : シャットダウンがうまくいかないかもしれない場合のケア
                // todo : ソケットエラーの内容に合わせたハンドリング
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
            // 準備がまだなだけでエラーではない
            return QwfsResult::Ok;
        }

        char receiveBuffer[MAX_DATAGRAM_SIZE] = { 0 };

        // quiche_conn_send 一回で取り切れない場合があるのでループで取り切る必要がある
        // todo : 処理が長引いた場合の一旦中断
        while (1)
        {
            // UDP パケットを受信
            struct sockaddr_storage recvPeerAddr;
            socklen_t recvPeerAddrLen = sizeof(struct sockaddr_in6);
            memset(&recvPeerAddr, 0, recvPeerAddrLen);
            ssize_t recvSize = recvfrom(_sock, receiveBuffer, sizeof(receiveBuffer), 0, (struct sockaddr*)&recvPeerAddr, &recvPeerAddrLen);
            if (SOCKET_ERROR >= recvSize)
            {
                auto error = WSAGetLastError();
                if (WSAEWOULDBLOCK != error)
                {
                    // 切断 → ソケットの作成からやり直す必要がある
                    // todo : シャットダウンがうまくいかないかもしれない場合のケア
                    // todo : ソケットエラーの内容に合わせたハンドリング
                    SetPhase(Phase::StartShutdown);
                    std::stringstream ss;
                    ss << "Failed to recv packet(Erroris " << error << ")." << std::endl;
                    return SetStatus(QwfsStatus::ErrorSocket, ss.str().c_str());
                }
                break;
            }

            // 受信したパケットを quiche に渡す
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
                // 主に実装ミスかメモリ不足
                std::stringstream ss;
                ss << "Failed to process packet(quiche_error is " << readSize << ")." << std::endl;
                auto result = SetStatus(ConvertQuicheErrorToStatus(static_cast<quiche_error>(readSize), _status), ss.str().c_str());
                if (QwfsStatus::ErrorTls == _status)
                {
                    SetPhase(Phase::StartShutdown);
                }
                else
                {
                    // todo : 致命的エラーの処遇. ConvertQuicheErrorToStatus 内でやる方が楽かも
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
            // そもそもハンドシェイクが終わってない場合、クローズ処理を開始している場合は確認不要
            return;
        }

        if (quiche_conn_is_closed(_quicheConnection))
        {
            // サーバから切断された時に入る
            // こちらからも close してあげる必要がある
            SetStatus(QwfsStatus::ErrorConnectionClose, "Connection close from server");
            SetPhase(Phase::StartShutdown);
        }
    }

    void Connection::CheckConnectionTimeout()
    {
        // todo : きちんとした挙動の理解
        if (
            (nullptr == _quicheConnection)  ||
            (_phase < Phase::Handshake)     ||
            (_status < QwfsStatus::Wait)
            )
        {
            // クローズ処理を開始している場合は確認不要
            return;
        }

        auto timeout = quiche_conn_timeout_as_millis(_quicheConnection);
        if (0U == timeout)
        {
            // タイムアウトイベントを呼ぶ
            quiche_conn_on_timeout(_quicheConnection);

            if (!(_status == QwfsStatus::Aborting) && (quiche_conn_is_closed(_quicheConnection)))
            {
                // クローズしてあげる
                SetPhase(Phase::StartShutdown);
                SetStatus(QwfsStatus::ErrorTimeout, "Connection close");    // ソケットエラー時とかにも入るのでメッセージも timeout に言及しない
            }
        }
    }

    QwfsResult Connection::StartStream()
    {
        // 現在処理中のリクエストに空きが無ければ抜ける
        if ((_h3Streams.size() >= _options._maxConcurrentStreams) || (_h3StreamsStock.size() == 0))
        {
            return QwfsResult::Ok;
        }

        // Wait または Connecting の時しか新規開始できない
        if ((QwfsStatus::Wait != _status) && (QwfsStatus::Connecting != _status))
        {
            return QwfsResult::Ok;
        }

        // ここまでくればリクエストの処理をして OK
        uint64_t num = 0;
        auto canRequestNum = _options._maxConcurrentStreams - _h3Streams.size();
        for (; num < canRequestNum; ++num)
        {
            auto stream = _h3StreamsStock.begin();      // 下の _h3StreamsStock 内で _h3StreamsStock は再構築されるので毎回 begin で確認
            if (_h3StreamsStock.end() == stream)
            {
                break;
            }
            auto streamId = (*stream)->Start(_quicheConnection, _quicheH3Connection);
            if (0 > streamId)
            {
                if (-12 == streamId)
                {
                    // ストリームの作成の限界を超えている場合に -12 が返ってくる
                    // quiche 側の持つ peer の initial_max_streams_bidi の値が MAX_STREAMS により更新されるはずなので、しばらくまってリトライすれば解消するはず(現状しない場合もあります……)
                    break;
                }
                SetPhase(Phase::Wait);
                return SetStatus(QwfsStatus::CriticalErrorQuiche, "Failed to create stream");
            }
            _h3StreamsMap.emplace(streamId, stream->get());  // stream id は固有値なので find 確認はしない
            _h3Streams.push_back(std::move(*stream));
            _h3StreamsStock.pop_front();
        }

        if ((0 < num) && (QwfsStatus::Wait == _status))
        {
            // wait 状態であれば新規開始として Connecting にしてプログラスも 0 から始め直す
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
        // todo ; クローズの理由をちゃんと入れる
        quiche_conn_close(_quicheConnection, false, 0, nullptr, 0);
        _callAbort = true;

        // リストの全クリア。このタイミングでしないと IssueCallbacks で成功したものが引っかかってしまう
        _h3StreamsMap.clear();
        _h3Streams.clear();
        _h3StreamsErrorStock.clear();
        _h3StreamsStock.clear();

        SetPhase(Phase::StartShutdown);
        SetStatus(QwfsStatus::Aborting);
    }

    // パケットの初期化の実施
    QwfsResult Connection::UpdatePhaseCreateSocket(Connection* connection) const
    {
        auto result = connection->CreateUdpSocket();
        if (QwfsResult::Ok != result)
        {
            // ソケットからの再生成を待つ(エラー遷移は CreateUdpSocket 内で実施)
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
            // 確立したコネクションのバージョンを確認したい場合の処理
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

            // HTTP/3 コネクションの作成(このタイミングではまだ通信しない)
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

    // リクエスト開始待ち
    QwfsResult Connection::UpdatePhaseWait(Connection* connection) const
    {
        // リトライ要求が来ているか確認
        if (connection->_callRetry)
        {
            connection->DebugOutput("[qwfs info]Start retry");

            // エラーのリストを戻す
            while (!connection->_h3StreamsErrorStock.empty())
            {
                auto itr = connection->_h3StreamsErrorStock.begin();
                connection->_h3StreamsStock.push_back(std::move(*itr));
                connection->_h3StreamsErrorStock.pop_front();
            }
            if (QwfsStatus::Wait > _status)
            {
                // 内部状態等をクリアしないといけないので quiche_conn も作り直さないといけない
                connection->SetPhase(Phase::Initialize);
                connection->SetStatus(QwfsStatus::Wait);
            }
            connection->_callRetry = false;
            return QwfsResult::Ok;
        }

        // エラー状態の際は retry により再開しない限りは処理しない
        if (QwfsStatus::Wait > connection->_status)
        {
            return QwfsResult::Ok;
        }

        // 通信処理
        // quiche 側に HTTP のイベントが来ているかチェック
        // イベントを取り切るので while で回す → stream id 単位で何度もイベントが来る
        while (1)
        {
            quiche_h3_event* event;
            auto streamId = quiche_h3_conn_poll(connection->_quicheH3Connection, connection->_quicheConnection, &event);
            if (0 > streamId)
            {
                break;
            }
            // todo : 戻り値確認
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

        // 追加リクエスト対応
        auto result = connection->StartStream();
        if (QwfsResult::Ok != result)
        {
            // エラー処理は内部でやってるので返すだけ
            return result;
        }

        // 通信完了チェック
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
            // todo : closesocket 関連のちゃんとした処理
            closesocket(connection->_sock);
            connection->_sock = INVALID_SOCKET;

            if (connection->_callAbort)
            {
                connection->DebugOutput("[qwfs info]Start abort");
                connection->_callAbort = false;
                connection->SetPhase(Phase::CreateSocket);
                // ステータスは wait に戻してあげて、リクエストが積まれたら再開できるようにする
                connection->SetStatus(QwfsStatus::Wait);
            }
            else
            {
                // エラー時はリクエストを保存しておいて Retry 時に自動で再発行できるようにしておく
                for (auto& stream : connection->_h3Streams)
                {
                    connection->_h3StreamsStock.emplace_front(std::move(stream));
                }
                connection->_h3Streams.clear();
                connection->_h3StreamsMap.clear();
                connection->_h3StreamsErrorStock.swap(connection->_h3StreamsStock);
                // 処理だけ wait に戻してリトライ開始か Abort を待つ
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
        // どう対応していいのか仕様が明記されておらず分からないものも多いが、一部エラーにするとまずいものがあるのでコンバートする
        switch (error)
        {
            // 内部的に通信続行で直りそうなもの
            case QUICHE_ERR_DONE:
            case QUICHE_ERR_FLOW_CONTROL:
            case QUICHE_ERR_FINAL_SIZE:
            case QUICHE_ERR_CONGESTION_CONTROL:
                return nowStatus;

            // リトライで直りそうなもの
            case QUICHE_ERR_STREAM_RESET:
                return QwfsStatus::Error;   // 適当なステータスを返して Connection 側でリトライしてもらう

            // こちらの実装ミスで発生するもの
            case QUICHE_ERR_BUFFER_TOO_SHORT:
            case QUICHE_ERR_STREAM_LIMIT:
                return QwfsStatus::CriticalError;

            // 証明書の設定で発生する TLS 関連のエラー
            case QUICHE_ERR_TLS_FAIL:
                // ソケットクローズからやり直す必要がある
                return QwfsStatus::ErrorTlsInvalidCert;

            // 証明書の設定以外の TLS 関連のエラー
            case QUICHE_ERR_CRYPTO_FAIL:
                return QwfsStatus::ErrorTls;

            // サーバからのデータがおかしい等の内部エラーっぽいもの。サーバの問題なので繋ぎ直してもダメ
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