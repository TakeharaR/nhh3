#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define QWFS_INVALID_SOCKET -1
#define QWFS_SOCKET_ERROR -1
#define QWFS_WOULDBLOCK EAGAIN

namespace qwfs::socketwrapper
{
	typedef int Socket;

	void PlatformInitialize();
	void PlatformFinalize();
	void Close(Socket sock);
	int IoCtlNonblock(Socket sock);
	int GetError();
}
