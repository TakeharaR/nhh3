#pragma once

#include <string>
#include <vector>

#include "quiche.h"

#include "qwfs_types.h"

namespace qwfs
{
    class Stream
    {
    public:
        Stream(
            QwfsId externalId
            , const char* saveFilePath
            , const char* hostName
            , const char* path
            , QwfsRequestHeaders* userHeaders
            , uint64_t headersSize
            , const QwfsCallbacks& callbacks
            , DebugOutputCallback debugOutput
        );
        ~Stream();
        int64_t Start(quiche_conn* quicheConnection, quiche_h3_conn* quicheH3Connection);
        int64_t Retry();
        QwfsStatus EventHeaders(quiche_h3_event* event);
        QwfsStatus EventData();
        void EventFinished();
        void Shutdown(QwfsStatus status, const char* cause);
        void AddProgress(uint64_t* progress, uint64_t* totalWriteSize);
        void CallSuccessCallback(QwfsId hostId);
        void CallErrorCallback(QwfsId hostId);

        QwfsStatus SetStatus(QwfsStatus status, const char* errorDetail = nullptr);
        inline QwfsStatus GetStatus() { return _status; }
        inline const char* GetErrorDetail() { return _errorDetail.c_str(); }
        inline uint64_t GetWriteSize() { return _writeSize; }
        inline int64_t GetStreamId() { return _streamId; }

    private:
        struct RequestHeaders
        {
            std::string _name;
            std::string _value;
        
            RequestHeaders(const char* name, const char* value) : _name(name), _value(value) {};
        };

        // todo : 各種コールバックで _externalId を返すようにする
        QwfsId                      _externalId;    // 外部識別子(リトライ時にも同じ ID を使いたいのでストリーム ID とは別管理)

        // コンストラクタのみで初期化
        std::string                 _saveFilePath;
        const QwfsCallbacks&        _callbacks;
        DebugOutputCallback         _debugOutput;
        std::vector<RequestHeaders> _reqHeaders;
        quiche_h3_header*           _reqHeadersForQuiche;

        // リトライ時に初期化
        quiche_conn*                _quicheQuicConnection;   // 親の参照なので解放しないこと
        quiche_h3_conn*             _quicheH3Connection;     // 親の参照なので解放しないこと
        int64_t                     _streamId;
        QwfsStatus                  _status;
        std::string                 _errorDetail;
        FILE*                       _file;
        char*                       _body;
        uint64_t                    _bodySize;      // content-length ある場合のサイズ
        uint64_t                    _writeSize;     // 現在書き込んだサイズ
        uint64_t                    _progress;      // GetProgress で取得されるまでに書き込んだサイズの累計
        uint64_t                    _responseCode;
        std::vector<std::string>    _resHeaders;

        //読み込み時に使う一時変数
        uint8_t*                    _recvTmpBuf;
        ssize_t                     _recvTmpBufSize;

        // for debug
        const char*                 _hostName;

        // functions
        void Clear();
        QwfsStatus ConvertQuicheH3ErrorToStatus(quiche_h3_error error, QwfsStatus nowStatus) const;
    };
}