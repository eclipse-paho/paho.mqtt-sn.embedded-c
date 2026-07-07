/**************************************************************************************
 * Copyright (c) 2021, 2026 Tomoaki Yamaguchi
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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef NETWORKDTLS6_H_
#define NETWORKDTLS6_H_

#ifdef DTLS6

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
#include <poll.h>

#define SOCKET_MAXHOSTNAME  200
#define SOCKET_MAXCONNECTIONS  5
#define SOCKET_MAXRECV  500
#define SOCKET_MAXBUFFER_LENGTH 500 // buffer size

#define STAT_NONE   0
#define STAT_UNICAST   1
#define STAT_MULTICAST 2
#define STAT_SSL 3

using namespace std;

namespace linuxAsyncClient {
/*========================================
 Class LDtls6Port
 =======================================*/
class LDtls6Port
{
    friend class LNetwork;
public:
    LDtls6Port();
    virtual ~LDtls6Port();

    bool open(LUdp6Config* config);

    int unicast(const uint8_t *buf, uint32_t length);
    int multicast( const uint8_t* buf, uint32_t length );
    int recv(uint8_t* buf, uint16_t len, bool nonblock, in6_addr* ipaddress, in_port_t* port );
    int recv(uint8_t* buf, int flags);
    bool checkRecvBuf();
    bool isUnicast();
    SSL* getSSL(void);
    int sslConnect(in6_addr ipAddress, uint16_t port);
private:
    void close();
    int recvfrom ( uint8_t* buf, uint16_t len, int flags, in6_addr* ipaddress, in_port_t* port );

    int _sockfdMcast;
    int _sockfdSsl;
    SSL_CTX *_ctx;
    SSL *_ssl;
    in_port_t _gPortNo;
    in_port_t _uPortNo;
    sockaddr_in6 _gIpAddr;
    char *_gIpAddrStr;
    uint32_t _ifIndex;
    string _interfaceName;
    uint8_t  _castStat;
    bool   _disconReq;

};

#define NO_ERROR    0
#define PACKET_EXCEEDS_LENGTH  1
/*===========================================
               Class  Network
 ============================================*/
class LNetwork: public LDtls6Port
{
public:
    LNetwork();
    ~LNetwork();

    int  broadcast(const uint8_t* payload, uint16_t payloadLen);
    int  unicast(const uint8_t* payload, uint16_t payloadLen);
    void setGwAddress(void);
    void resetGwAddress(void);
    bool initialize(LUdp6Config* config);
    uint8_t*  getMessage(int* len);
    bool isBroadcastable();
    int sslConnect(void);
private:
    void setSleep();
    int  readApiFrame(void);

    in6_addr _gwIpAddress;
    in6_addr _ipAddress;
    in_port_t _gwPortNo;
    in_port_t _portNo;
    int     _returnCode;
    bool _sleepflg;
    uint8_t _rxDataBuf[MQTTSN_MAX_PACKET_SIZE + 1];  // defined in MqttsnClientApp.h

};

}    /* end of namespace */
#endif /* DTLS6 */
#endif /* NETWORKDTLS6_H_ */
