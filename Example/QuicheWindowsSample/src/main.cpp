#include "QuicheWindowsSample.h"

// サーバの指定
#if 0
// 外部サイトを使う場合
const char* HOST = "quic.aiortc.org";
//const char* HOST = "blog.cloudflare.com";
const char* PORT = "443";
#else
const char* HOST = "localhost";
const char* PORT = "4433";
#endif

int main(int argc, char* argv[])
{
    const char* host = HOST;
    const char* port = PORT;
    if (2 == argc)
    {
        host = argv[1];
        port = argv[2];
    }

    QuicheWrapper qw(host,  port);
    
    return qw.Execute();
}
