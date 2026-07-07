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

#ifndef SENSORNETWORK_H_
#define SENSORNETWORK_H_

#include "MQTTSNGWDefines.h"
#include "Threading.h"
#include <netinet/ip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <poll.h>

using namespace std;

namespace MQTTSNGW
{
/*===========================================
 Class  SensorNetAddreess
 ============================================*/
typedef struct
{
    int af;
    union
    {
        struct in_addr ad4;
        struct in6_addr ad6;
    } addr;
} ipAddr_t;

class SensorNetAddress
{
public:
    SensorNetAddress();
    ~SensorNetAddress();
    void setAddress(ipAddr_t *Address, uint16_t port);
    int setAddress(string *ipAddrPort);
    int setIpAddress(string *IpAddress);
    void setFamily(int type);
    int getFamily(void);
    void setPort(in_port_t port);
    void setSockaddr4(sockaddr_in *sockaddr);
    void setSockaddr6(sockaddr_in6 *sockaddr);
    void cpyAddr4(sockaddr_in *sockaddr);
    void cpyAddr6(sockaddr_in6 *sockaddr);
    void cpyAddr(SensorNetAddress *addr);
    in_port_t getPort(void);
    ipAddr_t* getIpAddress(void);
    void setIndex(int index);
    int getIndex(void);

    void clear(void);

    bool isMatch(SensorNetAddress *addr);
    SensorNetAddress& operator =(SensorNetAddress &addr);
    char* sprint(char *buf);
private:
    int _pfdsIndex;
    in_port_t _portNo;
    ipAddr_t _ipAddr;
};

/*===========================================
 Class  Connections
 ============================================*/
#define POLL_UCAST  0
#define POLL_MCAST  1
#define POLL_SSL    2

typedef struct
{
    int af;
    SSL *ssl;
} afSSL_t;

class Connections
{
public:
    Connections();
    ~Connections();
    void initialize(int maxClient);
    void close(int index);
    int poll(int timeout);
    int addClientSock(int sock);
    int addClientSSL(SSL *ssl, int sock);
    void setSockMulticast(int sock);
    void setSockUnicast(int sock);
    int getNumOfConnections(void);
    int getNumOfClients(void);
    SSL* getClientSSL(int index);
    int getEventClient(int index);
    int getSockClient(int index);
    int getSockMulticast(void);
    int getSockUnicast(void);
    int getEventMulticast(void);
    int getEventUnicast(void);
    int getEventListen(void);
    void closeSSL(int index);
    void print(void);
private:
    pollfd *_pollfds;
    SSL **_ssls;
    int _maxfds;
    int _numfds;
    Mutex _mutex;
};

/*===========================================
 Class  SensorNetwork
 ============================================*/
class SensorNetwork
{
    friend class SensorNetSubTask;
public:
    SensorNetwork();
    ~SensorNetwork();

    int unicast(const uint8_t *payload, uint16_t payloadLength, SensorNetAddress *sendto);
    int broadcast(const uint8_t *payload, uint16_t payloadLength);
    int read(uint8_t *buf, uint16_t bufLen);
    void initialize(void);
    const char* getDescription(void);
    SensorNetAddress* getSenderAddress(void);
    Connections* getConnections(void);
    void close();

private:
    int openV4(string *ipAddress, uint16_t multiPortNo, uint16_t uniPortNo, uint32_t ttl);
    int openV6(string *ipAddress, string *interface, uint16_t multiPortNo, uint16_t uniPortNo, uint32_t hops);
    int multicastRecv(uint8_t *buf, uint16_t len);
    int getSendClient(int index, SensorNetAddress *addr);
    int getSenderAddress(int sock, SensorNetAddress *addr);
    int getUnicastClient(SensorNetAddress *addr);
    void clearRecvData(int sock);

    Mutex _mutex;
    SensorNetAddress _senderAddr;
    SensorNetAddress _multicastAddr;
    SensorNetAddress _unicastAddr;
    string _description;
    SSL_CTX *_dtlsctx;
    Connections *_conns;
    sockaddr_in _serverAddr4;
    sockaddr_in6 _serverAddr6;
    int _af;
};

}
#endif /* SENSORNETWORK_H_ */
