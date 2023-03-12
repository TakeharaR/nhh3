#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#define QWFS_INVALID_SOCKET INVALID_SOCKET
#define QWFS_SOCKET_ERROR SOCKET_ERROR
#define QWFS_WOULDBLOCK WSAEWOULDBLOCK

namespace qwfs::socketwrapper
{
	typedef SOCKET Socket;

	void PlatformInitialize();
	void PlatformFinalize();
	void Close(Socket sock);
	int IoCtlNonblock(Socket sock);
	int GetError();
}

