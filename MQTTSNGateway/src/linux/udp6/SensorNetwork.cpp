/**************************************************************************************
 * Copyright (c) 2017, Benjamin Aigner
 * Copyright (c) 2021, Tomoaki Yamaguchi
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
 *    Benjamin Aigner - Adaption of the UDPv4 code to use UDPv6
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Tieto Poland Sp. z o.o. - improve portability
 **************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <regex>
#include <string>
#include <stdlib.h>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"

//using namespace std;
using namespace MQTTSNGW;

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
SensorNetAddress::SensorNetAddress()
{
    memset((void*) &_IpAddr, 0, sizeof(_IpAddr));
}

SensorNetAddress::~SensorNetAddress()
{
}

sockaddr_in6* SensorNetAddress::getIpAddress(void)
{
    return &_IpAddr;
}

uint16_t SensorNetAddress::getPortNo(void)
{
    return _IpAddr.sin6_port;
}

void SensorNetAddress::setAddress(struct sockaddr_in6 *IpAddr)
{
    memcpy((void*) &_IpAddr, IpAddr, sizeof(_IpAddr));
}

/**
 *  convert Text data to SensorNetAddress
 *  @param  data is a string [IPV6_Address]:PortNo
 *  @return success = 0,  Invalid format = -1
 */
int SensorNetAddress::setAddress(string* data)
{
    size_t pos = data->find_last_of("]:");

    if (pos != string::npos)
    {
        int portNo = 0;
        string port = data->substr(pos + 1);

        if ((portNo = atoi(port.c_str())) > 0)
        {
            _IpAddr.sin6_port = htons(portNo);
            _IpAddr.sin6_family = AF_INET6;
            string ip = data->substr(1, pos - 2);
            const char *cstr = ip.c_str();

            if (inet_pton(AF_INET6, cstr, &(_IpAddr.sin6_addr)) == 1)
            {
                return 0;
            }
        }
    }
    memset((void*) &_IpAddr, 0, sizeof(_IpAddr));
    return -1;
}

/**
 *  convert Text data to SensorNetAddress
 *  @param  data is pointer of IP_Address format text
 *  @return success = 0,  Invalid format = -1
 */
int SensorNetAddress::setAddress(const char* data)
{
    if (inet_pton(AF_INET6, data, &(_IpAddr.sin6_addr)) == 1)
    {
        _IpAddr.sin6_family = AF_INET6;
        return 0;
    }
    else
    {
        return -1;
    }
}

char* SensorNetAddress::getAddress(void)
{
    inet_ntop(AF_INET6, &(_IpAddr.sin6_addr), _addrString, INET6_ADDRSTRLEN);
    return _addrString;
}

bool SensorNetAddress::isMatch(SensorNetAddress* addr)
{
    return (this->_IpAddr.sin6_port == addr->_IpAddr.sin6_port)
            && (memcmp(this->_IpAddr.sin6_addr.s6_addr, addr->_IpAddr.sin6_addr.s6_addr,
                    sizeof(this->_IpAddr.sin6_addr.s6_addr)) == 0);
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress& addr)
{
    memcpy(&this->_IpAddr, &addr._IpAddr, sizeof(this->_IpAddr));
    return *this;
}


char* SensorNetAddress::sprint(char* buf)
{
    sprintf(buf, "[%s]:", getAddress());
    sprintf(buf + strlen(buf), "%d", ntohs(_IpAddr.sin6_port));
    return buf;
}

/*===========================================
 Class  SensorNetwork
 ============================================*/
SensorNetwork::SensorNetwork()
{
}

SensorNetwork::~SensorNetwork()
{
}

int SensorNetwork::unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendToAddr)
{
    return UDPPort6::unicast(payload, payloadLength, sendToAddr);
}

int SensorNetwork::broadcast(const uint8_t* payload, uint16_t payloadLength)
{
    return UDPPort6::broadcast(payload, payloadLength);
}

int SensorNetwork::read(uint8_t* buf, uint16_t bufLen)
{
    return UDPPort6::recv(buf, bufLen, &_clientAddr);
}

void SensorNetwork::initialize(void)
{
    char param[MQTTSNGW_PARAM_MAX];
    uint16_t unicastPortNo = 0;
    uint16_t multicastPortNo = 0;
    string ip;
    string multicast;
    string interface;
    uint32_t hops = 1;

    if (theProcess->getParam("MulticastIPv6", param) == 0)
    {
        multicast = param;
        _description += "Multicast Address: [";
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
        _description += ", Gateway Port:";
        _description += param;
    }
    if (theProcess->getParam("MulticastIPv6If", param) == 0)
    {
        interface = param;
        _description += ", Interface: ";
        _description += param;
    }
    if (theProcess->getParam("MulticastHops", param) == 0)
    {
        hops = atoi(param);
        _description += ", Hops:";
        _description += param;
    }

    if (UDPPort6::open(unicastPortNo, multicastPortNo, multicast.c_str(), interface.c_str(), hops) < 0)
    {
        throw EXCEPTION("Can't open a UDP6", errno);
    }
}

const char* SensorNetwork::getDescription(void)
{
    return _description.c_str();
}

SensorNetAddress* SensorNetwork::getSenderAddress(void)
{
    return &_clientAddr;
}

/*=========================================
 Class udpStack
 =========================================*/

UDPPort6::UDPPort6()
{
    _disconReq = false;
    _hops = 0;
}

UDPPort6::~UDPPort6()
{
    close();
}

void UDPPort6::close(void)
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

int UDPPort6::open(uint16_t uniPortNo, uint16_t multiPortNo, const char *multicastAddr, const char *interfaceName,
        uint32_t hops)
{
    int optval = 0;
    int sock = 0;
    sockaddr_in6 addr6;
    uint32_t ifindex = 0;

    errno = 0;

    if (uniPortNo == 0 || multiPortNo == 0)
    {
        D_NWSTACK("error portNo undefined in UDPPort6::open\n");
        return -1;
    }

    // Create a unicast socket
    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        D_NWSTACK("UDP6::open - unicast socket: %s", strerror(errno));
        return -1;
    }

    _pollfds[0].fd = sock;
    _pollfds[0].events = POLLIN;

    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &optval, sizeof(optval));

    optval = 1;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("\033[0m\033[0;31m unicast socket error %s IPV6_V6ONLY\033[0m\033[0;37m\n", strerror(errno));
        close();
        return -1;
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)) < 0)
    {
        D_NWSTACK("\033[0m\033[0;31m error %s IPV6_UNICAST_HOPS\033[0m\033[0;37m\n", strerror(errno));
        close();
        return -1;
    }

    if (strlen(interfaceName) > 0)
    {
        ifindex = if_nametoindex(interfaceName);
#ifdef __APPLE__
        setsockopt(sock, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex));
#else
        setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interfaceName, strlen(interfaceName));
#endif
    }

    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(uniPortNo);
    addr6.sin6_addr = in6addr_any;

    if (::bind(sock, (sockaddr*) &addr6, sizeof(addr6)) < 0)
    {
        D_NWSTACK("error can't bind unicast socket in UDPPort6::open: %s\n", strerror(errno));
        close();
        return -1;
    }


    // create a MULTICAST socket

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        D_NWSTACK("UDP6::open - multicast: %s", strerror(errno));
        close();
        return -1;
    }
    _pollfds[1].fd = sock;
    _pollfds[1].events = POLLIN;

    optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("\033[0m\033[0;31m multicast socket error %s SO_REUSEADDR\033[0m\033[0;37m\n", strerror(errno));
        close();
        return -1;
    }
    optval = 1;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("\033[0m\033[0;31m multicast socket error %s IPV6_V6ONLY\033[0m\033[0;37m\n", strerror(errno));
        close();
        return -1;
    }

    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(multiPortNo);
    addr6.sin6_addr = in6addr_any;

    if (::bind(sock, (sockaddr*) &addr6, sizeof(addr6)) < 0)
    {
        close();
        D_NWSTACK("error can't bind multicast socket in UDPPort6::open: %s\n", strerror(errno));
        return -1;
    }

    ipv6_mreq addrm;
    addrm.ipv6mr_interface = ifindex;
    inet_pton(AF_INET6, multicastAddr, &addrm.ipv6mr_multiaddr);
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &addrm, sizeof(addrm)) < 0)
    {
        D_NWSTACK("\033[0m\033[0;31m error %d IPV6_JOIN_GROUP in Udp6Port::open\033[0m\033[0;37m\n", errno);
        close();
        return false;
    }

#ifdef DEBUG_NW
    optval = 1;
#else
    optval = 0;
#endif

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char*) &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("\033[0m\033[0;31m error %s IPV6_MULTICAST_LOOP\033[0m\033[0;37m\n", strerror(errno));
        close();
        return false;
    }
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops)) < 0)
    {
        D_NWSTACK("\033[0m\033[0;31m error %s IPV6_MULTICAST_HOPS\033[0m\033[0;37m\n", strerror(errno));
        close();
        return -1;
    }

    memcpy(&addr6.sin6_addr, &addrm.ipv6mr_multiaddr, sizeof(addrm.ipv6mr_multiaddr));
    _grpAddr.setAddress(&addr6);
    return 0;
}

int UDPPort6::unicast(const uint8_t* buf, uint32_t length, SensorNetAddress* addr)
{
    sockaddr_in6 dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    dest.sin6_port = addr->getPortNo();
    memcpy(dest.sin6_addr.s6_addr, (const void*) &addr->getIpAddress()->sin6_addr, sizeof(in6_addr));

#ifdef  DEBUG_NW
    char addrBuf[INET6_ADDRSTRLEN];
    addr->sprint(addrBuf);
    D_NWSTACK("sendto %s\n", addrBuf);
#endif

    int status = ::sendto(_pollfds[0].fd, buf, length, 0, (const sockaddr*) &dest, sizeof(dest));

    if (status < 0)
    {
        D_NWSTACK("%s in UDPPor6t::sendto\n", strerror(errno));
    }
    return status;
}

int UDPPort6::broadcast(const uint8_t* buf, uint32_t length)
{
    sockaddr_in6 dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    dest.sin6_port = _grpAddr.getPortNo();
    memcpy(dest.sin6_addr.s6_addr, (const void*) &_grpAddr.getIpAddress()->sin6_addr, sizeof(in6_addr));

#ifdef  DEBUG_NW
    char addrBuf[INET6_ADDRSTRLEN];
    addr->sprint(addrBuf);
    D_NWSTACK("sendto %s\n", addrBuf);
#endif

    int status = ::sendto(_pollfds[1].fd, buf, length, 0, (const sockaddr*) &dest, sizeof(dest));

    if (status < 0)
    {
        D_NWSTACK("UDP6::broadcast - sendto: %s", strerror(errno));
        return status;
    }

    return 0;
}

int UDPPort6::recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr)
{
    int rc = poll(_pollfds, 2, 2000);  // Timeout 2secs
    if (rc == 0)
    {
        return rc;
    }

    for (int i = 0; i < 2; i++)
    {
        if (_pollfds[i].revents & POLLIN)
        {
            return recvfrom(_pollfds[i].fd, buf, len, 0, addr);
        }
    }
    return 0;
}

int UDPPort6::recvfrom(int sockfd, uint8_t* buf, uint16_t len, uint8_t flags, SensorNetAddress* addr)
{
    sockaddr_in6 sender;
    socklen_t addrlen = sizeof(sender);
    memset(&sender, 0, addrlen);

    int status = ::recvfrom(sockfd, buf, len, flags, (sockaddr*) &sender, &addrlen);

    if (status < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno in UDPPort6::recvfrom: %s\n", strerror(errno));
        return -1;
    }
    addr->setAddress(&sender);

#ifdef DEBUG_NW
    char addrBuf[INET6_ADDRSTRLEN];
    addr->sprint(addrBuf);
    D_NWSTACK("sendto %s length = %d\n", addrBuf, status);
#endif
    return status;
}
