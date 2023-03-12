#pragma once

#include <list>
#include <deque>
#include <array>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <chrono>

#include "quiche.h"

#include "qwfs_types.h"
#include "qwfs_h3_stream.h"
#include "qwfs_socket.h"

namespace qwfs
{
    class Connection
    {
    public:
        enum class Phase
        {
            Uninitialize = 0,
            CreateSocket,
            Initialize,
            Handshake,
            Wait,
            StartShutdown,
            WaitShutdown,
            ConnectionMigration,
            Max,
        };

        inline static const char* PhaseStr[(int)Phase::Max] =
        {
            "Uninitialize",
            "CreateSocket",
            "Initialize",
            "Handshake",
            "Wait",
            "StartShutdown",
            "WaitShutdown",
            "ConnectionMigration",
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
        QwfsResult Reconnect();

        inline const char* GetHostName() const { return _hostName.c_str(); }
        inline QwfsId GetQwfsId() const { return _id; }
        inline QwfsStatus GetStatus() const { return _status; }
        inline const char* GetErrorDetail() const { return _errorDetail.c_str(); }


    private:
        // constant
        static const long long ConnectionMigrationTimeOut = 5 * 1000;    // msec
        static const int TokenLength = 16;    // msec

        // using
        using Token = std::array<uint8_t, TokenLength>;

        // valiables
        QwfsId                  _id;                // コネクション固有の ID
        QwfsStatus              _status;
        Phase                   _phase;
        std::string             _errorDetail;
        std::string             _hostName;
        std::string             _port;
        socketwrapper::Socket   _sock;
        struct addrinfo*        _peerAddInfo;
        struct addrinfo*        _localAddInfo;
        socklen_t               _localAddrLen;
        QwfsOptions             _options;
        std::string             _caCertsList;
        std::string             _qlogPath;
        std::string             _zeroRttSessionPath;
        QwfsCallbacks           _callbacks;
        DebugOutputCallback     _debugOutput;

        quiche_config*          _quicheConfig;
        quiche_h3_config*       _quicheH3Config;
        quiche_conn*            _quicheConnection;
        quiche_h3_conn*         _quicheH3Connection;

        std::deque<std::unique_ptr<Stream>>     _h3StreamsErrorStock;   // エラー状態になったリクエスト。リトライ時に _h3StreamsStock に移動される
        std::deque<std::unique_ptr<Stream>>     _h3StreamsStock;        // 通信開始待機中のリクエスト
        std::list<std::unique_ptr<Stream>>      _h3Streams;             // 通信開始したリクエスト
        std::unordered_map<uint64_t, Stream*>   _h3StreamsMap;

        bool                _callRetry;
        bool                _callAbort;
        bool                _isLoadedSessionForZeroRtt;
        bool                _settingsReceived;

        uint64_t    _totalWriteSize;        // これまでに完了したリクエストの合計サイズ。書き込み中のサイズは持たない。Connectiong からステータスが変わる際にクリアされる
        std::function<QwfsResult(Connection* connection)> _updateFunc;

        // quic info
        uint8_t _scid[QUICHE_MAX_CONN_ID_LEN];
        uint8_t _dcid[QUICHE_MAX_CONN_ID_LEN];
        uint8_t _odcid[QUICHE_MAX_CONN_ID_LEN];
        quiche_stats* _quicheStats;

        // for connection migration
        std::chrono::system_clock::time_point  _startTime;

        // functions
        void ClearConfig();
        void ClearQuicheConnection();
        void ClearForRetry();
        QwfsResult SetOptionsInternal();
        quiche_config* CreateQuicheConfig(const QwfsOptions& options) const;
        quiche_h3_config* CreateQuicheHttp3Config(const QwfsOptions& options) const;
        quiche_conn* CreateQuicheConnection(const char* host, quiche_config* config) const;
        Token CreateToken() const;
        QwfsResult CreateUdpSocket();
        void CloseUdpSocket();
        QwfsResult SetQlogSettings();
        QwfsResult Send();
        QwfsResult Receive();
        void CheckConnectionClosed();
        void CheckConnectionTimeout();
        void Timeout();
        QwfsResult StartStream();
        QwfsResult StartStream(std::unique_ptr<Stream>& stream);
        void RetryStream();
        void ShutdownStream();

        // for 0-RTT
        bool IsEnabledZeroRtt() const;
        bool SaveSessionForZeroRtt() const;
        bool LoadSessionForZeroRtt() const;
        void SettingsReceived() { _settingsReceived = true; };

        // for connection migration
        bool IsEnabledConnectionMigration() const;
        inline void StartConnectionMigrationTimer() { _startTime = std::chrono::system_clock::now(); };
        bool ConnectionMigrationHasTimedOut() const;
        QwfsResult TransitionConnectionMigration();

        // UpdatePhase
        QwfsResult UpdatePhaseInitialize(Connection* connection) const;
        QwfsResult UpdatePhaseCreateSocket(Connection* connection) const;
        QwfsResult UpdatePhaseHandshake(Connection* connection) const;
        QwfsResult UpdatePhaseWait(Connection* connection) const;
        QwfsResult UpdatePhaseStartShutdown(Connection* connection) const;
        QwfsResult UpdatePhaseWaitShutdown(Connection* connection) const;
        QwfsResult UpdatePhaseConnectionMigration(Connection* connection) const;

        // status
        QwfsResult SetStatus(QwfsStatus status, const char* message = nullptr);
        void SetPhase(Phase phase);
        QwfsResult ConvertStatusToResult(QwfsStatus status) const;
        QwfsStatus ConvertQuicheErrorToStatus(quiche_error error, QwfsStatus nowStatus) const;

        //
        void DebugOutput(const char* message) const;
    };
}