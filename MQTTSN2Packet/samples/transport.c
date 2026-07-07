/*******************************************************************************
 * Copyright (c) 2014, 2026 IBM Corp., Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Sergio R. Caprile - "commonalization" from prior samples and/or documentation extension
 *    Updated to use getaddrinfo() for hostname resolution in transport_sendPacketBuffer
 *******************************************************************************/

#include <sys/types.h>

#if !defined(SOCKET_ERROR)
	/** error in socket operation */
	#define SOCKET_ERROR -1
#endif

#if defined(WIN32)
/* default on Windows is 64 - increase to make Linux and Windows the same */
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <ws2tcpip.h>
#define MAXHOSTNAMELEN 256
#define EAGAIN WSAEWOULDBLOCK
#define EINTR WSAEINTR
#define EINVAL WSAEINVAL
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTCONN WSAENOTCONN
#define ECONNRESET WSAECONNRESET
#define ioctl ioctlsocket
#define socklen_t int
#else
#define INVALID_SOCKET SOCKET_ERROR
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#endif

#if defined(WIN32)
#include <Iphlpapi.h>
#else
#include <sys/ioctl.h>
#include <net/if.h>
#endif

/**
This simple low-level implementation assumes a single connection for a single thread. Thus, a static
variable is used for that connection.
On other scenarios, the user must solve this by taking into account that the current implementation of
MQTTSNPacket_read() has a function pointer for a function call to get the data to a buffer, but no provisions
to know the caller or other indicator (the socket id): int (*getfn)(unsigned char*, int)
*/
static int mysock = INVALID_SOCKET;

int Socket_error(char* aString, int sock)
{
#if defined(WIN32)
	int errno;
#endif

#if defined(WIN32)
	errno = WSAGetLastError();
#endif
	if (errno != EINTR && errno != EAGAIN && errno != EINPROGRESS && errno != EWOULDBLOCK)
	{
		if (strcmp(aString, "shutdown") != 0 || (errno != ENOTCONN && errno != ECONNRESET))
		{
			int orig_errno = errno;
			char* errmsg = strerror(errno);

			printf("Socket error %d (%s) in %s for socket %d\n", orig_errno, errmsg, aString, sock);
		}
	}
	return errno;
}


int transport_sendPacketBuffer(char* host, int port, unsigned char* buf, int buflen)
{
	struct addrinfo hints;
	struct addrinfo* result;
	struct addrinfo* rp;
	char portstr[6]; /* "65535\0" */
	int rc = SOCKET_ERROR;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;   /* accept IPv4 or IPv6 */
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags    = AI_ADDRCONFIG;

	snprintf(portstr, sizeof(portstr), "%d", port);

	rc = getaddrinfo(host, portstr, &hints, &result);
	if (rc != 0)
	{
		printf("getaddrinfo failed for host %s port %d: %s\n", host, port, gai_strerror(rc));
		return SOCKET_ERROR;
	}

	rc = SOCKET_ERROR;
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		if (sendto(mysock, buf, buflen, 0, rp->ai_addr, (socklen_t)rp->ai_addrlen) != SOCKET_ERROR)
		{
			rc = 0;
			break;
		}
		Socket_error("sendto", mysock);
	}

	freeaddrinfo(result);
	return rc;
}


int transport_getdata(unsigned char* buf, int count)
{
	int rc = recvfrom(mysock, buf, count, 0, NULL, NULL);
	printf("received %d bytes count %d\n", rc, (int)count);
	return rc;
}

/**
return >=0 for a socket descriptor, <0 for an error code
*/
int transport_open()
{
	mysock = socket(AF_INET, SOCK_DGRAM, 0);
	if (mysock == INVALID_SOCKET)
		return Socket_error("socket", mysock);

	return mysock;
}

int transport_close()
{
int rc;

	rc = shutdown(mysock, SHUT_WR);
	rc = close(mysock);

	return rc;
}
