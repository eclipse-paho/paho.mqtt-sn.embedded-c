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
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <string.h>
#include <regex>
#include <string>
#include <stdlib.h>
#include <poll.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <memory>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"
#include "MQTTSNGateway.h"

using namespace std;
using namespace MQTTSNGW;

extern Gateway *theGateway;

#define COOKIE_SECRET_LENGTH 16
int cookie_initialized = 0;
unsigned char cookie_secret[COOKIE_SECRET_LENGTH];

/*===========================================
 Class  SensorNetAddreess

 These 4 methods are minimum requirements for the SensorNetAddress class.
 isMatch(SensorNetAddress* )
 operator =(SensorNetAddress& )
 setAddress(string* )
 sprint(char* )

 UDPPort class requires these 3 methods.
 getIpAddress(void)
 getPortNo(void)
 setAddress(uint32_t IpAddr, uint16_t port)

 ============================================*/
SensorNetAddress::SensorNetAddress()
{
    _portNo = 0;
    memset(&_ipAddr, 0, sizeof(_ipAddr));
}

SensorNetAddress::~SensorNetAddress()
{
}

void SensorNetAddress::setFamily(int type)
{
    _ipAddr.af = type;
}

int SensorNetAddress::getFamily(void)
{
    return _ipAddr.af;
}

ipAddr_t* SensorNetAddress::getIpAddress(void)
{
    return &_ipAddr;
}

in_port_t SensorNetAddress::getPort(void)
{
    return _portNo;
}

void SensorNetAddress::setAddress(ipAddr_t *IpAddr, uint16_t port)
{

    _ipAddr.addr.ad6 = IpAddr->addr.ad6;
    _portNo = htons(port);

    _ipAddr.af = IpAddr->af;
}

void SensorNetAddress::setPort(uint16_t port)
{
    _portNo = htons(port);
}

/**
 *  Set Address data to SensorNetAddress
 *
 *  @param  *ip_port is "IP_Address:PortNo" format string
 *  @return success = 0,  Invalid format = -1
 *
 *  This function is used in ClientList::authorize(const char* fileName)
 *  e.g.
 *  Authorized clients are defined by fileName = "clients.conf"
 *
 *  Client02,172.16.1.7:12002
 *  Client03,172.16.1.8:13003
 *  Client01,172.16.1.6:12001
 *          or
 *  Client01,[xxxx::xxxx]:11001
 *  Client02,[xxxx::xxxx]:12001
 *
 *  This definition is necessary when using TLS/DTLS connection.
 *  Gateway rejects clients are not in the list for security reasons.
 *
 */
int SensorNetAddress::setAddress(string *ipAddrPort)
{
    string port("");
    string ip("");
    size_t pos;
    int portNo = 0;
    _portNo = 0;
    memset(&_ipAddr.addr, 0, sizeof(_ipAddr.addr.ad6));

    if (*ipAddrPort->c_str() == '[')
    {
        // AF_INET6
        pos = ipAddrPort->find_last_of("]:");
        if (pos != string::npos)
        {
            ip = ipAddrPort->substr(1, pos - 2);
            port = ipAddrPort->substr(pos + 1);
        }
    }
    else
    {
        // AF_INET
        pos = ipAddrPort->find_last_of(':');
        if (pos != string::npos)
        {
            ip = ipAddrPort->substr(0, pos);
            port = ipAddrPort->substr(pos + 1);
        }
    }

    if (port == "" || ip == "")
    {
        return -1;
    }

    if (setIpAddress(&ip) == 0)
    {
        if ((portNo = atoi(port.c_str())) != 0)
        {
            _portNo = htons(portNo);
            return 0;
        }
    }
    return -1;
}

int SensorNetAddress::setIpAddress(string *ipAddress)
{
    if (inet_pton(AF_INET, (const char*) ipAddress->c_str(), (void*) &_ipAddr.addr) == 1)
    {
        _ipAddr.af = AF_INET;
    }
    else if (inet_pton(AF_INET6, (const char*) ipAddress->c_str(), (void*) &_ipAddr.addr) == 1)
    {
        _ipAddr.af = AF_INET6;
    }
    else
    {
        _ipAddr.af = 0;
        return -1;
    }
    return 0;
}

bool SensorNetAddress::isMatch(SensorNetAddress *addr)
{
    if (this->_portNo != addr->_portNo || this->_ipAddr.af != addr->_ipAddr.af)
    {
        return false;
    }

    if (this->_ipAddr.af == AF_INET
            && memcmp((const void*) &this->_ipAddr.addr.ad4, (const void*) &addr->_ipAddr.addr.ad4, sizeof(struct in_addr))
                    == 0)
    {
        return true;
    }

    if (this->_ipAddr.af == AF_INET6
            && memcmp((const void*) &this->_ipAddr.addr.ad6, (const void*) &addr->_ipAddr.addr.ad6, sizeof(struct in6_addr))
                    == 0)
    {
        return true;
    }
    return false;
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress &addr)
{
    this->_portNo = addr._portNo;
    memcpy((void*) &this->_ipAddr, (const void*) &addr._ipAddr, sizeof(_ipAddr));
    this->_pfdsIndex = addr._pfdsIndex;
    return *this;
}

void SensorNetAddress::setSockaddr4(sockaddr_in *sockaddr)
{
    _ipAddr.af = sockaddr->sin_family;
    _portNo = sockaddr->sin_port;
    memcpy((void*) &_ipAddr.addr.ad4, (void*) &sockaddr->sin_addr, sizeof(_ipAddr.addr.ad4));
    this->_pfdsIndex = 0;
}

void SensorNetAddress::setSockaddr6(sockaddr_in6 *sockaddr)
{
    _ipAddr.af = sockaddr->sin6_family;
    _portNo = sockaddr->sin6_port;
    memcpy((void*) &_ipAddr.addr.ad6, (void*) &sockaddr->sin6_addr, sizeof(_ipAddr.addr.ad6));
    this->_pfdsIndex = 0;
}

void SensorNetAddress::cpyAddr4(sockaddr_in *sockaddr)
{
    sockaddr->sin_family = _ipAddr.af;
    memcpy((void*) &sockaddr->sin_addr, (void*) &_ipAddr.addr.ad4, sizeof(_ipAddr.addr.ad4));
    sockaddr->sin_port = _portNo;
}

void SensorNetAddress::cpyAddr6(sockaddr_in6 *sockaddr)
{
    sockaddr->sin6_family = _ipAddr.af;
    memcpy((void*) &sockaddr->sin6_addr, (void*) &_ipAddr.addr.ad6, sizeof(_ipAddr.addr.ad6));
    sockaddr->sin6_port = _portNo;
}

char* SensorNetAddress::sprint(char *buf)
{
    char senderstr[INET6_ADDRSTRLEN];
    char *ptr = senderstr;

    if (_ipAddr.af == AF_INET)
    {
        ptr = inet_ntoa(_ipAddr.addr.ad4);
        sprintf(buf, "%s:", ptr);
    }
    else if (_ipAddr.af == AF_INET6)
    {
        inet_ntop(AF_INET6, (const void*) &_ipAddr.addr.ad6, ptr, INET6_ADDRSTRLEN);
        sprintf(buf, "[%s]:", ptr);
    }
    else
    {
        *buf = 0;
        return buf;
    }
    sprintf(buf + strlen(buf), "%d", ntohs(_portNo));
    return buf;
}

void SensorNetAddress::setIndex(int index)
{
    _pfdsIndex = index;
}
int SensorNetAddress::getIndex(void)
{
    return _pfdsIndex;
}

void SensorNetAddress::clear(void)
{
    memset(&_ipAddr, 0, sizeof(_ipAddr));
    _portNo = 0;
}

Connections::Connections()
{
    _pollfds = nullptr;
    _ssls = nullptr;
    _maxfds = 0;
    _numfds = 2;
}

Connections::~Connections()
{
    if (_ssls)
    {
        for (int i = 0; i < _numfds; i++)
        {
            if (_ssls[i] > 0)
            {
                SSL_shutdown(_ssls[i]);
                SSL_free(_ssls[i]);
            }
        }
        free(_ssls);
    }

    if (_pollfds)
    {
        for (int i = 0; i < _numfds; i++)
        {
            if (_pollfds[i].fd > 0)
            {
                ::close(_pollfds[i].fd);
            }
        }
        free(_pollfds);
    }
}

void Connections::initialize(int maxClient)
{
    _maxfds = maxClient + POLL_SSL;
    if ((_pollfds = (pollfd*) calloc(_maxfds, sizeof(pollfd))) == NULL)
    {
        throw EXCEPTION("Can't allocate pollfd.", 0);
    }
    if ((_ssls = (SSL**) calloc(_maxfds, sizeof(SSL*))) == NULL)
    {
        throw EXCEPTION("Can't allocate ssls.", 0);
    }
}

void Connections::closeSSL(int index)
{
    index += POLL_SSL;
    SSL_shutdown(_ssls[index]);
    SSL_free(_ssls[index]);
    _ssls[index] = (SSL*) -1;
}

int Connections::getEventUnicast(void)
{
    return _pollfds[POLL_UCAST].revents;
}

int Connections::getEventMulticast(void)
{
    return _pollfds[POLL_MCAST].revents;
}

int Connections::getEventClient(int index)
{
    return _pollfds[index + POLL_SSL].revents;
}

int Connections::getSockMulticast(void)
{
    return _pollfds[POLL_MCAST].fd;
}

void Connections::setSockMulticast(int sock)
{
    _mutex.lock();
    _pollfds[POLL_MCAST].fd = sock;
    _pollfds[POLL_MCAST].events = POLLIN;
    _mutex.unlock();
}

void Connections::setSockUnicast(int sock)
{
    _mutex.lock();
    _pollfds[POLL_UCAST].fd = sock;
    _pollfds[POLL_UCAST].events = POLLIN;
    _mutex.unlock();
}

int Connections::getSockUnicast(void)
{
    return _pollfds[POLL_UCAST].fd;
}

int Connections::getSockClient(int index)
{
    return _pollfds[index + POLL_SSL].fd;
}

void Connections::close(int index)
{
    int idx = index + POLL_SSL;
    _mutex.lock();
    int sock = _pollfds[idx].fd;
    SSL *ssl = _ssls[idx];

    for (; idx < _numfds; idx++)
    {
        _ssls[idx] = _ssls[idx + 1];
        _pollfds[idx] = _pollfds[idx + 1];

        if (_ssls[idx + 1] == 0)
        {
            break;
        }
    }

    if (ssl > 0)
    {
        _numfds--;
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (sock > 0)
    {
        ::close(sock);
    }
    _mutex.unlock();
}

int Connections::poll(int timeout)
{
    return ::poll(_pollfds, _numfds, timeout);
}

int Connections::addClientSSL(SSL *ssl, int sock)
{
    _mutex.lock();
    _pollfds[_numfds].fd = sock;
    _pollfds[_numfds].events = POLLIN;
    _ssls[_numfds] = ssl;
    int rc = _numfds - POLL_SSL;
    _numfds++;
    _mutex.unlock();
    return rc;
}

int Connections::getNumOfConnections(void)
{
    return _numfds;
}

int Connections::getNumOfClients(void)
{
    return _numfds - POLL_SSL > 0 ? _numfds - POLL_SSL : 0;
}

SSL* Connections::getClientSSL(int index)
{
    return _ssls[index + POLL_SSL];
}

/*================================================================
 Class  SensorNetwork

 getDescpription( )  is used by Gateway::initialize( )
 initialize( )       is used by Gateway::initialize( )
 getSenderAddress( ) is used by ClientRecvTask::run( )
 broadcast( )        is used by MQTTSNPacket::broadcast( )
 unicast( )          is used by MQTTSNPacket::unicast( )
 read( )             is used by MQTTSNPacket::recv( )

 ================================================================*/
#define DTLS_CLIENTHELLO  22
#define DTLS_APPL         23
#define DTLS_OTHERS       100

/* Certificate verification. Returns 1 if trusted, else 0 */
int verify_cert(int ok, X509_STORE_CTX *ctx);

/* Generate cookie. Returns 1 on success, 0 otherwise */
int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len);

/* Verify cookie. Returns 1 on success, 0 otherwise */
int verify_cookie(SSL *ssl, const unsigned char *cookie, unsigned int cookie_len);

SensorNetwork::SensorNetwork()
{
    _senderAddr = new SensorNetAddress();
    _multicastAddr = new SensorNetAddress();
    _unicastAddr = new SensorNetAddress();
    _conns = new Connections();
    _dtlsctx = nullptr;
    _af = 0;
}

SensorNetwork::~SensorNetwork()
{
    if (_conns != nullptr)
    {
        delete _conns;
    }
    if (_senderAddr != nullptr)
    {
        delete _senderAddr;
    }
    if (_multicastAddr != nullptr)
    {
        delete _multicastAddr;
    }
    if (_unicastAddr != nullptr)
    {
        delete _unicastAddr;
    }
}

int SensorNetwork::unicast(const uint8_t *payload, uint16_t payloadLength, SensorNetAddress *sendToAddr)
{
    _mutex.lock();

    SSL *ssl = _conns->getClientSSL(sendToAddr->getIndex());
    int len = SSL_write(ssl, payload, payloadLength);
    int rc = SSL_get_error(ssl, len);
    if (rc < 0)
    {
        D_NWSTACK("error %d in SensorNetwork::unicast\n", rc);
        len = -1;
    }
    _mutex.unlock();
    return len;
}

int SensorNetwork::broadcast(const uint8_t *payload, uint16_t payloadLength)
{
    _mutex.lock();

    int status;
#ifndef DTLS6
    sockaddr_in dest;
    _multicastAddr->cpyAddr4(&dest);

    status = ::sendto(_conns->getSockUnicast(), payload, payloadLength, 0, (const sockaddr*) &dest, sizeof(dest));
    if (status < 0)
    {
        WRITELOG("AF_INET errno = %d in UDP4_6Port::sendto\n", errno);
    }

    D_NWSTACK("sendto %s:%u length = %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), status);

#else
    sockaddr_in6 dest;
    _multicastAddr->cpyAddr6(&dest);

    status = ::sendto(_conns->getSockUnicast(), payload, payloadLength, 0, (const sockaddr*) &dest, sizeof(dest));
    if (status < 0)
    {
        WRITELOG("AF_INET6 errno = %d in SensorNetwork::broadcast\n", errno);
    }

#ifdef DEBUG_NW
    char buff[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &dest.sin6_addr, buff, INET6_ADDRSTRLEN);
    D_NWSTACK("sendto [%s]:%u length = %d\n", buff, ntohs(dest.sin6_port), status);
#endif
#endif
    _mutex.unlock();
    return status;
}

int SensorNetwork::read(uint8_t *buf, uint16_t bufLen)
{
    int optval;
    int clientIndex = -1;
    int sockListen = 0;
    char errmsg[256];
    union
    {
        struct sockaddr_in s4;
        struct sockaddr_in6 s6;
    } client_addr;

    SensorNetAddress client;
    client.clear();
    client.setFamily(_af);

    // Check POLL_IN
    int cnt = _conns->poll(2000);  // Timeout 2secs
    if (cnt == 0)
    {
        // Timeout
        return cnt;
    }
    else if (cnt < 0)
    {
        return -1;
    }


    _mutex.lock();

    //  Check Unicast Port

    if (_conns->getEventUnicast() & POLLIN)
    {
        D_NWSTACK("Connect RECV\n");
        // SSL connection request from a client
        optval = 1;

        client.clear();
        client.setFamily(_af);

        getUnicastClient(&client);
        sockListen = _conns->getSockUnicast();

        // Listen Connection
        SSL *ssl = SSL_new(_dtlsctx);
        BIO *bio = BIO_new_dgram(sockListen, BIO_NOCLOSE);
        SSL_set_bio(ssl, bio, bio);

        SSL_set_options(ssl, SSL_OP_COOKIE_EXCHANGE);

        int rc = 0;

        // SSL Listen
        D_NWSTACK("Listen SSL\n");

        rc = DTLSv1_listen(ssl, (BIO_ADDR*) &client_addr);

        if (rc <= 0)
        {
            ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
            WRITELOG("Listen rc=%d %s\n", rc, errmsg);
            _mutex.unlock();
            return 0;
        }

        // Handle client connection
#ifndef DTLS6
        // DTLS over IPv4
        int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
        setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &optval, sizeof(optval));
        // Bind to Dtls PortNo
        bind(client_fd, (sockaddr*) &_serverAddr4, sizeof(sockaddr_in));
        connect(client_fd, (sockaddr*) &client_addr, sizeof(sockaddr_in));
        client.setSockaddr4((sockaddr_in*) &client_addr.s4);
#else
        // DTLS over IPv6
        int client_fd = socket(AF_INET6, SOCK_DGRAM, 0);
        setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &optval, sizeof(optval));
        // Bind to Dtls PortNo
        bind(client_fd, (sockaddr*) &_serverAddr6, sizeof(sockaddr_in6));
        connect(client_fd, (sockaddr*) &client_addr, sizeof(sockaddr_in6));
        client.setSockaddr6((sockaddr_in6*) &client_addr.s6);
#endif

        BIO *cbio = SSL_get_rbio(ssl);
        BIO_set_fd(cbio, client_fd, BIO_NOCLOSE);
        BIO_ctrl(cbio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &client_addr);

        // Finish handshake
        int ret = SSL_accept(ssl);
        if (ret <= 0)
        {
            ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
            WRITELOG("SSL_accept ret=%d %s\n", ret, errmsg);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            ::close(client_fd);
        }
        else
        {
            // add ssl & socket to Connections instance
            int index = _conns->addClientSSL(ssl, client_fd);

            // save SensorNetworkAddress of Client
            client.setIndex(index);
            _senderAddr = &client;
#ifdef DEBUG_NW
            char clientaddrBuf[128];
            client.sprint(clientaddrBuf);
            D_NWSTACK("Client %s SSL Accepted. idx=%d\n", clientaddrBuf, index);
#endif
        }
        _mutex.unlock();
    }

    // check Multicast
    else if (_conns->getEventMulticast() & POLLIN)
    {
        _mutex.unlock();
        return multicastRecv(buf, bufLen);
    }
    else
    {
        // Check SSL packet from clients
        size_t recvlen = 0;
        SSL *ssl = 0;
        int numfds = _conns->getNumOfConnections();

        for (int i = 0; i < numfds - POLL_SSL; i++)
        {
            if (_conns->getEventClient(i) == POLLIN)
            {
                D_NWSTACK("SSL Packet RECV\n");
                int dtls = getSendClient(i, &client);

                if (dtls > 0)
                {
                    if (dtls == DTLS_CLIENTHELLO)
                    {
                        // Received packet is ClientHello
#ifdef DEBUG_NW
                        char clientaddrBuf[128];
                        client.sprint(clientaddrBuf);
                        D_NWSTACK("Client %s A packet is ClientHello. Client reconnected. Close connection. SSL_connection will timeout.\n", clientaddrBuf);
#endif
                        clientIndex = i;
                        _mutex.unlock();
                        _conns->close(clientIndex);
                        return 0;
                    }

                    // The packet is a MQTT-SN message
                    ssl = _conns->getClientSSL(i);
                    int len = SSL_read_ex(ssl, (void*) buf, (size_t) bufLen, &recvlen);
                    if (SSL_get_error(ssl, len) >= 0)
                    {
#ifdef DEBUG_NW
                        char clientaddrBuf[128];
                        client.sprint(clientaddrBuf);
                        D_NWSTACK("Client %s SSL Accepted. idx=%d\n", clientaddrBuf, i);
#endif
                        _senderAddr = &client;
                        _senderAddr->setIndex(i);
                    }
                    else
                    {
                        D_NWSTACK("SSL RECV Error\n");
                        _conns->close(i);
                        recvlen = -1;
                    }
                    _mutex.unlock();
                    return recvlen;
                }
            }
        }
    }
    return 0;
}

void SensorNetwork::initialize(void)
{
    char param[MQTTSNGW_PARAM_MAX];
    char errmsg[256];
    uint16_t multicastPortNo = 0;
    uint16_t unicastPortNo = 0;

    SensorNetAddress add;
    sockaddr_in6 soadd;
    add.setSockaddr6(&soadd);

#ifndef DTLS6
    string ip;
    uint32_t ttl = 1;

    if (theProcess->getParam("MulticastIP", param) == 0)
    {
        ip = param;
        _description += "IPv4 DTLS Multicast ";
        _description += param;
    }
    if (theProcess->getParam("MulticastPortNo", param) == 0)
    {
        multicastPortNo = atoi(param);
        _description += ":";
        _description += param;
    }
    if (theProcess->getParam("GatewayPortNo", param) == 0)
    {
        unicastPortNo = atoi(param);
        _description += ", Gateway PortNo:";
        _description += param;
    }
    if (theProcess->getParam("MulticastTTL", param) == 0)
    {
        ttl = atoi(param);
        _description += ", TTL:";
        _description += param;
    }
#else
    string ip6;
    uint32_t hops = 1;
    string interface;

    if (theProcess->getParam("MulticastIPv6", param) == 0)
    {
        ip6 = param;
        _description += "IPv6 DTLS Multicast [";
        _description += param;
    }
    if (theProcess->getParam("MulticastIPv6PortNo", param) == 0)
    {
        multicastPortNo = atoi(param);
        _description += "]:";
        _description += param;
    }
    if (theProcess->getParam("GatewayIPv6PortNo", param) == 0)
    {
        unicastPortNo = atoi(param);
        _description += ", Gateway PortNo:";
        _description += param;
    }
    if (theProcess->getParam("MulticastIPv6If", param) == 0)
    {
        interface = param;
        _description += ", Interface:";
        _description += param;
    }
    if (theProcess->getParam("MulticastHops", param) == 0)
    {
        hops = atoi(param);
        _description += ", Hops:";
        _description += param;
    }
#endif

    if (theGateway->getGWParams()->gwCertskey == nullptr)
    {
        throw EXCEPTION("DtlsCertsKey is required.", 0);
    }
    if (theGateway->getGWParams()->gwPrivatekey == nullptr)
    {
        throw EXCEPTION("DtlsPrivateKey is required.", 0);
    }

    /*  allocate Connections */
    _conns->initialize(theGateway->getGWParams()->maxClients);

    SSL_load_error_strings();
    SSL_library_init();

    _dtlsctx = SSL_CTX_new(DTLS_server_method());
    if (_dtlsctx == 0)
    {
        ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
        D_NWSTACK("SSL_CTX_new() %s\n", errmsg);
        throw EXCEPTION("SSL_CTX_new()", 0);
    }
    SSL_CTX_set_min_proto_version(_dtlsctx, DTLS1_VERSION);

    if (SSL_CTX_use_certificate_file(_dtlsctx, theGateway->getGWParams()->gwCertskey, SSL_FILETYPE_PEM) != 1)
    {
        ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
        D_NWSTACK("SSL_CTX_use_certificate_file() %s %s\n", theGateway->getGWParams()->gwCertskey, errmsg);
        throw EXCEPTION("SSL_CTX_use_certificate_file()", 0);
    }
    if (SSL_CTX_use_PrivateKey_file(_dtlsctx, theGateway->getGWParams()->gwPrivatekey, SSL_FILETYPE_PEM) != 1)
    {
        ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
        D_NWSTACK("SSL_CTX_use_PrivateKey_file() %s %s\n", theGateway->getGWParams()->gwPrivatekey, errmsg);
        throw EXCEPTION("SSL_CTX_use_PrivateKey_file()", 0);
    }

    /* Client certification and cookie are not required */
    SSL_CTX_set_verify(_dtlsctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_cookie_generate_cb(_dtlsctx, generate_cookie);
    SSL_CTX_set_cookie_verify_cb(_dtlsctx, verify_cookie);

    /*  Prepare UDP and UDP6 sockets for Multicasting and unicasting */
#ifndef DTLS6
    if (openV4(&ip, multicastPortNo, unicastPortNo, ttl) < 0)
    {
        throw EXCEPTION("Can't open a UDP4", errno);
    }
#else
    if (openV6(&ip6, &interface, multicastPortNo, unicastPortNo, hops) < 0)
    {
        throw EXCEPTION("Can't open a UDP6", errno);
    }
#endif
}

const char* SensorNetwork::getDescription(void)
{
    return _description.c_str();
}

SensorNetAddress* SensorNetwork::getSenderAddress(void)
{
    return _senderAddr;
}

int SensorNetwork::openV4(string *ipAddress, uint16_t multiPortNo, uint16_t uniPortNo, uint32_t ttl)
{
    int optval = 0;
    int rc = -1;
    int sock = 0;
    errno = 0;
    _af = AF_INET;

    if (uniPortNo == 0 || multiPortNo == 0)
    {
        D_NWSTACK("error portNo undefined in UDP4_6Port::openV4\n");
        return rc;
    }

    /*------ Create unicast socket --------*/
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        D_NWSTACK("can't create unicast socket in UDP4_6Port::openV4 error %d %s\n", errno, strerror(errno));
        return -1;
    }

    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    _serverAddr4.sin_family = AF_INET;
    _serverAddr4.sin_port = htons(uniPortNo);
    _serverAddr4.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sock, (sockaddr*) &_serverAddr4, sizeof(_serverAddr4)) < 0)
    {
        D_NWSTACK("can't bind unicast socket in UDP4_6Port::openV4 error %d %s\n", errno, strerror(errno));
        return -1;
    }
    _conns->setSockUnicast(sock);

    /*------ Create Multicast socket --------*/
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        D_NWSTACK("can't create multicast socket in UDP4_6Port::openV4 error %d %s\n", errno, strerror(errno));
        return -1;
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in addrm;
    addrm.sin_family = AF_INET;
    addrm.sin_port = htons(multiPortNo);
    addrm.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sock, (sockaddr*) &addrm, sizeof(addrm)) < 0)
    {
        D_NWSTACK("can't bind multicast socket in UDP4_6Port::openV4 error %d %s\n", errno, strerror(errno));
        return -1;
    }

    ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_interface.s_addr = INADDR_ANY;
    mreq.imr_multiaddr.s_addr = inet_addr(ipAddress->c_str());

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        D_NWSTACK("Multicast IP_ADD_MEMBERSHIP in UDP4_6Port::openV4 error %d %s\n", errno, strerror(errno));
        return -1;
    }
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
    {
        D_NWSTACK("Multicast IP_MULTICAST_TTL in UDP4_6Port::openV4 error %d %s\n", errno, strerror(errno));
        return -1;
    }

#ifdef DEBUG_NW
    optval = 1;
#else
    optval = 0;
#endif

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*) &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("error %d IP_MULTICAST_LOOP in UDP4_6Port::openV4 %s\n", errno, strerror(errno));
        return -1;
    }
    _multicastAddr->setFamily(AF_INET);
    _multicastAddr->setIpAddress(ipAddress);
    _multicastAddr->setPort(multiPortNo);
    _conns->setSockMulticast(sock);
    return 0;
}

int SensorNetwork::openV6(string *ipAddress, string *interface, uint16_t multiPortNo, uint16_t uniPortNo, uint32_t hops)
{
    int optval = 0;
    int sock = 0;
    uint32_t ifindex = 0;

    errno = 0;

    if (uniPortNo == 0 || multiPortNo == 0)
    {
        WRITELOG("error portNo undefined in SensorNetwork::openV6\n");
        return -1;
    }

    _multicastAddr->setPort(multiPortNo);
    _unicastAddr->setPort(uniPortNo);

    if (_multicastAddr->setIpAddress(ipAddress) < 0)
    {
        D_NWSTACK("Incorrect IPV6 address in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    /*------ Create unicast socket --------*/
    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        D_NWSTACK("can't create unicast socket in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }
    _conns->setSockUnicast(sock);

    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    optval = 1;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("IPV6_ONLY in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    memset(&_serverAddr6, 0, sizeof(_serverAddr6));
    _serverAddr6.sin6_family = AF_INET6;
    _serverAddr6.sin6_port = htons(uniPortNo);
    _serverAddr6.sin6_addr = in6addr_any;

    if (::bind(sock, (sockaddr*) &_serverAddr6, sizeof(_serverAddr6)) < 0)
    {
        D_NWSTACK("can't bind unicast socket in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    if (interface->size() > 0)
    {
        ifindex = if_nametoindex(interface->c_str());
#ifdef __APPLE__
        setsockopt(sock, IPPROTO_IP, IP_BOUND_IF, &ifindex, interface->size());
#else
        setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interface->c_str(), interface->size());
#endif
    }

    // Create Multicast socket
    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        D_NWSTACK("can't create multicast socket in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    _conns->setSockMulticast(sock);

    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0)
    {
        D_NWSTACK("IPV6_MULTICAST_IF in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("IPV6_ONLY in SensorNetworkSensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    sockaddr_in6 addrm;
    addrm.sin6_family = AF_INET6;
    addrm.sin6_port = htons(multiPortNo);
    addrm.sin6_addr = in6addr_any;

    if (::bind(sock, (sockaddr*) &addrm, sizeof(addrm)) < 0)
    {
        D_NWSTACK("can't bind multicast socket in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    struct ipv6_mreq mreq;
    mreq.ipv6mr_multiaddr = _multicastAddr->getIpAddress()->addr.ad6;
    mreq.ipv6mr_interface = ifindex;

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        D_NWSTACK("Multicast IPV6_ADD_MEMBERSHIP in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

#ifdef DEBUG_NW
    optval = 1;
#else
    optval = 0;
#endif

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("IPV6_MULTICAST_LOOP in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops)) < 0)
    {
        D_NWSTACK("Multicast IPV6_MULTICAST_HOPS in SensorNetwork::openV6 error %s\n", strerror(errno));
        return -1;
    }
    _multicastAddr->setFamily(AF_INET6);
    _multicastAddr->setIpAddress(ipAddress);
    _multicastAddr->setPort(multiPortNo);
    return 0;
}

int SensorNetwork::multicastRecv(uint8_t *buf, uint16_t len)
{
    int rc = -1;

#ifndef DTLS6
    sockaddr_in sender;
    socklen_t addrlen = sizeof(sender);
    memset(&sender, 0, addrlen);

    rc = ::recvfrom(_conns->getSockMulticast(), buf, len, 0, (sockaddr*) &sender, &addrlen);
    if (rc < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno  %s IPv4 in SensorNetwork::multicastRecv\n", strerror(errno));
        return -1;
    }

    D_NWSTACK("IPv4 multicast recved from %s:%d length = %d\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), rc);

#else
    sockaddr_in6 sender;
    socklen_t addrlen = sizeof(sender);
    memset(&sender, 0, addrlen);

    rc = ::recvfrom(_conns->getSockMulticast(), buf, len, 0, (sockaddr*) &sender, &addrlen);
    if (rc < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno = %d IPv6 in SensorNetwork::multicastRecv\n", errno);
        return -1;
    }
#ifdef DEBUG_NW
    char buff[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sender.sin6_addr, buff, INET6_ADDRSTRLEN);
    D_NWSTACK("IPv6 multicast recved from %s:%u length = %d\n", buff, ntohs(sender.sin6_port), rc);
#endif
#endif

    return rc;
}

int SensorNetwork::getUnicastClient(SensorNetAddress *addr)
{
    return getSenderAddress(_conns->getSockUnicast(), addr);
}

int SensorNetwork::getSendClient(int index, SensorNetAddress *addr)
{
    return getSenderAddress(_conns->getSockClient(index), addr);
}

int SensorNetwork::getSenderAddress(int sock, SensorNetAddress *addr)
{
    int len = -1;

#ifndef DTLS6
    // AF_INET
    sockaddr_in sender4 = { 0 };
    socklen_t addrlen4 = sizeof(sender4);
    char buf[16];
    int rc = DTLS_OTHERS;

    len = ::recvfrom(sock, buf, 15, MSG_PEEK, (sockaddr*) &sender4, &addrlen4);

    if (len < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno = %d in UDPPort::getSender\n", errno);
        return -1;
    }
    addr->setFamily(AF_INET);
    addr->getIpAddress()->addr.ad4 = sender4.sin_addr;
    addr->setPort(sender4.sin_port);
    D_NWSTACK("SensorNetwork::getSenderAddress recved from %s:%d length = %d\n", inet_ntoa(sender4.sin_addr),
            ntohs(sender4.sin_port), len);

    if (len >= 13)
    {
        if (buf[0] == DTLS_CLIENTHELLO || buf[0] == DTLS_APPL)
        {
            rc = buf[0];
        }
    }
    return rc;

#else
    //AF_INET6
    sockaddr_in6 sender6 = { 0 };
    socklen_t addrlen6 = sizeof(sender6);
    unsigned long int buf = 0;
    len = ::recvfrom(sock, &buf, 1, MSG_PEEK, (sockaddr*) &sender6, &addrlen6);

    if (len < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno = %d in SensorNetwork::getSender\n", errno);
        return len;
    }

    addr->setFamily(AF_INET6);
    addr->setSockaddr6(&sender6);
    addr->setPort(sender4.sin_port);

#ifdef DEBUG_NW
    char senderstr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sender6.sin6_addr,senderstr,INET6_ADDRSTRLEN);
    D_NWSTACK("recved from %s:%d length = %d\n",senderstr ,ntohs(sender6.sin6_port), len);
#endif
#endif

    return len;
}

void SensorNetwork::clearRecvData(int sock)
{
    uint8_t buf[MQTTSNGW_MAX_PACKET_SIZE];
    ::recv(sock, buf, MQTTSNGW_MAX_PACKET_SIZE, 0);
}

int verify_cert(int ok, X509_STORE_CTX *ctx)
{
    return 1;
}

int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len)
{
    unsigned char *buf;
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    unsigned int rsltlen;
    union
    {
        struct sockaddr_storage ss;
        struct sockaddr_in6 s6;
        struct sockaddr_in s4;
    } peer;

    if (!cookie_initialized)
    {
        if (!RAND_bytes(cookie_secret, COOKIE_SECRET_LENGTH))
        {
            return 0;
        }
        cookie_initialized = 1;
    }

    (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    len = 0;
    switch (peer.ss.ss_family)
    {
    case AF_INET:
        len += sizeof(struct in_addr);
        break;
    case AF_INET6:
        len += sizeof(struct in6_addr);
        break;
    default:
        OPENSSL_assert(0);
        break;
    }
    len += sizeof(in_port_t);
    buf = (unsigned char*) OPENSSL_malloc(len);

    switch (peer.ss.ss_family)
    {
    case AF_INET:
        memcpy(buf, &peer.s4.sin_port, sizeof(in_port_t));
        memcpy(buf + sizeof(peer.s4.sin_port), &peer.s4.sin_addr, sizeof(struct in_addr));
        break;
    case AF_INET6:
        memcpy(buf, &peer.s6.sin6_port, sizeof(in_port_t));
        memcpy(buf + sizeof(in_port_t), &peer.s6.sin6_addr, sizeof(struct in6_addr));
        break;
    default:
        OPENSSL_assert(0);
        break;
    }

    HMAC(EVP_sha1(), (const void*) cookie_secret, COOKIE_SECRET_LENGTH, (const unsigned char*) buf, len, result, &rsltlen);
    OPENSSL_free(buf);

    memcpy(cookie, result, rsltlen);
    *cookie_len = rsltlen;

    return 1;
}

int verify_cookie(SSL *ssl, const unsigned char *cookie, unsigned int cookie_len)
{
    unsigned char *buf;
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    unsigned int rsltlen;
    union
    {
        struct sockaddr_storage ss;
        struct sockaddr_in6 s6;
        struct sockaddr_in s4;
    } peer;

    if (!cookie_initialized)
    {
        return 0;
    }

    (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    len = 0;
    switch (peer.ss.ss_family)
    {
    case AF_INET:
        len += sizeof(struct in_addr);
        break;
    case AF_INET6:
        len += sizeof(struct in6_addr);
        break;
    default:
        OPENSSL_assert(0);
        break;
    }
    len += sizeof(in_port_t);
    buf = (unsigned char*) OPENSSL_malloc(len);

    switch (peer.ss.ss_family)
    {
    case AF_INET:
        memcpy(buf, &peer.s4.sin_port, sizeof(in_port_t));
        memcpy(buf + sizeof(in_port_t), &peer.s4.sin_addr, sizeof(struct in_addr));
        break;
    case AF_INET6:
        memcpy(buf, &peer.s6.sin6_port, sizeof(in_port_t));
        memcpy(buf + sizeof(in_port_t), &peer.s6.sin6_addr, sizeof(struct in6_addr));
        break;
    default:
        OPENSSL_assert(0);
        break;
    }

    HMAC(EVP_sha1(), (const void*) cookie_secret, COOKIE_SECRET_LENGTH, (const unsigned char*) buf, len, result, &rsltlen);
    OPENSSL_free(buf);

    if (cookie_len == rsltlen && memcmp(result, cookie, rsltlen) == 0)
    {
        return 1;
    }
    return 0;
}

