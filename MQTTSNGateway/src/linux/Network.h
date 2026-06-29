/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef NETWORK_H_
#define NETWORK_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "Threading.h"
#include "MQTTSNGWDefines.h"

using namespace std;
using namespace MQTTSNGW;

/*========================================
 Class TCPStack
 =======================================*/
class TCPStack
{
public:
	TCPStack();
	virtual ~TCPStack();

	// Server initialization
	bool bind(const char* service);
	bool listen();
	bool accept(TCPStack&);

	// Client initialization
	bool connect(const char* host, const char* service);

	int send(const uint8_t* buf, int length);
	int recv(uint8_t* buf, int len);
	void close();

	void setNonBlocking(const bool);

	bool isValid();
	int getSock();

private:
	int _sockfd;
	addrinfo* _addrinfo;
	Mutex _mutex;
};

/*========================================
 Class Network
 =======================================*/
class Network: public TCPStack
{
public:
	Network();
	virtual ~Network();

	bool connect(const char* host, const char* port, const char* caPath, const char* caFile, const char* cert, const char* prvkey);
	bool connect(const char* host, const char* port);
	void close(void);
	int  send(const uint8_t* buf, uint16_t length);
	int  recv(uint8_t* buf, uint16_t len);

	bool isValid(void);
	bool isSecure(void);
	int  getSock(void);
    void setSecure(bool secureFlg);

private:
	static SSL_CTX* _ctx;
	static SSL_SESSION* _session;
	static int _numOfInstance;
	SSL* _ssl;
	bool _secureFlg;
	Mutex _mutex;
	bool _busy;
	bool _sslValid;
};

#endif /* NETWORK_H_ */
