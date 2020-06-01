#include "qwfs.h"
#include "qwfs_quich_wrapper.h"

void qwfsInitialize()
{
    return qwfs::Initialize();
}

void qwfsUninitialize()
{
    return qwfs::Uninitialize();
}

QwfsId qwfsCreate(const char* hostName, const char* port, QwfsCallbacks callbacks)
{
    return qwfs::Create(hostName, port, callbacks);
}

QwfsResult qwfsDestroy(QwfsId hostId)
{
    return qwfs::Destroy(hostId);
}

QwfsResult qwfsSetOptions(QwfsId hostId, QwfsOptions options)
{
    return qwfs::SetOptions(hostId, options);
}

// todo : リクエストをまとめてリストで受け取る
QwfsResult qwfsGetRequest(QwfsId hostId, uint64_t externalId, const char* path, const char* saveFilePath, QwfsRequestHeaders* headers, uint64_t headersSize)
{
    return qwfs::GetRequest(hostId, externalId, path, saveFilePath, headers, headersSize);
}

QwfsResult qwfsPostRequest(QwfsId hostId)
{
    return qwfs::PostRequest(hostId);
}

QwfsResult qwfsUpdate(QwfsId hostId)
{
    return qwfs::Update(hostId);
}

QwfsResult qwfsIssueCallbacks(QwfsId hostId)
{
    return qwfs::IssueCallbacks(hostId);
}

QwfsResult qwfsGetStatus(QwfsId hostId, QwfsStatus* status)
{
    return qwfs::GetStatus(hostId, status);
}

QwfsResult qwfsGetProgress(QwfsId hostId, uint64_t* progress, uint64_t* totalWriteSize)
{
    return qwfs::GetProgress(hostId, progress, totalWriteSize);
}

QwfsResult qwfsRetry(QwfsId hostId)
{
    return qwfs::Retry(hostId);
}

QwfsResult qwfsAbort(QwfsId hostId)
{
    return qwfs::Abort(hostId);
}

const char* qwfsGetErrorDetail(QwfsId hostId)
{
    return qwfs::GetErrorDetail(hostId);
}

QwfsResult qwfsSetDebugOutput(DebugOutputCallback debugOutput)
{
    return qwfs::SetDebugOutput(debugOutput);
}




