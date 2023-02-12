#pragma once
#include <stdint.h>

const uint64_t INVALID_QWFS_ID          = UINT64_MAX;
const uint64_t MAX_DATAGRAM_SIZE        = 65527U;
const uint64_t INITIAL_MAX_DATA_SIZE    = 1024U * 1024U;    // �Ƃ肠���� 1MB

typedef uint64_t QwfsId;

enum class QwfsResult
{
    // �R�l�N�V��������蒼�����ď��������Ȃ��ƕ����ł��Ȃ��G���[(�ڍׂ� GetStatus �Ŏ擾)
    CriticalError       = -101,

    // �ʐM�����s��ԂɂȂ����G���[(�ĊJ�ɂ̓��g���C�K�{�B�ڍׂ� GetStatus �Ŏ擾)
    Error               = -100,

    // API �R�[���݂̂ɉe������G���[(�֐��ČĂяo���ŕ����\������)
    ErrorInvalidArg     = -1,
    ErrorInvalidCall    = -2,       // �K�؂ȃX�e�[�^�X�ȊO�Ŋ֐����Ăяo���ꂽ
    ErrorFileIO         = -3,
    ErrorNotExist       = -4,       // �������Ă��Ȃ��z�X�g�ɑ΂��ď������悤�Ƃ���

    // ����n
    Ok                  = 0,
};

// �R�l�N�V�����E�X�g���[���̏��
enum class QwfsStatus
{
    // �R�l�N�V��������蒼�����ď��������Ȃ��ƕ����ł��Ȃ��G���[���
    CriticalError           = -100,
    CriticalErrorQuiche     = -101,

    // �ʐM�����s��ԂɂȂ����G���[���(�ĊJ�ɂ̓��g���C�K�{)
    Error                   = -1,
    ErrorSocket             = -2,
    ErrorResolveHost        = -3,
    ErrorTimeout            = -4,
    ErrorConnectionClose    = -5,
    ErrorInvalidServer      = -6,     // �T�[�o����󂯎�����l�����������ۂ̃G���[(transport �w)
    ErrorInvalidResponse    = -7,     // �T�[�o����󂯎�����l�����������ۂ̃G���[(HTTP �w)
    ErrorTlsInvalidCert     = -8,     // TLS Handshake �Ɏ��s(�s���ȏؖ���)
    ErrorTls                = -9,     // TLS �Ɏ��s(�ؖ����ȊO�̗��R)
    ErrorNetwork            = -10,
    ErrorVersionFallback    = -11,    // HTTP/3 �Őڑ��ł��� HTTP/1.1 �o�R�ōĐڑ�����K�v������
    ErrorFileIO             = -30,

    // ����ȏ��
    Wait                    = 0,    // �ʐM���̃��N�G�X�g������Ȃ��ꍇ�̏��
    Connecting              = 1,    // �ʐM���̃��N�G�X�g����ł�����ꍇ�̏��(�R�l�N�V��������̃X�e�[�^�X)
    Completed               = 2,    // �ʐM���̃��N�G�X�g����ł�����ꍇ�̏��(�X�g���[������̃X�e�[�^�X)
    Aborting                = 3,    // �A�{�[�g��
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

// BOOL �Ŏ󂯂��������������C�A�E�g�I�ɂ͗ǂ�����
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


