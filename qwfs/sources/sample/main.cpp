#include <chrono>
#include <thread>

#include "qwfs.h"

// �T�[�o�̎w��
#if 1
// �O���T�C�g���g���ꍇ
const char* HOST = "cloudflare-quic.com";
//const char* HOST = "www.facebook.com";
//const char* HOST = "www.litespeedtech.com";
//const char* HOST = "www.google.com";
//const char* HOST = "www.fastly.com";
const char* PORT = "443";
bool verify = true;
#else
const char* HOST = "127.0.0.1";
const char* PORT = "4433";
bool verify = false;
#endif

static void NormalDebugOutput(const char* output)
{
    fprintf(stderr, "%s\n", output);
}

static int finished = 0;
namespace QwfsTest
{
    void Excute(QwfsId hostId)
    {
        int loopCount = 64;
        finished = 0;

        uint64_t externalId = 0;    // ���g�p
        for (auto ii = 0; ii < loopCount; ++ii)
        {
            // ��������
            qwfsGetRequest(hostId, externalId, nullptr, nullptr, nullptr, 0);
            
            // �t�@�C���ۑ���(�����ۑ����̓t�@�C������ς���)
            //qwfsGetRequest(hostId, externalId, nullptr, "barbara.txt", nullptr, 0);
        }

        auto status = QwfsStatus::Connecting;
        int count = 0;
        do 
        {
            count++;
            qwfsUpdate(hostId);
            qwfsIssueCallbacks(hostId);

#if 0
            // for Abort/Connection Migration test
            if (count == 7)
            {
                qwfsReconnect(hostId);
                //qwfsAbort(hostId);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1024));
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
#endif

            qwfsGetStatus(hostId, &status);
        } while ((status > QwfsStatus::Error)&& (finished != loopCount));
    }
};

int main()
{
    QwfsCallbacks callbacks;
    callbacks._successBinary = [](QwfsId externalId, uint64_t responseCode, const char** headers, uint64_t headersSize, const void* body, uint64_t bodySize)
    {
        ++finished;
    };

    callbacks._successFile = [](QwfsId externalId, uint64_t responseCode, const char** headers, uint64_t headersSize, const char* filePath)
    {
        ++finished;
    };

    callbacks._error = [](QwfsId externalId, QwfsStatus status, const char* errorDetail)
    {
        ++finished;
    };

    qwfsInitialize();
    qwfsSetDebugOutput(NormalDebugOutput);
    auto hostId = qwfsCreate(HOST, PORT, callbacks);
    QwfsOptions options;
    options._qlogPath = "G://tmp";
    options._workPath = "G://tmp";    // 0-RTT �p�̃t�@�C�������Ɏw�肪�K�v
    options._verifyPeer = verify;
    options._quicOprtions._disableActiveMigration = false;
    options._quicOprtions._enableEarlyData = false;
    qwfsSetOptions(hostId, options);
    QwfsTest::Excute(hostId);

#if 0
    // for Abort/Connection Migration test
    qwfsReconnect(hostId);
    QwfsTest::Excute(hostId);
#endif

    qwfsUninitialize();
    qwfsDestroy(hostId);

    // �ď������`�F�b�N
    //qwfsInitialize();
    //QwfsTest::Excute(HOST, PORT, true);
    //qwfsUninitialize();
 }