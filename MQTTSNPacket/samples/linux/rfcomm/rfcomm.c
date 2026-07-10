/*******************************************************************************
 * Copyright (c) 2021 tomoaki@tomy-tech.com
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
 *    Tomoaki Yamaguchi - initial implementation
 *******************************************************************************/
#if defined(WIN32) || defined(__APP__)
#error "Only available on Linux."
#endif


#if !defined(SOCKET_ERROR)
	/** error in socket operation */
	#define SOCKET_ERROR -1
#endif

#define INVALID_SOCKET SOCKET_ERROR
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "rfcomm.h"

static int mysock = INVALID_SOCKET;

int Socket_error(char* aString, int sock)
{
	if (errno != EINTR && errno != EAGAIN && errno != EINPROGRESS && errno != EWOULDBLOCK)
	{
		if (strcmp(aString, "shutdown") != 0 || (errno != ENOTCONN && errno != ECONNRESET))
		{
			int orig_errno = errno;
			char* errmsg = strerror(errno);

			printf("Socket error %d (%s) in %s for socket %d\n", orig_errno, errmsg, aString, sock);
		}
	}
    return -errno;
}


int rfcomm_sendPacketBuffer(unsigned char* buf, int buflen)
{
	int rc = 0;

    if ((rc = write(mysock, buf, buflen)) == SOCKET_ERROR)
    {
		Socket_error("sendto", mysock);
    }
	else
    {
		rc = 0;
    }
	return rc;
}


int rfcomm_getdata(unsigned char* buf, int count)
{
    int rc = recv(mysock, buf, count, 0);
    printf("received %d bytes count %d\n", rc, (int) count);
	return rc;
}

/**
return >=0 for a socket descriptor, <0 for an error code
*/
int rfcomm_open(char* addr, unsigned char channel)
{
    const int reuse = 1;

    mysock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (mysock == INVALID_SOCKET)
		return Socket_error("socket", mysock);

    setsockopt(mysock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_rc addru = { 0 };
    addru.rc_family = AF_BLUETOOTH;
    addru.rc_channel = channel;
    str2ba(addr, &addru.rc_bdaddr);

    // connect to server
    errno = 0;
    if (connect(mysock, (struct sockaddr *) &addru, sizeof(addru)) < 0)
    {
        rfcomm_close();
        return Socket_error("connect", mysock);
    }
	return mysock;
}

int rfcomm_close()
{
    int rc;

	rc = shutdown(mysock, SHUT_WR);
	rc = close(mysock);
    mysock = INVALID_SOCKET;
	return rc;
}
