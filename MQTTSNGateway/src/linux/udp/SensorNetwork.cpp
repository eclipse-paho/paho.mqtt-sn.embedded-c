/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi
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
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <regex>
#include <string>
#include <stdlib.h>
#include <poll.h>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"

using namespace std;
using namespace MQTTSNGW;

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
	_IpAddr = 0;
}

SensorNetAddress::~SensorNetAddress()
{

}

uint32_t SensorNetAddress::getIpAddress(void)
{
	return _IpAddr;
}

uint16_t SensorNetAddress::getPortNo(void)
{
	return _portNo;
}

void SensorNetAddress::setAddress(uint32_t IpAddr, uint16_t port)
{
	_IpAddr = IpAddr;
	_portNo = port;
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
 *
 *  This definition is necessary when using TLS connection.
 *  Gateway rejects clients not on the list for security reasons.
 *
 */
int SensorNetAddress::setAddress(string* ip_port)
{
	size_t pos = ip_port->find_first_of(":");

	if ( pos == string::npos )
	{
		_portNo = 0;
		_IpAddr = INADDR_NONE;
		return -1;
	}

	string ip = ip_port->substr(0, pos);
	string port = ip_port->substr(pos + 1);
	int portNo = 0;

	if ((portNo = atoi(port.c_str())) == 0 || (_IpAddr = inet_addr(ip.c_str())) == INADDR_NONE)
	{
		return -1;
	}
	_portNo = htons(portNo);
	return 0;
}

bool SensorNetAddress::isMatch(SensorNetAddress* addr)
{
	return ((this->_portNo == addr->_portNo) && (this->_IpAddr == addr->_IpAddr));
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress& addr)
{
	this->_portNo = addr._portNo;
	this->_IpAddr = addr._IpAddr;
	return *this;
}


char* SensorNetAddress::sprint(char* buf)
{
	struct in_addr  inaddr = { _IpAddr };
	char* ip = inet_ntoa(inaddr);
	sprintf( buf, "%s:", ip);
	sprintf( buf + strlen(buf), "%d", ntohs(_portNo));
	return buf;
}


/*================================================================
   Class  SensorNetwork

   In Gateway version 1.0

   getDescpription( )  is used by Gateway::initialize( )
 initialize( )       is used by Gateway::initialize( )
   getSenderAddress( ) is used by ClientRecvTask::run( )
   broadcast( )        is used by MQTTSNPacket::broadcast( )
   unicast( )          is used by MQTTSNPacket::unicast( )
   read( )             is used by MQTTSNPacket::recv( )

 ================================================================*/

SensorNetwork::SensorNetwork()
{
}

SensorNetwork::~SensorNetwork()
{
}

int SensorNetwork::unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendToAddr)
{
	return UDPPort::unicast(payload, payloadLength, sendToAddr);
}

int SensorNetwork::broadcast(const uint8_t* payload, uint16_t payloadLength)
{
	return UDPPort::broadcast(payload, payloadLength);
}

int SensorNetwork::read(uint8_t* buf, uint16_t bufLen)
{
	return UDPPort::recv(buf, bufLen, &_senderAddr);
}

/**
 *  Prepare UDP sockets and description of SensorNetwork like
 *   "UDP Multicast 225.1.1.1:1883 Gateway Port 10000".
 *   The description is for a start up prompt.
 */
void SensorNetwork::initialize(void)
{
	char param[MQTTSNGW_PARAM_MAX];
	uint16_t multicastPortNo = 0;
	uint16_t unicastPortNo = 0;
	string ip;
	unsigned int ttl = 1;
	/*
	 * theProcess->getParam( ) copies
	 * a text specified by "Key" into param[] from the Gateway.conf
	 *
	 *  in Gateway.conf e.g.
	 *
	 *  # UDP
     *  GatewayPortNo=10000
     *  MulticastIP=225.1.1.1
     *  MulticastPortNo=1883
     *
     */
    if (theProcess->getParam("MulticastIP", param) == 0)
    {
        ip = param;
        _description = "UDP Multicast ";
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
        _description += ", Gateway Port:";
        _description += param;
    }
    if (theProcess->getParam("MulticastTTL", param) == 0)
    {
        ttl = atoi(param);
        _description += ", TTL:";
        _description += param;
    }

    /*  setup UDP sockets */
	errno = 0;
	if ( UDPPort::open(ip.c_str(), multicastPortNo, unicastPortNo, ttl) < 0 )
	{
		throw EXCEPTION("Can't open a UDP", errno);
	}
}

const char* SensorNetwork::getDescription(void)
{
	return _description.c_str();
}

SensorNetAddress* SensorNetwork::getSenderAddress(void)
{
	return &_senderAddr;
}

/*=========================================
 Class udpStack
 =========================================*/

UDPPort::UDPPort()
{
	_disconReq = false;
    memset(_pollFds, 0, sizeof(_pollFds));
}

UDPPort::~UDPPort()
{
	close();
}

void UDPPort::close(void)
{
    for (int i = 0; i < 2; i++)
    {
        if (_pollFds[i].fd > 0)
        {
            ::close(_pollFds[i].fd);
            _pollFds[i].fd = 0;
        }
    }
}

int UDPPort::open(const char *multicastIP, uint16_t multiPortNo, uint16_t uniPortNo, unsigned int ttl)
{
    int optval = 0;
    int sock = 0;

    if (uniPortNo == 0 || multiPortNo == 0)
    {
        D_NWSTACK("error portNo undefined in UDPPort::open\n");
        return -1;
    }

    /*------ Create unicast socket --------*/
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        D_NWSTACK("error can't create unicast socket in UDPPort::open\n");
        return -1;
    }

    sockaddr_in addru;
    addru.sin_family = AF_INET;
    addru.sin_port = htons(uniPortNo);
    addru.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sock, (sockaddr*) &addru, sizeof(addru)) < 0)
	{
		D_NWSTACK("error can't bind unicast socket in UDPPort::open\n");
		return -1;
	}

    _pollFds[0].fd = sock;
    _pollFds[0].events = POLLIN;

    /*------ Create Multicast socket --------*/
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
	{
		D_NWSTACK("error can't create multicast socket in UDPPort::open\n");
		close();
		return -1;
	}

    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in addrm;
    addrm.sin_family = AF_INET;
    addrm.sin_port = htons(multiPortNo);
    addrm.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sock, (sockaddr*) &addrm, sizeof(addrm)) < 0)
    {
        D_NWSTACK("error can't bind multicast socket in UDPPort::open\n");
        return -1;
    }

    ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_interface.s_addr = INADDR_ANY;
    mreq.imr_multiaddr.s_addr = inet_addr(multicastIP);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        D_NWSTACK("error Multicast IP_ADD_MEMBERSHIP in UDPPort::open\n");
        close();
        return -1;
    }

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
    {
        D_NWSTACK("error Multicast IP_MULTICAST_TTL in UDPPort::open\n");
        close();
        return -1;
    }

#ifdef DEBUG_NW
    optval = 1;
#else
    optval = 0;
#endif

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof(optval)) < 0)
    {
        D_NWSTACK("error IP_MULTICAST_LOOP in UDPPort::open\n");
        close();
        return -1;
    }

    _multicastAddr.setAddress(inet_addr(multicastIP), htons(multiPortNo));
    _pollFds[1].fd = sock;
    _pollFds[1].events = POLLIN;

    return 0;
}

int UDPPort::unicast(const uint8_t* buf, uint32_t length, SensorNetAddress* addr)
{
    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = addr->getPortNo();
    dest.sin_addr.s_addr = addr->getIpAddress();

    int status = ::sendto(_pollFds[0].fd, buf, length, 0, (const sockaddr*) &dest, sizeof(dest));
    if (status < 0)
    {
        D_NWSTACK("errno == %d in UDPPort::sendto\n", errno);
    }

    D_NWSTACK("sendto %s:%u length = %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), status);
    return status;
}

int UDPPort::broadcast(const uint8_t* buf, uint32_t length)
{
	return unicast(buf, length, &_multicastAddr);
}

int UDPPort::recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr)
{
    int rc = 0;
    poll(_pollFds, 2, 2000);  // Timeout 2 seconds

    if (_pollFds[0].revents == POLLIN)
    {
        rc = recvfrom(_pollFds[0].fd, buf, len, 0, addr);
    }
    else if (_pollFds[1].revents == POLLIN)
    {
        rc = recvfrom(_pollFds[1].fd, buf, len, 0, addr);
    }
    return rc;
}

int UDPPort::recvfrom(int sockfd, uint8_t* buf, uint16_t len, uint8_t flags, SensorNetAddress* addr)
{
    sockaddr_in sender;
    socklen_t addrlen = sizeof(sender);
    memset(&sender, 0, addrlen);

    int status = ::recvfrom(sockfd, buf, len, flags, (sockaddr*) &sender, &addrlen);

    if (status < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno == %d in UDPPort::recvfrom\n", errno);
        return -1;
    }
    addr->setAddress(sender.sin_addr.s_addr, sender.sin_port);
    D_NWSTACK("recved from %s:%d length = %d\n", inet_ntoa(sender.sin_addr),ntohs(sender.sin_port), status);
    return status;
}

