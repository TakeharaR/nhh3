#pragma once

#include "qwfs_types.h"

namespace qwfs
{
    void Initialize();
    void Uninitialize();
    QwfsId Create(const char* hostName, const char* port, const QwfsCallbacks& callbacks);
    QwfsResult Destroy(QwfsId hostId);
    QwfsResult SetOptions(QwfsId hostId, const QwfsOptions& options);
    QwfsResult GetRequest(QwfsId hostId, QwfsId externalId , const char* path, const char* saveFilePath, QwfsRequestHeaders* headers, uint64_t headersSize);
    QwfsResult PostRequest(QwfsId hostId);
    QwfsResult Update(QwfsId hostId);
    QwfsResult IssueCallbacks(QwfsId hostId);
    QwfsResult GetStatus(QwfsId hostId, QwfsStatus* status);
    QwfsResult GetProgress(QwfsId hostId, uint64_t* progress, uint64_t* totalWriteSize);
    QwfsResult Retry(QwfsId hostId);
    QwfsResult Abort(QwfsId hostId);
    QwfsResult Reconnect(QwfsId hostId);
    const char* GetErrorDetail(QwfsId hostId);
    QwfsResult SetDebugOutput(DebugOutputCallback debugOutput);

}