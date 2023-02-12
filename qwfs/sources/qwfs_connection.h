#pragma once

#ifdef _WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <list>
#include <deque>
#include <string>
#include <functional>
#include <memory>

#include "quiche.h"

#include "qwfs_types.h"
#include "qwfs_h3_stream.h"

namespace qwfs
{
    class Connection
    {
    public:
        enum class Phase
        {
            Uninitialize = 0,
            Initialize,
            CreateSocket,
            Handshake,
            Wait,
            StartShutdown,
            WaitShutdown,
        };

        const char* PhaseStr[7] =
        {
            "Uninitialize",
            "Initialize",
            "CreateSocket",
            "Handshake",
            "Wait",
            "StartShutdown",
            "WaitShutdown",
        };

        Connection() {};
        Connection(QwfsId id, const char* host, const char* port, const QwfsCallbacks& callbacks, DebugOutputCallback debugOutput);
        ~Connection();
        QwfsResult SetOptions(const QwfsOptions& options);
        QwfsResult PublishRequest(QwfsId externalId, const char* path, const char* saveFilePath, QwfsRequestHeaders* headers, uint64_t headersSize);
        QwfsResult Update();
        void IssueCallbacks();
        void GetProgress(uint64_t* progress, uint64_t* totalWriteSize);
        void Retry();
        void Abort();

        inline const char* GetHostName() const { return _hostName.c_str(); }
        inline QwfsId GetQwfsId() const { return _id; }
        inline QwfsStatus GetStatus() const { return _status; }
        inline const char* GetErrorDetail() const { return _errorDetail.c_str(); }

    private:
        // valiables
        QwfsId              _id;                // �R�l�N�V�����ŗL�� ID
        QwfsStatus          _status;
        Phase               _phase;
        std::string         _errorDetail;
        std::string         _hostName;
        std::string         _port;
        SOCKET              _sock;              // �����I�ɋ@���ˑ����b�s���O�w���쐬
        struct addrinfo*    _peerAddInfo;
        struct addrinfo*    _localAddInfo;
        QwfsOptions         _options;
        std::string         _caCertsList;
        std::string         _qlogPath;
        QwfsCallbacks       _callbacks;
        DebugOutputCallback _debugOutput;

        quiche_config*      _quicheConfig;
        quiche_h3_config*   _quicheH3Config;
        quiche_conn*        _quicheConnection;
        quiche_h3_conn*     _quicheH3Connection;

        std::deque<std::unique_ptr<Stream>>     _h3StreamsErrorStock;   // �G���[��ԂɂȂ������N�G�X�g�B���g���C���� _h3StreamsStock �Ɉړ������
        std::deque<std::unique_ptr<Stream>>     _h3StreamsStock;        // �ʐM�J�n�ҋ@���̃��N�G�X�g
        std::list<std::unique_ptr<Stream>>      _h3Streams;             // �ʐM�J�n�������N�G�X�g
        std::unordered_map<uint64_t, Stream*>   _h3StreamsMap;

        bool                _callRetry;
        bool                _callAbort;

        uint64_t    _totalWriteSize;        // ����܂łɊ����������N�G�X�g�̍��v�T�C�Y�B�������ݒ��̃T�C�Y�͎����Ȃ��BConnectiong ����X�e�[�^�X���ς��ۂɃN���A�����

        std::function<QwfsResult(Connection* connection)> _updateFunc;

        // functions
        void ClearConfig();
        void ClearQuicheConnection(Connection* connection) const;
        QwfsResult SetOptionsInternal();
        quiche_config* CreateQuicheConfig(const QwfsOptions& options) const;
        quiche_h3_config* CreateQuicheHttp3Config(const QwfsOptions& options) const;
        quiche_conn* CreateQuicheConnection(const char* host, quiche_config* config) const;
        QwfsResult CreateUdpSocket();
        QwfsResult SetQlogSettings();
        QwfsResult Send();
        QwfsResult Receive();
        void CheckConnectionClosed();
        void CheckConnectionTimeout();
        QwfsResult StartStream();
        QwfsResult StartStream(std::unique_ptr<Stream>& stream);
        void ShutdownStream();

        QwfsResult UpdatePhaseInitialize(Connection* connection) const;
        QwfsResult UpdatePhaseCreateSocket(Connection* connection) const;
        QwfsResult UpdatePhaseHandshake(Connection* connection) const;
        QwfsResult UpdatePhaseWait(Connection* connection) const;
        QwfsResult UpdatePhaseStartShutdown(Connection* connection) const;
        QwfsResult UpdatePhaseWaitShutdown(Connection* connection) const;

        QwfsResult SetStatus(QwfsStatus status, const char* message = nullptr);
        void SetPhase(Phase phase);
        QwfsResult ConvertStatusToResult(QwfsStatus status) const;
        QwfsStatus ConvertQuicheErrorToStatus(quiche_error error, QwfsStatus nowStatus) const;

        void DebugOutput(const char* message);
    };
}