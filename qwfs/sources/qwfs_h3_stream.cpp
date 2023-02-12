#include <cassert>
#include <sstream>
#include <iomanip>

#include "qwfs_h3_stream.h"

const uint64_t DEFAULT_HEADERS_NUM = 5U;

namespace qwfs
{
    Stream::Stream(
        QwfsId externalId
        , const char* saveFilePath
        , const char* hostName
        , const char* path
        , QwfsRequestHeaders* userHeaders
        , uint64_t headersSize
        , const QwfsCallbacks& callbacks
        , DebugOutputCallback debugOutput
    ) :
        _status(QwfsStatus::Connecting)
        , _externalId(externalId)
        , _callbacks(callbacks)
        , _debugOutput(debugOutput)
        , _quicheQuicConnection(nullptr)
        , _quicheH3Connection(nullptr)
        , _streamId(0U)
        , _file(nullptr)
        , _body(nullptr)
        , _bodySize(0U)
        , _writeSize(0U)
        , _progress(0U)
        , _responseCode(0U)
        , _hostName(hostName)
    {
        if (nullptr != saveFilePath)
        {
            _saveFilePath = saveFilePath;
        }

        // ヘッダリストの作成
        if ((nullptr == path) || (0 == strlen(path)))
        {
            path = "/";
        }

        auto headerNum = headersSize + DEFAULT_HEADERS_NUM;
        _reqHeadersForQuiche = new quiche_h3_header[headerNum];
        _reqHeaders.reserve(headerNum);

        // Pseudo - Header を追加
        _reqHeaders.emplace_back(":method", "GET");
        _reqHeaders.emplace_back(":scheme", "https");
        _reqHeaders.emplace_back(":authority", _hostName);
        _reqHeaders.emplace_back(":path", path);
        _reqHeaders.emplace_back("user-agent", "quiche");

        // ユーザ定義ヘッダを追加
        if (nullptr != userHeaders)
        {
            for (auto num = 0; num < headersSize; ++num)
            {
                _reqHeaders.emplace_back(userHeaders[num].name, userHeaders[num].value);
            }
        }

        for (auto num = 0; num < _reqHeaders.size(); ++num)
        {
            _reqHeadersForQuiche[num].name = reinterpret_cast<const uint8_t*>(_reqHeaders[num]._name.c_str());
            _reqHeadersForQuiche[num].name_len = _reqHeaders[num]._name.size();
            _reqHeadersForQuiche[num].value = reinterpret_cast<const uint8_t*>(_reqHeaders[num]._value.c_str());
            _reqHeadersForQuiche[num].value_len = _reqHeaders[num]._value.size();
        }
    }

    Stream::~Stream()
    {
        Clear();
        if (nullptr != _reqHeadersForQuiche)
        {
            delete[] _reqHeadersForQuiche;
        }
    }

    int64_t Stream::Start(quiche_conn* quicheConnection, quiche_h3_conn* quicheH3Connection)
    {
        assert(nullptr != quicheConnection);
        assert(nullptr != quicheH3Connection);

        // ファイル保存準備
        if (!_saveFilePath.empty())
        {
            // todo : 日本語ファイル名対応 and 一時ファイル作成対応
            _file = fopen(_saveFilePath.c_str(), "wb");
            if (nullptr == _file)
            {
                std::stringstream ss;
                ss << "[qwfs error][" << _hostName << "][STREAM:" << _streamId << "]Failed to open file(" << _saveFilePath << ")" << std::endl;
                SetStatus(QwfsStatus::ErrorFileIO, ss.str().c_str());
                return -1;
            }
        }

        _quicheQuicConnection = quicheConnection;
        _quicheH3Connection = quicheH3Connection;
        _streamId = quiche_h3_send_request(_quicheH3Connection, _quicheQuicConnection, _reqHeadersForQuiche, _reqHeaders.size(), true);
        if (0 > _streamId)
        {
            std::stringstream ss;
            ss << "[qwfs error][" << _hostName << "][STREAM:" << _streamId << "]Failed to quiche_h3_send_request(" << _saveFilePath << ")" << std::endl;
            SetStatus(QwfsStatus::CriticalErrorQuiche, ss.str().c_str());
        }
        else if (nullptr != _debugOutput)
        {
            std::stringstream ss;
            ss << "[qwfs info][" << _hostName << "][STREAM:" << _streamId << "]Start request" << std::endl;
            _debugOutput(ss.str().c_str());
        }

        return _streamId;
    }

    int64_t Stream::Retry()
    {
        Clear();
        SetStatus(QwfsStatus::Connecting);
        return Start(_quicheQuicConnection, _quicheH3Connection);
    }

    QwfsStatus Stream::EventHeaders(quiche_h3_event* event)
    {
        // HTTP ヘッダの受信が終わったので処理する

        // quiche_h3_event_for_each_header は事前に個数がわからないのでとりあえず 1 変数に詰めて分解は C# でやる
        // todo : 効率化
        auto getHeader = [](uint8_t* name, size_t name_len, uint8_t* value, size_t value_len, void* argp) -> int
        {
            auto stream = reinterpret_cast<Stream*>(argp);
            std::string headerName(reinterpret_cast<char*>(name), name_len);
            std::string headerValue(reinterpret_cast<char*>(value), value_len);

            if ("content-length" == headerName)
            {
                stream->_bodySize = std::stoull(headerValue);
            }

            if (":status" == headerName)
            {
                stream->_responseCode = std::stoull(headerValue);
            }

            stream->_resHeaders.push_back(std::move(headerName));
            stream->_resHeaders.push_back(std::move(headerValue));

            // todo : 不正ヘッダ確認したい場合はここでチェックして非 0 を返す(中断される)

            return 0;
        };
        quiche_h3_event_for_each_header(event, getHeader, this);

        // Body がない場合も継続して QUICHE_H3_EVENT_FINISHED へ？(要確認)

        return _status;
    }

    QwfsStatus Stream::EventData()
    {
        // HTTP ボディの受信を積み上げる

        // quiche からの受信したバッファを受け取る。一度で受け取れないことがある(上層の quiche_h3_conn_poll でまたここにくる)
        char buf[MAX_DATAGRAM_SIZE] = { 0 };
        auto recvSize = quiche_h3_recv_body(_quicheH3Connection, _quicheQuicConnection, _streamId, reinterpret_cast<uint8_t*>(buf), sizeof(buf));
        if (0 > recvSize)
        {
            std::stringstream ss;
            ss << "[qwfs error][" << _hostName << "][STREAM:" << _streamId << "]Failed to quiche_h3_recv_body(quiche_error is " << recvSize << ")." << std::endl;
            return SetStatus(ConvertQuicheH3ErrorToStatus(static_cast<quiche_h3_error>(recvSize), _status), ss.str().c_str());
        }

        // 書き込み処理
        if (nullptr != _file)
        {
            fwrite(buf, recvSize, 1, _file);
        }
        else
        {
            if (0U == _writeSize)
            {
                if (0U != _bodySize)
                {
                    _body = reinterpret_cast<char*>(malloc(_bodySize));
                }
                else
                {
                    _body = reinterpret_cast<char*>(malloc(recvSize));
                }
            }
            else
            {
                if (0U == _bodySize)
                {
                    _body = reinterpret_cast<char*>(realloc(reinterpret_cast<void*>(_body), _writeSize + recvSize));
                }
            }
            std::memcpy(reinterpret_cast<void*>(&_body[_writeSize]), buf, recvSize);
        }
        _writeSize += recvSize;
        _progress += recvSize;

        return _status;
    }

    void Stream::EventFinished()
    {
        if (nullptr != _debugOutput)
        {
            std::stringstream ss;
            ss << "[qwfs info][" << _hostName << "][STREAM:" << _streamId << "]Complete request" << std::endl;
            _debugOutput(ss.str().c_str());
        }
        SetStatus(QwfsStatus::Completed);
    }

    void Stream::Shutdown(QwfsStatus status, const char* cause)
    {
        // 強制終了
        if (QwfsStatus::Connecting == _status)
        {
            // 個別の失敗コールバックは呼ばない
            // リトライ時に不要なデータはこの時点で解放しておく
            Clear();
            SetStatus(status, cause);
        }
    }

    void Stream::AddProgress(uint64_t* progress, uint64_t* totalWriteSize)
    {
        *progress += _progress;
        *totalWriteSize += _writeSize;
        _progress = 0U;
    }

    void Stream::CallSuccessCallback(QwfsId hostId)
    {
        // todo : 効率化
        std::vector<const char*> headers;
        for (auto& header : _resHeaders)
        {
            headers.push_back(header.c_str());
        }

        if (nullptr != _file)
        {
            if (nullptr != _callbacks._successFile)
            {
                _callbacks._successFile(hostId, _responseCode, headers.data(), _resHeaders.size(), _saveFilePath.c_str());
            }
        }
        else
        {
            if (nullptr != _callbacks._successBinary)
            {
                _callbacks._successBinary(hostId, _responseCode, headers.data(), _resHeaders.size(), _body, _writeSize);
            }
        }
    }

    void Stream::CallErrorCallback(QwfsId hostId)
    {
        if (nullptr != _callbacks._error)
        {
            _callbacks._error(hostId, _status, _errorDetail.c_str());
        }
    }

    QwfsStatus Stream::SetStatus(QwfsStatus status, const char* errorDetail)
    {
        _status = status;
        if (nullptr != errorDetail)
        {
            _errorDetail = errorDetail;
            if (nullptr != _debugOutput)
            {
                _debugOutput(errorDetail);
            }
        }
        else
        {
            _errorDetail.clear();
        }
        return _status;
    }

    void Stream::Clear()
    {
        _streamId = 0U;

        _resHeaders.clear();

        if (nullptr != _file)
        {
            fclose(_file);
            _file = nullptr;
        }
        if (nullptr != _body)
        {
            free(_body);
            _body = nullptr;
        }
        _bodySize = 0U;
        _writeSize = 0U;
        _progress = 0U;
        _responseCode = 0U;
    }

    QwfsStatus Stream::ConvertQuicheH3ErrorToStatus(quiche_h3_error error, QwfsStatus nowStatus) const
    {
        switch (error)
        {
            // Transport 層のエラー
            // 内部的に通信続行で直りそうなもの
            case QUICHE_H3_TRANSPORT_ERR_DONE:
            case QUICHE_H3_TRANSPORT_ERR_FLOW_CONTROL:
            case QUICHE_H3_TRANSPORT_ERR_FINAL_SIZE:
            case QUICHE_H3_TRANSPORT_ERR_CONGESTION_CONTROL:
                return nowStatus;

            // リトライで直りそうなもの
            case QUICHE_H3_TRANSPORT_ERR_STREAM_RESET:
                return QwfsStatus::Error;   // 適当なステータスを返して Connection 側でリトライしてもらう

            // こちらの実装ミスで発生するもの
            case QUICHE_H3_TRANSPORT_ERR_BUFFER_TOO_SHORT:
            case QUICHE_H3_TRANSPORT_ERR_STREAM_LIMIT:
                return QwfsStatus::CriticalError;

            // 証明書の設定で発生する TLS 関連のエラー
            case QUICHE_H3_TRANSPORT_ERR_TLS_FAIL:
                // ソケットクローズからやり直す必要がある
                return QwfsStatus::ErrorTlsInvalidCert;

            // 証明書の設定以外の TLS 関連のエラー
            case QUICHE_H3_TRANSPORT_ERR_CRYPTO_FAIL:
                return QwfsStatus::ErrorTls;

            // サーバからのデータがおかしい等の内部エラーっぽいもの。サーバの問題なので繋ぎ直してもダメ
            case QUICHE_H3_TRANSPORT_ERR_UNKNOWN_VERSION:
            case QUICHE_H3_TRANSPORT_ERR_INVALID_FRAME:
            case QUICHE_H3_TRANSPORT_ERR_INVALID_PACKET:
            case QUICHE_H3_TRANSPORT_ERR_INVALID_STATE:
            case QUICHE_H3_TRANSPORT_ERR_INVALID_STREAM_STATE:
            case QUICHE_H3_TRANSPORT_ERR_INVALID_TRANSPORT_PARAM:
                return QwfsStatus::ErrorInvalidServer;


            // HTTP/3 層のエラー

            // 内部的に通信続行で直りそうなもの
            case QUICHE_H3_ERR_DONE:
            case QUICHE_H3_ERR_EXCESSIVE_LOAD:          // 過負荷による輻輳制御発生時に起きるっぽいのでここにしておく
            case QUICHE_H3_ERR_STREAM_BLOCKED:          // QUIC 内に余裕ができればリトライで直るっぽいのでここにしておく
            case QUICHE_H3_ERR_FRAME_UNEXPECTED:        // サーバ側が間違ったフレームを送ってきたとかその辺の挙動？っぽいのでここにしておく
            case QUICHE_H3_ERR_FRAME_ERROR:
            case QUICHE_H3_ERR_REQUEST_INCOMPLETE:
                return nowStatus;

            // リトライで直りそうなもの
            case QUICHE_H3_ERR_CLOSED_CRITICAL_STREAM:
            case QUICHE_H3_ERR_REQUEST_REJECTED:        // 理由次第ではリトライで復帰しないが基本的にはサーバの状況が改善すれば直る見込み
            case QUICHE_H3_ERR_REQUEST_CANCELLED:       // 同上
            case QUICHE_H3_ERR_CONNECT_ERROR:           // コネクションのリセットもしくはクローズが発生
                return QwfsStatus::Error;   // 適当なステータスを返して Connection 側でリトライしてもらう

            // こちらの実装ミスで発生するもの
            case QUICHE_H3_ERR_BUFFER_TOO_SHORT:
            case QUICHE_H3_ERR_ID_ERROR:            // ストリーム ID が使いまわされたり、上限や下限を超えると発生
            case QUICHE_H3_ERR_SETTINGS_ERROR:      // セッティングフレームの内容が不正な場合に発生
            case QUICHE_H3_ERR_MESSAGE_ERROR:       // HTTP メッセージの不正
                assert(false);
                return QwfsStatus::CriticalError;

            // サーバからのデータがおかしい等の内部エラーっぽいもの。サーバの問題なので繋ぎ直してもダメ
            case QUICHE_H3_ERR_STREAM_CREATION_ERROR:
            case QUICHE_H3_ERR_MISSING_SETTINGS:
            case QUICHE_H3_ERR_QPACK_DECOMPRESSION_FAILED:
                return QwfsStatus::ErrorInvalidResponse;

            // HTTP/3 で接続できず HTTP/1.1 経由で再接続する必要がある
            case QUICHE_H3_ERR_VERSION_FALLBACK:
                return QwfsStatus::ErrorVersionFallback;

            case QUICHE_H3_ERR_INTERNAL_ERROR:
            default:
                return QwfsStatus::CriticalErrorQuiche;
        }
    }
}