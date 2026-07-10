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
#ifdef DTLS6

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <net/if.h>
#include <termios.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <poll.h>
#include "LMqttsnClientApp.h"
#include "LNetworkDtls6.h"
#include "LTimer.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern uint16_t getUint16(const uint8_t *pos);
extern uint32_t getUint32(const uint8_t *pos);
extern LScreen *theScreen;
extern bool theClientMode;

/* Certificate verification. Returns 1 if trusted, else 0 */
int verify_cert(int ok, X509_STORE_CTX *ctx);

/*=========================================
 Class LNetwork
 =========================================*/
LNetwork::LNetwork()
{
    _sleepflg = false;
    resetGwAddress();
}

LNetwork::~LNetwork()
{
}

int LNetwork::broadcast(const uint8_t *xmitData, uint16_t dataLen)
{
    return LDtls6Port::multicast(xmitData, (uint32_t) dataLen);
}

int LNetwork::unicast(const uint8_t *xmitData, uint16_t dataLen)
{
    return LDtls6Port::unicast(xmitData, dataLen);
}

uint8_t* LNetwork::getMessage(int *len)
{
    *len = 0;
    if (checkRecvBuf())
    {
        uint16_t recvLen = LDtls6Port::recv(_rxDataBuf, MQTTSN_MAX_PACKET_SIZE, false, &_ipAddress, &_portNo);
        int diffAddr = memcmp(_ipAddress.s6_addr, _gwIpAddress.s6_addr, sizeof(_gwIpAddress.s6_addr));
        if (isUnicast() && diffAddr && (_portNo != _gwPortNo))
        {
            return 0;
        }

        if (recvLen < 0)
        {
            *len = recvLen;
            return 0;
        }
        else
        {
            if (_rxDataBuf[0] == 0x01)
            {
                *len = getUint16(_rxDataBuf + 1);
            }
            else
            {
                *len = _rxDataBuf[0];
            }
            return _rxDataBuf;
        }
    }
    return 0;
}

void LNetwork::setGwAddress(void)
{
    _gwPortNo = _portNo;
    memcpy(&_gwIpAddress.s6_addr, &_ipAddress.s6_addr, sizeof(_ipAddress.s6_addr));

}

void LNetwork::resetGwAddress(void)
{
    memset(&_gwIpAddress, 0, sizeof(_gwIpAddress));
    _gwPortNo = 0;
}

bool LNetwork::initialize(LUdp6Config *config)
{
    return LDtls6Port::open(config);
}

void LNetwork::setSleep()
{
    _sleepflg = true;
}

bool LNetwork::isBroadcastable()
{
    return true;
}

int LNetwork::sslConnect(void)
{
    return LDtls6Port::sslConnect(_gwIpAddress, _gwPortNo);
}

/*=========================================
 Class Dtls6Port
 =========================================*/
LDtls6Port::LDtls6Port()
{
    _disconReq = false;
    memset(_pollfds, 0, sizeof(_pollfds));
    _sock = 0;
    _castStat = 0;
    _ifIndex = 0;
    _gIpAddrStr = NULL;
}

LDtls6Port::~LDtls6Port()
{
    close();
    if (_gIpAddrStr)
    {
        free(_gIpAddrStr);
    }
}

void LDtls6Port::close()
{
    for (int i = 0; i < 2; i++)
    {
        if (_pollfds[i].fd > 0)
        {
            ::close(_pollfds[i].fd);
            _pollfds[i].fd = 0;
        }
    }
}

bool LDtls6Port::open(LUdp6Config *config)
{
    int optval = 1;
    int sock = 0;
    sockaddr_in6 addr6;
    char errmsg[256];

    _gPortNo = htons(config->gPortNo);
    _uPortNo = htons(config->uPortNo);

    if (_gPortNo == 0 || _uPortNo == 0)
    {
        return false;
    }

    SSL_load_error_strings();
    SSL_library_init();
    _ctx = SSL_CTX_new(DTLS_client_method());

    if (_ctx == 0)
    {
        ERR_error_string_n(ERR_get_error(), errmsg, sizeof(errmsg));
        DISPLAY("SSL_CTX_new() %s\n", errmsg);
        return false;
    }

    /* Client certification and cookie are not required */
    SSL_CTX_set_verify(_ctx, SSL_VERIFY_PEER, verify_cert);

    if (strlen(config->interface) > 0)
    {
        _ifIndex = if_nametoindex(config->interface);
        _interfaceName = config->interface;
    }

    /* create a multicast socket */
    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return false;
    }

    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval));

    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = _gPortNo;
    addr6.sin6_addr = in6addr_any;

    if (::bind(sock, (sockaddr*) &addr6, sizeof(addr6)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror %s ::bind() in Udp6Port::open\033[0m\033[0;37m\n", strerror(errno));
        return false;
    }

    ipv6_mreq addrm;
    addrm.ipv6mr_interface = _ifIndex;
    inet_pton(AF_INET6, config->ipAddress, &addrm.ipv6mr_multiaddr);
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &addrm, sizeof(addrm)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror %s IPV6_ADD_MEMBERSHIP in Udp6Port::open\033[0m\033[0;37m\n", strerror(errno));
        close();
        return false;
    }
    optval = 0;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optval, sizeof(optval)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror %s IPV6_MULTICAST_LOOP in Udp6Port::open\033[0m\033[0;37m\n", strerror(errno));
        close();
        return false;
    }
    _pollfds[1].fd = sock;
    _pollfds[1].events = POLLIN;
    _gIpAddr.sin6_family = AF_INET6;
    _gIpAddr.sin6_port = _gPortNo;
    memcpy(&_gIpAddr.sin6_addr, (const void*) &addrm.ipv6mr_multiaddr, sizeof(addrm.ipv6mr_multiaddr));
    _gIpAddrStr = strdup(config->ipAddress);
    return true;
}

bool LDtls6Port::isUnicast()
{
    return (_sock == _pollfds[0].fd && _sock > 0);
}

int LDtls6Port::unicast(const uint8_t *buf, uint32_t length)
{
    int status = SSL_write(_ssl, buf, length);
    if (status <= 0)
    {
        int rc = 0;
        SSL_get_error(_ssl, rc);
        DISPLAY("errno == %d in LDtls6Port::unicast\n", rc);
    }
    else
    {
        D_NWLOG("sendto gateway via DTLS ");
        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34msendto the gateway via SSL  ");
            pos = strlen(sbuf);
            for (uint16_t i = 0; i < length; i++)
            {
                sprintf(sbuf + pos, " %02x", *(buf + i));
                if (strlen(sbuf) > SCREEN_BUFF_SIZE - 20)  // -20 for Escape sequence
                {
                    break;
                }
                pos += 3;
            }
            sprintf(sbuf + strlen(sbuf), "\033[0;37m\n");
            theScreen->display(sbuf);
        }
    }
    return status;
}

int LDtls6Port::multicast(const uint8_t *buf, uint32_t length)
{
    char sbuf[SCREEN_BUFF_SIZE];
    char portStr[8];
    sprintf(portStr, "%d", ntohs(_gPortNo));

    int status = ::sendto(_pollfds[1].fd, buf, length, 0, (sockaddr*) &_gIpAddr, sizeof(_gIpAddr));
    if (status < 0)
    {
        D_NWLOG("multicast to [%s]:%-6s  ", _gIpAddrStr, portStr);D_NWLOG("\033[0m\033[0;31merrno = %d %s in Udp6Port::multicast\033[0m\033[0;37m\n", errno, strerror(errno));
        return errno;
    }
    else
    {
        D_NWLOG("multicast to [%s]:%-6s", _gIpAddrStr, portStr);
        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }D_NWLOG("\n");

        if (!theClientMode)
        {
            memset(sbuf, 0, SCREEN_BUFF_SIZE);
            int pos = 0;
            sprintf(sbuf, "\033[0;34mmulticast to [%s]:%-6s", _gIpAddrStr, portStr);
            pos = strlen(sbuf);
            for (uint16_t i = 0; i < length; i++)
            {
                sprintf(sbuf + pos, " %02x", *(buf + i));
                if (strlen(sbuf) > SCREEN_BUFF_SIZE - 20)
                {
                    break;
                }
                pos += 3;
            }
            sprintf(sbuf + strlen(sbuf), "\033[0;37m\n");
            theScreen->display(sbuf);
        }
        return status;
    }
}

bool LDtls6Port::checkRecvBuf()
{
    uint8_t buf[2];

    int cnt = poll(_pollfds, 2, 2000);  // Timeout 2secs
    if (cnt == 0)
    {
        return false;
    }

    if (_pollfds[0].revents & POLLIN)
    {
        if (::recv(_pollfds[0].fd, buf, 1, MSG_DONTWAIT | MSG_PEEK) > 0)
        {
            _castStat = STAT_SSL;
            _sock = _pollfds[0].fd;
            return true;
        }
    }
    else if (_pollfds[1].revents & POLLIN)
    {
        if (::recv(_pollfds[1].fd, buf, 1, MSG_DONTWAIT | MSG_PEEK) > 0)
        {
            _castStat = STAT_MULTICAST;
            _sock = _pollfds[1].fd;
            return true;
        }
    }
    return false;
}

int LDtls6Port::recv(uint8_t *buf, uint16_t len, bool flg, in6_addr *ipAddressPtr, in_port_t *portPtr)
{
    int flags = flg ? MSG_DONTWAIT : 0;
    return recvfrom(buf, len, flags, ipAddressPtr, portPtr);
}

int LDtls6Port::recvfrom(uint8_t *buf, uint16_t length, int flags, in6_addr *ipAddressPtr, in_port_t *portPtr)
{
    sockaddr_in6 sender;
    int status = 0;
    socklen_t addrlen = sizeof(sender);
    memset(&sender, 0, addrlen);
    char addrBuf[INET6_ADDRSTRLEN];

    if (_castStat == STAT_SSL)
    {
        D_NWLOG("Ucast ");
        if (SSL_read(_ssl, buf, length) == 0)
        {
            return 0;
        }
    }
    else if (_castStat == STAT_MULTICAST)
    {
        D_NWLOG("Mcast ");
        status = ::recvfrom(_sock, buf, length, flags, (sockaddr*) &sender, &addrlen);
    }
    else
    {
        return 0;
    }

    if (status < 0 && errno != EAGAIN)
    {
        D_NWLOG("\033[0m\033[0;31merrno == %d in LDtls6Port::recvfrom \033[0m\033[0;37m\n", errno);
        DISPLAY("\033[0m\033[0;31merrno == %d in LDtls6Port::recvfrom \033[0m\033[0;37m\n", errno);
    }
    else if (status > 0)
    {
        inet_ntop(AF_INET6, &sender.sin6_addr, addrBuf, INET6_ADDRSTRLEN);
        memcpy(ipAddressPtr->s6_addr, (const void*) sender.sin6_addr.s6_addr, sizeof(sender.sin6_addr.s6_addr));
        *portPtr = sender.sin6_port;

        D_NWLOG("recved %-15s:%-6u", addrBuf, htons(*portPtr));

        for (uint16_t i = 0; i < status; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34mrecved %-15s:%-6u", addrBuf, htons(*portPtr));
            pos = strlen(sbuf);
            for (uint16_t i = 0; i < status; i++)
            {
                sprintf(sbuf + pos, " %02x", *(buf + i));
                if (strlen(sbuf) > SCREEN_BUFF_SIZE - 20)
                {
                    break;
                }
                pos += 3;
            }
            sprintf(sbuf + strlen(sbuf), "\033[0;37m\n");
            theScreen->display(sbuf);
        }
        return status;
    }
    else
    {
        return 0;
    }
    return status;
}

int LDtls6Port::sslConnect(in6_addr ipAddress, uint16_t portNo)
{
    int optval = 1;
    int sock = _pollfds[0].fd;

    if (_ssl != 0)
    {
        SSL_free(_ssl);
    }

    if (sock > 0)
    {
        ::close(sock);
    }

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock <= 0)
    {
        return -1;
    }
    optval = 1;
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR || SO_REUSEPORT, &optval, sizeof(optval));

    if (_ifIndex > 0)
    {
#ifdef __APPLE__
        setsockopt(sock, IPPROTO_IP, IP_BOUND_IF, &_ifIndex, sizeof(_ifIndex));
#else
        setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, _interfaceName.c_str(), _interfaceName.size());
#endif
    }

    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = _uPortNo;
    addr.sin6_addr = in6addr_any;

    if (::bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        return -1;
    }
    _pollfds[0].fd = sock;
    _pollfds[0].events = POLLIN;

    uint16_t listenPort = htons(portNo);
    struct sockaddr_in6 dest;
    dest.sin6_family = AF_INET6;
    dest.sin6_port = htons(listenPort);
    memcpy(dest.sin6_addr.s6_addr, (const void*) ipAddress.s6_addr, sizeof(ipAddress.s6_addr));

    int rc = 0;

    BIO *cbio = BIO_new_dgram(sock, BIO_NOCLOSE);
    connect(sock, (sockaddr*) &dest, sizeof(sockaddr_in6));
    BIO_ctrl(cbio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &dest);
    _ssl = SSL_new(_ctx);
    SSL_set_bio(_ssl, cbio, cbio);

#ifdef DEBUG_NW
    char addrBuf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &dest.sin6_addr, addrBuf, INET6_ADDRSTRLEN);
    D_NWLOG("connect to %-15s:%-6u\n", addrBuf, ntohs(dest.sin6_port));
#endif

    errno = 0;
    int stat = SSL_connect(_ssl);
    if (stat != 1)
    {
        rc = -1;
        D_NWLOG("SSL fail to connect %s\n",strerror(errno));
    }
    else
    {
        D_NWLOG("SSL connected\n");
    }
    return rc;
}

int verify_cert(int ok, X509_STORE_CTX *ctx)
{
    return 1;
}

#endif

