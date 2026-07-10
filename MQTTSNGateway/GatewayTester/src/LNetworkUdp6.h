/**************************************************************************************
 * Copyright (c) 2021, Tomoaki Yamaguchi
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

#ifndef NETWORKUDP6_H_
#define NETWORKUDP6_H_

#ifdef UDP6

#include <sys/time.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <poll.h>


#define SOCKET_MAXHOSTNAME  200
#define SOCKET_MAXCONNECTIONS  5
#define SOCKET_MAXRECV  500
#define SOCKET_MAXBUFFER_LENGTH 500 // buffer size

#define STAT_UNICAST   1
#define STAT_MULTICAST 2

using namespace std;

namespace linuxAsyncClient {
/*========================================
 Class LUpd6Port
 =======================================*/
class LUdp6Port
{
    friend class LNetwork;
public:
    LUdp6Port();
    virtual ~LUdp6Port();

    bool open(LUdp6Config *config);

    int unicast(const uint8_t *buf, uint32_t length, in6_addr ipaddress, uint16_t port);
	int multicast( const uint8_t* buf, uint32_t length );
    int recv(uint8_t *buf, uint16_t len, bool nonblock, in6_addr *ipaddress, uint16_t *port);
	int recv(uint8_t* buf, int flags);
	bool checkRecvBuf();
	bool isUnicast();

private:
	void close();
    int recvfrom(uint8_t *buf, uint16_t len, int flags, in6_addr *ipaddress, uint16_t *port);

    pollfd _pollfds[2];
	uint16_t _gPortNo;
	uint16_t _uPortNo;
    sockaddr_in6 _gIpAddr;
    char *_gIpAddrStr;
    char* _interface;
    int _sock;
	bool   _disconReq;

};

#define NO_ERROR	0
#define PACKET_EXCEEDS_LENGTH  1
/*===========================================
               Class  Network
 ============================================*/
class LNetwork: public LUdp6Port
{
public:
    LNetwork();
    ~LNetwork();

    int  broadcast(const uint8_t* payload, uint16_t payloadLen);
    int  unicast(const uint8_t* payload, uint16_t payloadLen);
    void setGwAddress(void);
    void resetGwAddress(void);
    bool initialize(LUdp6Config *config);
    uint8_t*  getMessage(int* len);
    bool isBroadcastable();
private:
    void setSleep();
    int  readApiFrame(void);

    in6_addr _gwIpAddress;
    in6_addr _ipAddress;
	uint16_t _gwPortNo;
	uint16_t _portNo;
    int     _returnCode;
    bool _sleepflg;
    uint8_t _rxDataBuf[MQTTSN_MAX_PACKET_SIZE + 1];  // defined in MqttsnClientApp.h

};

}    /* end of namespace */
#endif /* UDP6 */
#endif /* NETWORKUDP_H_ */
