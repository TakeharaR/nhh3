#pragma once

#include <unordered_map>

#include "qwfs_types.h"
#include "qwfs_connection.h"

namespace qwfs
{
    class ConnectionManager
    {
    public:
        ConnectionManager();
        Connection* Create(const char* hostName, const char* port, const QwfsCallbacks& callbacks, DebugOutputCallback debugOutput);
        QwfsResult Destroy(QwfsId id);
        Connection* GetConnection(QwfsId id);

    private:
        std::unordered_map<QwfsId, Connection>   _connections;      // todo : åüçıå¯ó¶âª
        QwfsId  _nextId;
    };

}