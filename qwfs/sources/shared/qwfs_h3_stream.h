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

    private:
        struct RequestHeaders
        {
            std::string _name;
            std::string _value;
        
            RequestHeaders(const char* name, const char* value) : _name(name), _value(value) {};
        };


        QwfsStatus      _status;
        std::string     _errorDetail;

        // �R���X�g���N�^�݂̂ŏ�����
        QwfsId                  _externalId;        // �O�����ʎq(���g���C���ɂ����� ID ���g�������̂ŃX�g���[�� ID �Ƃ͕ʊǗ�)
        std::string             _saveFilePath;
        const QwfsCallbacks&    _callbacks;
        DebugOutputCallback     _debugOutput;

        std::vector<RequestHeaders> _reqHeaders;
        quiche_h3_header*           _reqHeadersForQuiche;

        // ���g���C���ɏ�����
        quiche_conn*        _quicheQuicConnection;   // �e�̎Q�ƂȂ̂ŉ�����Ȃ�����
        quiche_h3_conn*     _quicheH3Connection;     // �e�̎Q�ƂȂ̂ŉ�����Ȃ�����
        int64_t             _streamId;

        FILE*               _file;
        char*               _body;
        uint64_t            _bodySize;      // content-length ����ꍇ�̃T�C�Y
        uint64_t            _writeSize;     // ���ݏ������񂾃T�C�Y
        uint64_t            _progress;      // GetProgress �Ŏ擾�����܂łɏ������񂾃T�C�Y�̗݌v
        uint64_t            _responseCode;

        std::vector<std::string>    _resHeaders;

        // for debug
        const char*         _hostName;

        void Clear();
        QwfsStatus ConvertQuicheH3ErrorToStatus(quiche_h3_error error, QwfsStatus nowStatus) const;
    };
}