#include "qwfs_socket.h"

void qwfs::socketwrapper::PlatformInitialize()
{
    WSADATA    wsaData;
    WSAStartup(MAKEWORD(2, 0), &wsaData);
}

void qwfs::socketwrapper::PlatformFinalize()
{
    WSACleanup();
}

void qwfs::socketwrapper::Close(Socket sock)
{
    closesocket(sock);
}

int qwfs::socketwrapper::IoCtlNonblock(Socket sock)
{
    u_long val = 1;
    return ioctlsocket(sock, FIONBIO, &val);
}

int qwfs::socketwrapper::GetError()
{
    return WSAGetLastError();
}
