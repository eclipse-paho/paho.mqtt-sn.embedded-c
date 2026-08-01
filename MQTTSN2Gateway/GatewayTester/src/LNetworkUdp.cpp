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
#ifdef UDP

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

#include "LMqttsnClientApp.h"
#include "LNetworkUdp.h"
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
    return LUdpPort::multicast(xmitData, (uint32_t) dataLen);
}

int LNetwork::unicast(const uint8_t *xmitData, uint16_t dataLen)
{
    return LUdpPort::unicast(xmitData, dataLen, _gwIpAddress, _gwPortNo);
}

uint8_t* LNetwork::getMessage(int *len)
{
    *len = 0;
    if (checkRecvBuf())
    {
        uint16_t recvLen = LUdpPort::recv(_rxDataBuf, MQTTSN_MAX_PACKET_SIZE, false, &_ipAddress, &_portNo);
        if (_gwIpAddress && isUnicast() && (_ipAddress != _gwIpAddress) && (_portNo != _gwPortNo))
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
            //if(recvLen != *len){
            //	*len = 0;
            //	return 0;
            //}else{
            return _rxDataBuf;
            //}
        }
    }
    return 0;
}

void LNetwork::setGwAddress(void)
{
    _gwPortNo = _portNo;
    _gwIpAddress = _ipAddress;
}

void LNetwork::setFixedGwAddress(void)
{
    _gwPortNo = LUdpPort::_gPortNo;
    _gwIpAddress = LUdpPort::_gIpAddr;
}

void LNetwork::resetGwAddress(void)
{
    _gwIpAddress = 0;
    _gwPortNo = 0;
}

bool LNetwork::initialize(LUdpConfig *config)
{
    return LUdpPort::open(config);
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
 Class udpStack
 =========================================*/
LUdpPort::LUdpPort()
{
    _disconReq = false;
    _sockfdUcast = -1;
    _sockfdMcast = -1;
    _castStat = 0;
}

LUdpPort::~LUdpPort()
{
    close();
}

void LUdpPort::close()
{
    if (_sockfdMcast > 0)
    {
        ::close(_sockfdMcast);
        _sockfdMcast = -1;
        if (_sockfdUcast > 0)
        {
            ::close(_sockfdUcast);
            _sockfdUcast = -1;
        }
    }
}

bool LUdpPort::open(LUdpConfig *config)
{
    int optval = 0;

    uint8_t sav = config->ipAddress[3];
    config->ipAddress[3] = config->ipAddress[0];
    config->ipAddress[0] = sav;
    sav = config->ipAddress[2];
    config->ipAddress[2] = config->ipAddress[1];
    config->ipAddress[1] = sav;

    _gPortNo = htons(config->gPortNo);
    _gIpAddr = getUint32((const uint8_t*) config->ipAddress);
    _uPortNo = htons(config->uPortNo);

    if (_gPortNo == 0 || _gIpAddr == 0 || _uPortNo == 0)
    {
        return false;
    }

    _sockfdUcast = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sockfdUcast < 0)
    {
        return false;
    }

    optval = 1;
    setsockopt(_sockfdUcast, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = _uPortNo;
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(_sockfdUcast, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror %s ::bind() to unicast address\033[0m\033[0;37m\n", strerror(errno));
        return false;
    }

    _sockfdMcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_sockfdMcast < 0)
    {
        return false;
    }

    sockaddr_in addrm;
    addrm.sin_family = AF_INET;
    addrm.sin_port = _gPortNo;
    addrm.sin_addr.s_addr = INADDR_ANY;

    optval = 1;
    setsockopt(_sockfdMcast, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (::bind(_sockfdMcast, (struct sockaddr*) &addrm, sizeof(addrm)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror %s ::bind() in UdpPort::open\033[0m\033[0;37m\n", strerror(errno));
        return false;
    }

    ip_mreq mreq;
    mreq.imr_interface.s_addr = INADDR_ANY;
    mreq.imr_multiaddr.s_addr = _gIpAddr;

    if (setsockopt(_sockfdMcast, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror IP_ADD_MEMBERSHIP in UdpPort::open\033[0m\033[0;37m\n");
        close();
        return false;
    }

    optval= 1;
    if (setsockopt(_sockfdMcast, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof(optval)) < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror IP_MULTICAST_LOOP in UdpPPort::open\033[0m\033[0;37m\n");
        close();
        return false;
    }
    return true;
}

bool LUdpPort::isUnicast()
{
    return (_castStat == STAT_UNICAST);
}

int LUdpPort::unicast(const uint8_t *buf, uint32_t length, uint32_t ipAddress, uint16_t port)
{
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = port;
    dest.sin_addr.s_addr = ipAddress;

    int status = ::sendto(_sockfdUcast, buf, length, 0, (const sockaddr*) &dest, sizeof(dest));
    if (status < 0)
    {
        D_NWLOG("errno == %d in UdpPort::unicast\n", errno);
    }
    else
    {
        D_NWLOG("sendto %-15s:%-6u",inet_ntoa(dest.sin_addr),htons(port));
        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        } D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34msendto %-15s:%-6u", inet_ntoa(dest.sin_addr), htons(port));
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
        }
    }
    return status;
}

int LUdpPort::multicast(const uint8_t *buf, uint32_t length)
{
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = _gPortNo;
    dest.sin_addr.s_addr = _gIpAddr;

    int status = ::sendto(_sockfdMcast, buf, length, 0, (const sockaddr*) &dest, sizeof(dest));
    if (status < 0)
    {
        D_NWLOG("\033[0m\033[0;31merrno == %d in UdpPort::multicast\033[0m\033[0;37m\n", errno);
        return errno;
    }
    else
    {
        D_NWLOG("sendto %-15s:%-6u",inet_ntoa(dest.sin_addr),htons(_gPortNo));

        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        } D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34msendto %-15s:%-6u", inet_ntoa(dest.sin_addr), htons(_gPortNo));
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

bool LUdpPort::checkRecvBuf()
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;    // 50 msec

    uint8_t buf[2];
    fd_set recvfds;
    int maxSock = 0;

    FD_ZERO(&recvfds);
    FD_SET(_sockfdUcast, &recvfds);
    FD_SET(_sockfdMcast, &recvfds);

    if (_sockfdMcast > _sockfdUcast)
    {
        maxSock = _sockfdMcast;
    }
    else
    {
        maxSock = _sockfdUcast;
    }

    select(maxSock + 1, &recvfds, 0, 0, &timeout);

    if (FD_ISSET(_sockfdUcast, &recvfds))
    {
        if (::recv(_sockfdUcast, buf, 1, MSG_DONTWAIT | MSG_PEEK) > 0)
        {
            _castStat = STAT_UNICAST;
            return true;
        }
    }
    else if (FD_ISSET(_sockfdMcast, &recvfds))
    {
        if (::recv(_sockfdMcast, buf, 1, MSG_DONTWAIT | MSG_PEEK) > 0)
        {
            _castStat = STAT_MULTICAST;
            return true;
        }
    }
    _castStat = 0;
    return false;
}

int LUdpPort::recv(uint8_t *buf, uint16_t len, bool flg, uint32_t *ipAddressPtr, uint16_t *portPtr)
{
    int flags = flg ? MSG_DONTWAIT : 0;
    return recvfrom(buf, len, flags, ipAddressPtr, portPtr);
}

int LUdpPort::recvfrom(uint8_t *buf, uint16_t length, int flags, uint32_t *ipAddressPtr, uint16_t *portPtr)
{
    struct sockaddr_in sender;
    int status;
    socklen_t addrlen = sizeof(sender);
    memset(&sender, 0, addrlen);

    if (isUnicast())
    {
        D_NWLOG("Ucast ");
        status = ::recvfrom(_sockfdUcast, buf, length, flags, (struct sockaddr*) &sender, &addrlen);
    }
    else if (_castStat == STAT_MULTICAST)
    {
        D_NWLOG("Mcast ");
        status = ::recvfrom(_sockfdMcast, buf, length, flags, (struct sockaddr*) &sender, &addrlen);
    }
    else
    {
        return 0;
    }

    if (status < 0 && errno != EAGAIN)
    {
        D_NWLOG("\033[0m\033[0;31merrno == %d in UdpPort::recvfrom \033[0m\033[0;37m\n", errno);
    }
    else if (status > 0)
    {
        *ipAddressPtr = sender.sin_addr.s_addr;
        *portPtr = sender.sin_port;
        D_NWLOG("recved %-15s:%-6u",inet_ntoa(sender.sin_addr), htons(*portPtr));
        for (uint16_t i = 0; i < status; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        } D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34mrecved %-15s:%-6u", inet_ntoa(sender.sin_addr), htons(*portPtr));
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

#endif

