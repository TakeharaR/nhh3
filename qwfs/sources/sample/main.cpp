#include <chrono>
#include <thread>

#include "qwfs.h"

// サーバの指定
#if 0
// 外部サイトを使う場合
#if 1
const char* HOST = "cloudflare-quic.com";
//const char* HOST = "www.facebook.com";
//const char* HOST = "www.litespeedtech.com";
//const char* HOST = "www.google.com";
//const char* HOST = "www.fastly.com";
const char* path = nullptr;
#else
// 多重化を試したい時
const char* HOST = "blog-cloudflare-com-assets.storage.googleapis.com";
const char* path = "/2019/07/http3-toggle-1.png";
#endif
const char* PORT = "443";
bool verify = true;
#else
const char* HOST = "127.0.0.1";
const char* PORT = "4433";
const char* path = nullptr;
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
        int loopCount = 1;
        finished = 0;

        uint64_t externalId = 0;    // 未使用
        for (auto ii = 0; ii < loopCount; ++ii)
        {
            // メモリ版
            qwfsGetRequest(hostId, externalId, path, nullptr, nullptr, 0);

            // ファイル保存版(複数保存時はファイル名を変える)
            //qwfsGetRequest(hostId, externalId, path, "barbara.txt", nullptr, 0);
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
            std::this_thread::sleep_for(std::chrono::milliseconds(160));
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
    options._workPath = "G://tmp";    // 0-RTT 用のファイル生成に指定が必要
    options._verifyPeer = verify;
    options._quicOprtions._disableActiveMigration = false;
    options._quicOprtions._enableEarlyData = true;
    qwfsSetOptions(hostId, options);
    QwfsTest::Excute(hostId);

#if 0
    // for Abort/Connection Migration test
    // 0-RTT の場合にサーバ側から NEW_CONNECTION_ID フレームを受け取っておきたいので少しまわしておく.
    int count = 0;
    auto status = QwfsStatus::Connecting;
    do
    {
        count++;
        qwfsUpdate(hostId);
        qwfsIssueCallbacks(hostId);
        qwfsGetStatus(hostId, &status);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (count < 30);       // 3 秒待てば NEW_CONNECTION_ID 来るだろうという想定
    qwfsReconnect(hostId);
    QwfsTest::Excute(hostId);
#endif

    qwfsUninitialize();
    qwfsDestroy(hostId);

    // 再初期化チェック
    //qwfsInitialize();
    //QwfsTest::Excute(HOST, PORT, true);
    //qwfsUninitialize();
 }