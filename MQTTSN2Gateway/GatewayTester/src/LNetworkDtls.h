/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi, Ian Craggs
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
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#ifndef NETWORKDTLS_H_
#define NETWORKDTLS_H_

#ifdef DTLS

#include <sys/time.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SOCKET_MAXHOSTNAME  200
#define SOCKET_MAXCONNECTIONS  5
#define SOCKET_MAXRECV  500
#define SOCKET_MAXBUFFER_LENGTH 500 // buffer size


using namespace std;

namespace linuxAsyncClient {
#define STAT_NONE   0
#define STAT_UNICAST   1
#define STAT_MULTICAST 2
#define STAT_SSL 3
/*========================================
 Class LDtlsPort
 =======================================*/
class LDtlsPort
{
    friend class LNetwork;
public:
    LDtlsPort();
    virtual ~LDtlsPort();

	bool open(LUdpConfig* config);

    int unicast(const uint8_t *buf, uint32_t length);
	int multicast( const uint8_t* buf, uint32_t length );
	int recv(uint8_t* buf, uint16_t len, bool nonblock, uint32_t* ipaddress, in_port_t* port );
	int recv(uint8_t* buf, int flags);
	bool checkRecvBuf();
	bool isUnicast();
    SSL* getSSL(void);
    int sslConnect(uint32_t ipAddress, in_port_t port);
private:
	void close();
	int recvfrom ( uint8_t* buf, uint16_t len, int flags, uint32_t* ipaddress, in_port_t* port );

    int _sockfdMcast;
    int _sockfdSsl;
    SSL_CTX *_ctx;
    SSL *_ssl;
    in_port_t _gPortNo;
    in_port_t _uPortNo;
	uint32_t _gIpAddr;
	uint8_t  _castStat;
	bool   _disconReq;

};

#define NO_ERROR	0
#define PACKET_EXCEEDS_LENGTH  1
/*===========================================
               Class  Network
 ============================================*/
class LNetwork: public LDtlsPort
{
public:
    LNetwork();
    ~LNetwork();

    int  broadcast(const uint8_t* payload, uint16_t payloadLen);
    int  unicast(const uint8_t* payload, uint16_t payloadLen);
    void setGwAddress(void);
    void resetGwAddress(void);
    bool initialize(LUdpConfig* config);
    uint8_t*  getMessage(int* len);
    bool isBroadcastable();
    int sslConnect(void);
private:
    void setSleep();
    int  readApiFrame(void);

    uint32_t _gwIpAddress;
	uint32_t _ipAddress;
    in_port_t _gwPortNo;
	in_port_t _portNo;
    int     _returnCode;
    bool _sleepflg;
    uint8_t _rxDataBuf[MQTTSN_MAX_PACKET_SIZE + 1];  // defined in MqttsnClientApp.h

};

}    /* end of namespace */
#endif /* DTLS */
#endif /* NETWORKDTLS_H_ */
