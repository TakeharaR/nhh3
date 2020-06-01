#pragma once

#include "qwfs_types.h"

#ifdef _WINDOWS
#define DllExport __declspec (dllexport)
#elif defined(__ANDROID)
#define DllExport __attribute__((visibility("default")))
#else
#define DllExport
#endif

#ifdef __cplusplus
extern "C" {
#endif
    DllExport void qwfsInitialize();
    DllExport void qwfsUninitialize();
    DllExport QwfsId qwfsCreate(const char* hostName, const char* port, QwfsCallbacks callbacks);
    DllExport QwfsResult qwfsDestroy(QwfsId hostId);
    DllExport QwfsResult qwfsSetOptions(QwfsId hostId, QwfsOptions options);
    DllExport QwfsResult qwfsGetRequest(QwfsId hostId, QwfsId externalId, const char* path, const char* saveFilePath, QwfsRequestHeaders* headers, uint64_t headersSize);
    DllExport QwfsResult qwfsUpdate(QwfsId hostId);
    DllExport QwfsResult qwfsIssueCallbacks(QwfsId hostId);
    DllExport QwfsResult qwfsGetStatus(QwfsId hostId, QwfsStatus* status);
    DllExport QwfsResult qwfsGetProgress(QwfsId hostId, uint64_t* progress, uint64_t* totalWriteSize);
    DllExport QwfsResult qwfsRetry(QwfsId hostId);
    DllExport QwfsResult qwfsAbort(QwfsId hostId);
    DllExport const char* qwfsGetErrorDetail(QwfsId hostId);
    DllExport QwfsResult qwfsSetDebugOutput(DebugOutputCallback debugOutput);
#ifdef __cplusplus
}
#endif
