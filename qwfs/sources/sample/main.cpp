#include <chrono>
#include <thread>

#include "qwfs.h"

static void NormalDebugOutput(const char* output)
{
    fprintf(stderr, "%s\n", output);
}

// ��Ő���
namespace QwfsTest
{
    void Excute(const char* host, const char* port, bool verifyPeer)
    {
        qwfsSetDebugOutput(NormalDebugOutput);

        QwfsCallbacks callbacks;

        callbacks._successBinary = [](QwfsId externalId, uint64_t responseCode, const char** headers, uint64_t headersSize, const void* body, uint64_t bodySize)
        {
            int hoge = 0;
        };

        callbacks._successFile = [](QwfsId externalId, uint64_t responseCode, const char** headers, uint64_t headersSize, const char* filePath)
        {
            int hoge = 0;
        };

        callbacks._error = [](QwfsId externalId, QwfsStatus status, const char* errorDetail)
        {
            int hoge = 0;
        };

        auto hostId = qwfsCreate(host, port, callbacks);

        QwfsOptions options;
        options._qlogPath = "G://tmp";
        options._verifyPeer = verifyPeer;
        qwfsSetOptions(hostId, options);

        // ��������
        uint64_t memoryId = 0;
        uint64_t fileId = 1;
        for (auto ii = 0; ii < 1; ++ii)
        {
            qwfsGetRequest(hostId, memoryId, nullptr, nullptr, nullptr, 0);
            
            // �t�@�C���ۑ���
            //qwfsGetRequest(hostId, fileId, nullptr, "barbara.txt", nullptr, 0);
        }

        auto status = QwfsStatus::Connecting;
        do 
        {
            qwfsUpdate(hostId);
            qwfsIssueCallbacks(hostId);

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            qwfsGetStatus(hostId, &status);
        } while (QwfsStatus::Connecting == status);
        
        qwfsDestroy(hostId);
    }
};


// �T�[�o�̎w��
#if 1
// �O���T�C�g���g���ꍇ
const char* HOST = "cloudflare-quic.com";
const char* PORT = "443";
#else
const char* HOST = "localhost";
const char* PORT = "4433";
#endif

int main()
{
    qwfsInitialize();
    QwfsTest::Excute(HOST, PORT, true);
    qwfsUninitialize();

    // �ď������`�F�b�N
    //qwfsInitialize();
    //QwfsTest::Excute(HOST, PORT, true);
    //qwfsUninitialize();
}