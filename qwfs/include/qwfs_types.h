#pragma once
#include <stdint.h>

const uint64_t INVALID_QWFS_ID          = UINT64_MAX;
const uint64_t MAX_DATAGRAM_SIZE        = 65527U;
const uint64_t INITIAL_MAX_DATA_SIZE    = 1024U * 1024U;    // とりあえず 1MB

typedef uint64_t QwfsId;

enum class QwfsResult
{
    // コネクションを作り直すか再初期化しないと復旧できないエラー(詳細は GetStatus で取得)
    CriticalError       = -101,

    // 通信が失敗状態になったエラー(再開にはリトライ必須。詳細は GetStatus で取得)
    Error               = -100,

    // API コールのみに影響するエラー(関数再呼び出しで復旧可能性あり)
    ErrorInvalidArg     = -1,
    ErrorInvalidCall    = -2,       // 適切なステータス以外で関数が呼び出された
    ErrorFileIO         = -3,
    ErrorNotExist       = -4,       // 生成していないホストに対して処理しようとした

    // 正常系
    Ok                  = 0,
};

// コネクション・ストリームの状態
enum class QwfsStatus
{
    // コネクションを作り直すか再初期化しないと復旧できないエラー状態
    CriticalError           = -100,
    CriticalErrorQuiche     = -101,

    // 通信が失敗状態になったエラー状態(再開にはリトライ必須)
    Error                   = -1,
    ErrorSocket             = -2,
    ErrorResolveHost        = -3,
    ErrorTimeout            = -4,
    ErrorConnectionClose    = -5,
    ErrorInvalidServer      = -6,     // サーバから受け取った値がおかしい際のエラー(transport 層)
    ErrorInvalidResponse    = -7,     // サーバから受け取った値がおかしい際のエラー(HTTP 層)
    ErrorTlsInvalidCert     = -8,     // TLS Handshake に失敗(不正な証明書)
    ErrorTls                = -9,     // TLS に失敗(証明書以外の理由)
    ErrorNetwork            = -10,
    ErrorVersionFallback    = -11,    // HTTP/3 で接続できず HTTP/1.1 経由で再接続する必要がある
    ErrorFileIO             = -30,

    // 正常な状態
    Wait                    = 0,    // 通信中のリクエストが一つもない場合の状態
    Connecting              = 1,    // 通信中のリクエストが一つでもある場合の状態(コネクション限定のステータス)
    Completed               = 2,    // 通信中のリクエストが一つでもある場合の状態(ストリーム限定のステータス)
    Aborting                = 3,    // アボート中
};

enum class QwfsCcType
{
    NewReno,
    Cubic,
};

enum QWFS_REQUEST_TYPE
{
    QWFS_REQUEST_TYPE_GET,
    //QWFS_REQUEST_TYPE_HEAD,
    //QWFS_REQUEST_TYPE_POST,
    //QWFS_REQUEST_TYPE_PUT,
    //QWFS_REQUEST_TYPE_DELETE,
    //QWFS_REQUEST_TYPE_CONNECT,
    //QWFS_REQUEST_TYPE_OPTIONS,
    //QWFS_REQUEST_TYPE_TRACE,
};

struct QwfsH3Options
{
    uint64_t _maxHeaderListSize;
    uint64_t _qpackMaxTableCapacity;
    uint64_t _qpackBlockedStreams;
    bool     _settingsEnableConnectProtocol;

    QwfsH3Options() : _maxHeaderListSize(1024U), _qpackMaxTableCapacity(1024U), _qpackBlockedStreams(512U), _settingsEnableConnectProtocol(false) {};
};

// BOOL で受けた方がメモリレイアウト的には良いかも
struct QwfsQuicOptions
{
    bool        _disableActiveMigration;
    bool        _enableEarlyData;
    QwfsCcType  _ccType;
    uint64_t    _maxUdpPayloadSize;
    uint64_t    _maxIdleTimeout;
    uint64_t    _initialMaxData;
    uint64_t    _initialMaxStreamDataBidiLocal;
    uint64_t    _initialMaxStreamDataBidiRemote;
    uint64_t    _initialMaxStreamDataUni;
    uint64_t    _initialMaxStreamsBidi;
    uint64_t    _initialMaxStreamsUni;
    uint64_t    _maxConnectionWindowSize;
    uint64_t    _maxStreamWindowSize;
    uint64_t    _activeConnectionIdLimit;

    QwfsQuicOptions() : 
        _disableActiveMigration(false)
        , _enableEarlyData(true)
        , _ccType(QwfsCcType::Cubic)
        , _maxUdpPayloadSize(MAX_DATAGRAM_SIZE)
        , _maxIdleTimeout(10000U)
        , _initialMaxData(INITIAL_MAX_DATA_SIZE)
        , _initialMaxStreamDataBidiLocal(INITIAL_MAX_DATA_SIZE)
        , _initialMaxStreamDataBidiRemote(INITIAL_MAX_DATA_SIZE)
        , _initialMaxStreamDataUni(INITIAL_MAX_DATA_SIZE)
        , _initialMaxStreamsBidi(128U)
        , _initialMaxStreamsUni(128U)
        , _maxConnectionWindowSize(25165824U)       // from quiche /src/args.rs
        , _maxStreamWindowSize(16777216U)           // from quiche /src/args.rs
        , _activeConnectionIdLimit(2U)              // from quiche /src/args.rs
    {};
};

struct QwfsRequestHeaders
{
    const char* name;
    const char* value;

    QwfsRequestHeaders() : name(nullptr), value(nullptr) {};
};

struct QwfsOptions
{
    bool            _verifyPeer;
    const char*     _caCertsList;
    const char*     _qlogPath;
    uint64_t        _maxConcurrentStreams;
    QwfsH3Options   _h3Oprtions;
    QwfsQuicOptions _quicOprtions;

    QwfsOptions() : _verifyPeer(true), _caCertsList(nullptr), _qlogPath(nullptr) {};
};

typedef void(*SuccessFileCallback)(QwfsId hostId, uint64_t responseCode, const char** headers, uint64_t headersSize, const char* filePath);
typedef void(*SuccessBinaryCallback)(QwfsId hostId, uint64_t responseCode, const char** headers, uint64_t headersSize, const void* body, uint64_t bodySize);
typedef void(*ErrorCallback)(QwfsId hostId, QwfsStatus status, const char* errorDetail);
typedef void(*DebugOutputCallback)(const char* message);

struct QwfsCallbacks
{
    SuccessFileCallback     _successFile;
    SuccessBinaryCallback   _successBinary;
    ErrorCallback           _error;

    QwfsCallbacks() : _successFile(nullptr), _successBinary(nullptr), _error(nullptr) {};
};


