#include <unistd.h>
#include <cerrno>

#include "qwfs_socket.h"

void qwfs::socketwrapper::PlatformInitialize()
{
}

void qwfs::socketwrapper::PlatformFinalize()
{

}

void qwfs::socketwrapper::Close(Socket sock)
{
    close(sock);
}

int qwfs::socketwrapper::IoCtlNonblock(Socket sock)
{
    return fcntl(sock, F_SETFL, O_NONBLOCK);
}

int qwfs::socketwrapper::GetError()
{
    return errno;
}
