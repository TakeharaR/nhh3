#include <string>

#include "qwfs_types.h"
#include "qwfs_quich_wrapper.h"
#include "qwfs_connection_manager.h"

namespace qwfs
{
    static ConnectionManager* _connectionManager = nullptr;
    static DebugOutputCallback _debugOutput = nullptr;

    void Initialize()
    {
        Uninitialize();
        _connectionManager = new ConnectionManager;
    }

    void Uninitialize()
    {
        if (nullptr != _connectionManager)
        {
            delete _connectionManager;
        }
        _connectionManager = nullptr;
    }

    QwfsId Create(const char* hostName, const char* port, const QwfsCallbacks& callbacks)
    {
        if (nullptr == _connectionManager)
        {
            return INVALID_QWFS_ID;
        }

        auto connection = _connectionManager->Create(hostName, port, callbacks, _debugOutput);
        if (nullptr == connection)
        {
            return INVALID_QWFS_ID;
        }

        return connection->GetQwfsId();
    }

    QwfsResult Destroy(QwfsId hostId)
    {
        if (nullptr == _connectionManager)
        {
            return QwfsResult::ErrorInvalidCall;
        }
        return _connectionManager->Destroy(hostId);
    }

    QwfsResult SetOptions(QwfsId hostId, const QwfsOptions& options)
    {
        if (nullptr == _connectionManager)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }
        return connection->SetOptions(options);
    }

    QwfsResult GetRequest(QwfsId hostId, QwfsId externalId, const char* path, const char* saveFilePath, QwfsRequestHeaders* headers, uint64_t headersSize)
    {
        if (nullptr == _connectionManager)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }
        return connection->PublishRequest(externalId, path, saveFilePath, headers, headersSize);
    }

    QwfsResult PostRequest(QwfsId)
    {
        return QwfsResult::Ok;
    }

    QwfsResult Update(QwfsId hostId)
    {
        if (nullptr == _connectionManager)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }
        return connection->Update();
    }

    QwfsResult IssueCallbacks(QwfsId hostId)
    {
        if (nullptr == _connectionManager)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }
        connection->IssueCallbacks();
        return QwfsResult::Ok;
    }
    

    QwfsResult GetStatus(QwfsId hostId, QwfsStatus* status)
    {
        if (nullptr == _connectionManager)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        if (nullptr == status)
        {
            return QwfsResult::ErrorInvalidArg;
        }

        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }

        *status = connection->GetStatus();
        return QwfsResult::Ok;
    }

    QwfsResult GetProgress(QwfsId hostId, uint64_t* progress, uint64_t* totalWriteSize)
    {
        if (nullptr == _connectionManager)
        {
            return QwfsResult::ErrorInvalidCall;
        }

        if ((nullptr == progress) || (nullptr == totalWriteSize))
        {
            return QwfsResult::ErrorInvalidArg;
        }

        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }
        connection->GetProgress(progress, totalWriteSize);
        return QwfsResult::Ok;
    }

    QwfsResult Retry(QwfsId hostId)
    {
        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }
        connection->Retry();
        return QwfsResult::Ok;
    }

    QwfsResult Abort(QwfsId hostId)
    {
        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return QwfsResult::ErrorInvalidArg;
        }
        connection->Abort();
        return QwfsResult::Ok;
    }

    const char* GetErrorDetail(QwfsId hostId)
    {
        if (nullptr == _connectionManager)
        {
            return nullptr;
        }

        auto connection = _connectionManager->GetConnection(hostId);
        if (nullptr == connection)
        {
            return nullptr;
        }
        return connection->GetErrorDetail();
    }

    static void QuicheDebugOutput(const char* line, void* arg)
    {
        auto func = reinterpret_cast<DebugOutputCallback>(arg);
        func(line);
    }

    // 色々暫定
    QwfsResult SetDebugOutput(DebugOutputCallback debugOutput)
    {
        if (nullptr == debugOutput)
        {
            return QwfsResult::ErrorInvalidArg;
        }

#if 1
        _debugOutput = debugOutput;
#else
        // Unity エディタのプレイ開始 → 停止の繰り返しで static mut な Rust のログ管理変数が破壊されるので無効化中
        // todo : DLL の動的ロード・アンロード
        if (nullptr != _debugOutput)
        {
            // 一回しかセットできない
            return QwfsResult::ErrorInvalidCall;
        }

        // ここで設定したくないけど暫定
        _debugOutput = debugOutput;
        quiche_enable_debug_logging(QuicheDebugOutput, _debugOutput);
#endif

        return QwfsResult::Ok;
    }
}