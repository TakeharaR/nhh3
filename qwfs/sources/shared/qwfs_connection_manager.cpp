#include "qwfs_connection_manager.h"
#include "qwfs_connection.h"

namespace qwfs
{
    ConnectionManager::ConnectionManager() : _nextId(0U)
    {
    }

    Connection* ConnectionManager::Create(const char* hostName, const char* port, const QwfsCallbacks& callbacks, DebugOutputCallback debugOutput)
    {
        if ((nullptr == hostName) || (nullptr == port))
        {
            return nullptr;
        }

        auto createId = _nextId;
        _connections.emplace(std::piecewise_construct, std::forward_as_tuple(_nextId), std::forward_as_tuple(_nextId, hostName, port, callbacks, debugOutput));
        if (QwfsStatus::Error >= _connections[createId].GetStatus())
        {
            // åªèÛé∏îsÇµÇ»Ç¢
        }
        ++_nextId;
        if (INVALID_QWFS_ID == _nextId)
        {
            _nextId = 0U;
        }
        return &_connections[createId];
    }

    QwfsResult ConnectionManager::Destroy(QwfsId id)
    {
        if (_connections.find(id) == _connections.end())
        {
            return QwfsResult::ErrorInvalidArg;
        }
        _connections.erase(id);
        return QwfsResult::Ok;
    }

    Connection* ConnectionManager::GetConnection(QwfsId id)
    {
        if (_connections.find(id) == _connections.end())
        {
            return nullptr;
        }
        return &_connections[id];
    }
}