/**************************************************************************************
 * Copyright (c) 2021, 2026 Tomoaki Yamaguchi, Ian Craggs
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
#ifdef UDP6

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <net/if.h>
#include "LMqttsnClientApp.h"
#include "LNetworkUdp6.h"
#include "LTimer.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern uint16_t getUint16(const uint8_t *pos);
extern uint32_t getUint32(const uint8_t *pos);
extern LScreen *theScreen;
extern bool theClientMode;
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
    return LUdp6Port::multicast(xmitData, (uint32_t) dataLen);
}

int LNetwork::unicast(const uint8_t *xmitData, uint16_t dataLen)
{
    return LUdp6Port::unicast(xmitData, dataLen, _gwIpAddress, _gwPortNo);
}

uint8_t* LNetwork::getMessage(int *len)
{
    *len = 0;
    if (checkRecvBuf())
    {
        uint16_t recvLen = LUdp6Port::recv(_rxDataBuf, MQTTSN_MAX_PACKET_SIZE, false, &_ipAddress, &_portNo);
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
    memcpy(_gwIpAddress.s6_addr, _ipAddress.s6_addr, sizeof(_gwIpAddress.s6_addr));
    _gwPortNo = _portNo;
}

void LNetwork::resetGwAddress(void)
{
    memset(_gwIpAddress.s6_addr, 0, sizeof(_gwIpAddress.s6_addr));
    _gwPortNo = 0;
}

bool LNetwork::initialize(LUdp6Config *config)
{
    return LUdp6Port::open(config);
}

void LNetwork::setSleep()
{
    _sleepflg = true;
}

bool LNetwork::isBroadcastable()
{
    return true;
}
/*=========================================
 Class udp6Stack
 =========================================*/
LUdp6Port::LUdp6Port()
{
    _disconReq = false;
    memset(_pollfds, 0, sizeof(_pollfds));
    _sock = 0;
    _interface = NULL;
    _gIpAddrStr = NULL;
}

LUdp6Port::~LUdp6Port()
{
    close();
    if (_gIpAddrStr)
    {
        free(_gIpAddrStr);
    }
    if (_interface)
    {
        free(_interface);
    }
}

void LUdp6Port::close()
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

bool LUdp6Port::open(LUdp6Config *config)
{
    int optval = 1;
    int sock = 0;
    uint32_t ifindex = 0;
    sockaddr_in6 addr6;

    _gPortNo = htons(config->gPortNo);
    _uPortNo = htons(config->uPortNo);

    if (_gPortNo == 0 || _uPortNo == 0)
    {
        return false;
    }

    /* create a unicast socket */
    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return false;
    }

    optval = 1;
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (strlen(config->interface) > 0)
    {
        ifindex = if_nametoindex(config->interface);
#ifdef __APPLE__
        setsockopt(sock, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex));
#else
        setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, config->interface, strlen(config->interface));
#endif
    }

    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = _uPortNo;
    addr6.sin6_addr = in6addr_any;

    if (::bind(sock, (sockaddr*) &addr6, sizeof(addr6)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror %s ::bind() to unicast address\033[0m\033[0;37m\n", strerror(errno));
        return false;
    }
    _pollfds[0].fd = sock;
    _pollfds[0].events = POLLIN;

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
    addrm.ipv6mr_interface = ifindex;
    inet_pton(AF_INET6, config->ipAddress, &addrm.ipv6mr_multiaddr);
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &addrm, sizeof(addrm)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror %s IPV6_ADD_MEMBERSHIP in Udp6Port::open\033[0m\033[0;37m\n", strerror(errno));
        close();
        return false;
    }
    optval = 1;
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

int LUdp6Port::unicast(const uint8_t *buf, uint32_t length, in6_addr ipAddress, uint16_t port)
{
    struct sockaddr_in6 dest;
    dest.sin6_family = AF_INET6;
    dest.sin6_port = port;
    memcpy(dest.sin6_addr.s6_addr, (const void*) ipAddress.s6_addr, sizeof(ipAddress.s6_addr));

    char addrBuf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &dest.sin6_addr, addrBuf, INET6_ADDRSTRLEN);
    D_NWLOG("unicast to [%s]:%-6u", addrBuf, htons(port));

    int status = ::sendto(_pollfds[0].fd, buf, length, 0, (const sockaddr*) &dest, sizeof(dest));
    if (status < 0)
    {
        D_NWLOG(" errno = %d %s  in Udp6Port::unicast\n", errno, strerror(errno));
    }
    else
    {
        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }
        D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34municast to [%s[:%-6u", addrBuf, htons(port));
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

int LUdp6Port::multicast(const uint8_t *buf, uint32_t length)
{
    char sbuf[SCREEN_BUFF_SIZE];
    char portStr[8];
    sprintf(portStr, "%d", ntohs(_gPortNo));

    int status = ::sendto(_pollfds[1].fd, buf, length, 0, (sockaddr*) &_gIpAddr, sizeof(_gIpAddr));
    if (status < 0)
    {
        D_NWLOG("multicast to [%s]:%-6s  ", _gIpAddrStr, portStr);
        D_NWLOG("\033[0m\033[0;31merrno = %d %s in Udp6Port::multicast\033[0m\033[0;37m\n", errno, strerror(errno));
        return errno;
    }
    else
    {
        D_NWLOG("multicast to [%s]:%-6s", _gIpAddrStr, portStr);
        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }
        D_NWLOG("\n");

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

bool LUdp6Port::checkRecvBuf()
{
    uint8_t buf[2];

    int cnt = poll(_pollfds, 2, 50);  // Timeout 50m secs
    if (cnt == 0)
    {
        return false;
    }

    for (int i = 0; i < 2; i++)
    {
        if (_pollfds[i].revents & POLLIN)
        {
            if (::recv(_pollfds[i].fd, buf, 1, MSG_DONTWAIT | MSG_PEEK) > 0)
            {
                _sock = _pollfds[i].fd;
                return true;
            }
        }
    }
    return false;

}

int LUdp6Port::recv(uint8_t *buf, uint16_t len, bool flg, in6_addr *ipAddressPtr, uint16_t *portPtr)
{
    int flags = flg ? MSG_DONTWAIT : 0;
    return recvfrom(buf, len, flags, ipAddressPtr, portPtr);
}

int LUdp6Port::recvfrom(uint8_t *buf, uint16_t length, int flags, in6_addr *ipAddressPtr, uint16_t *portPtr)
{
    struct sockaddr_in6 sender;
    int status;
    socklen_t addrlen = sizeof(sender);
    memset(&sender, 0, addrlen);
    char addrBuf[INET6_ADDRSTRLEN];

    status = ::recvfrom(_sock, buf, length, flags, (struct sockaddr*) &sender, &addrlen);

    if (status < 0 && errno != EAGAIN)
    {
        D_NWLOG("\033[0m\033[0;31merrno == %d in Udp6Port::recvfrom \033[0m\033[0;37m\n", errno);
    }

    if (status > 0)
    {
        inet_ntop(AF_INET6, &sender.sin6_addr, addrBuf, INET6_ADDRSTRLEN);
        memcpy(ipAddressPtr->s6_addr, (const void*) sender.sin6_addr.s6_addr, sizeof(sender.sin6_addr.s6_addr));
        *portPtr = sender.sin6_port;

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34mrecv from [%s]:%-6u", addrBuf, htons(*portPtr));
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

bool LUdp6Port::isUnicast(void)
{
    return (_sock == _pollfds[0].fd && _sock > 0);
}
#endif  /* UDP6 */

