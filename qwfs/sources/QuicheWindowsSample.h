#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

// for quiche build on Windows
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#include "quiche.h"

struct Http3StreamWrapper
{
    quiche_h3_conn* _http3stream;    // 今回はサンプルなので 1 ストリーム
    int64_t         _streamId;
};

class QuicheWrapper
{
public:
    // Functions
    QuicheWrapper(const char* host, const char* port);
    ~QuicheWrapper();
    int Execute();

private:
    // Constant
    static const int MAX_DATAGRAM_SIZE = 1350;

    // Functions
    static void Send(quiche_conn* conn, SOCKET sock);
    static ssize_t Receive(SOCKET sock, quiche_conn* conn, quiche_h3_conn* http3stream, int64_t streamId);
    static Http3StreamWrapper CreateHttpStream(quiche_conn* conn, const char* host);
    static int PollHttpResponse(quiche_conn* conn, quiche_h3_conn* http3stream);
    static SOCKET CreateUdpSocket(const char* host, const char* port);
    static quiche_config* CreateQuicheConfig();
    static quiche_conn* CreateQuicheConnection(const char* host, quiche_config* config);

    // Valiable
    SOCKET _sock;
    quiche_config* _config;
    quiche_conn* _conn;
    quiche_h3_conn* _http3stream;    // 今回はサンプルなので 1 ストリーム
    int64_t         _streamId;
    const char* _host;
};