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
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <string.h>
#include <regex>
#include <string>
#include <stdlib.h>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"

using namespace std;
using namespace MQTTSNGW;

/*===========================================
 * Class  SensorNetAddreess

 * These 4 methods are minimum requirements for the SensorNetAddress class.
 * isMatch(SensorNetAddress* )
 * operator =(SensorNetAddress& )
 * setAddress(string* )
 * sprint(char* )

 * BlePort class requires these 3 methods.
 * getIpAddress(void)
 * getPortNo(void)
 * setAddress(uint32_t BtAddr, uint16_t channel)

 ============================================*/
bdaddr_t NullAddr = { 0, 0, 0, 0, 0, 0 };

SensorNetAddress::SensorNetAddress()
{
    _channel = 0;
    bacpy(&_bdAddr, &NullAddr);
}

SensorNetAddress::~SensorNetAddress()
{

}

bdaddr_t* SensorNetAddress::getAddress(void)
{
    return &_bdAddr;
}

uint16_t SensorNetAddress::getPortNo(void)
{
    return _channel;
}

void SensorNetAddress::setAddress(bdaddr_t BdAddr, uint16_t channel)
{
    bacpy(&_bdAddr, &BdAddr);
    _channel = channel;
}

/**
 *  Set Address data to SensorNetAddress
 *
 *  @param  *dev_channel is "Device_Address.Channel" format string
 *  @return success = 0,  Invalid format = -1
 *
 *  Valid channels are 1 to 30.
 *
 *  Client01,XX:XX:XX:XX:XX:XX.1
 *
 */
int SensorNetAddress::setAddress(string* dev_channel)
{
    int rc = -1;
    size_t pos = dev_channel->find_first_of(".");

    if (pos == string::npos)
    {
        _channel = 0;
        memset(&_bdAddr, 0, sizeof(bdaddr_t));
        return rc;
    }

    string dvAddr = dev_channel->substr(0, pos);
    string strchannel = dev_channel->substr(pos + 1);
    if (strchannel == "*")
    {
        _channel = 0;
    }
    else
    {
        _channel = atoi(strchannel.c_str());
    }
    str2ba(dvAddr.c_str(), &_bdAddr);

    if ((_channel < 0 && _channel > 30) || bacmp(&_bdAddr, &NullAddr) == 0)
    {
        return rc;
    }
    return 0;
}

bool SensorNetAddress::isMatch(SensorNetAddress* addr)
{
    return ((this->_channel == addr->_channel) && bacmp(&this->_bdAddr, &addr->_bdAddr) == 0);
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress& addr)
{
    this->_channel = addr._channel;
    this->_bdAddr = addr._bdAddr;
    return *this;
}

char* SensorNetAddress::sprint(char* buf)
{
    ba2str(const_cast<bdaddr_t*>(&_bdAddr), buf);
    sprintf(buf + strlen(buf), ".%d", _channel);
    return buf;
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

SensorNetwork::SensorNetwork()
{

}

SensorNetwork::~SensorNetwork()
{
}

int SensorNetwork::unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendToAddr)
{
    uint16_t ch = sendToAddr->getPortNo();
    RfcommPort* blep = &_rfPorts[ch - 1];
    int rc = 0;
    errno = 0;

    if ((rc = blep->send(payload, (uint32_t) payloadLength)) < 0)
    {
        D_NWSTACK("errno == %d in BlePort::sendto %d\n", errno, ch);
    } D_NWSTACK("sendto %u length = %d\n", ch, rc);
    return rc;
}

int SensorNetwork::broadcast(const uint8_t* payload, uint16_t payloadLength)
{
    int rc = 0;

    for (int i = 0; i < MAX_RFCOMM_CH; i++)
    {
        errno = 0;
        if (_rfPorts[i].getSock() > 0)
        {
            if ((rc = _rfPorts[i].send(payload, (uint32_t) payloadLength)) < 0)
            {
                D_NWSTACK("errno == %d in BlePort::sendto %d\n", errno, i + 1);
            }D_NWSTACK("sendto %u length = %d\n", i + 1, rc);
        }
    }
    return rc;
}

int SensorNetwork::read(uint8_t* buf, uint16_t bufLen)
{
    struct timeval timeout;
    fd_set recvfds;
    int maxSock = 0;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;    // 1 sec
    FD_ZERO(&recvfds);

    for (int i = 0; i < MAX_RFCOMM_CH; i++)
    {
        if (_rfPorts[i]._rfCommSock > 0)
        {
            if (maxSock < _rfPorts[i]._rfCommSock)
            {
                maxSock = _rfPorts[i]._rfCommSock;
            }
            FD_SET(_rfPorts[i]._rfCommSock, &recvfds);
        }
        else if (_rfPorts[i]._listenSock > 0)
        {
            if (maxSock < _rfPorts[i]._listenSock)
            {
                maxSock = _rfPorts[i]._listenSock;
            }
            FD_SET(_rfPorts[i]._listenSock, &recvfds);
        }
    }

    int rc = 0;
    if (select(maxSock + 1, &recvfds, 0, 0, &timeout) > 0)
    {
        for (int i = 0; i < MAX_RFCOMM_CH; i++)
        {
            if (_rfPorts[i]._rfCommSock > 0)
            {
                if (FD_ISSET(_rfPorts[i]._rfCommSock, &recvfds))
                {
                    rc = _rfPorts[i].recv(buf, bufLen);
                    if (rc == -1)
                    {
                        _rfPorts[i].close();
                    }
                }
            }
            else if (_rfPorts[i]._listenSock > 0)
            {
                if (FD_ISSET(_rfPorts[i]._listenSock, &recvfds))
                {
                    int sock = _rfPorts[i].accept(&_senderAddr);
                    if (sock > 0)
                    {
                        _rfPorts[i]._rfCommSock = sock;
                    }
                }
            }
        }
    }
    return rc;
}

/**
 *  Prepare UDP sockets and description of SensorNetwork like
 *   "UDP Multicast 225.1.1.1:1883 Gateway Port 10000".
 *   The description is for a start up prompt.
 */
void SensorNetwork::initialize(void)
{
    char param[MQTTSNGW_PARAM_MAX];
    string devAddr;
    SensorNetAddress sa;

    /*
     * theProcess->getParam( ) copies
     * a text specified by "Key" into param[] from the Gateway.conf
     *
     *  in Gateway.conf e.g.
     *
     *  # BLE
     *  RFCOMM=XX:XX:XX:XX:XX:XX.0
     *
     */
    if (theProcess->getParam("RFCOMMAddress", param) == 0)
    {
        devAddr = param;
        _description = "Bluetooth RFCOMM ";
        _description += param;
    }

    errno = 0;
    if (sa.setAddress(&devAddr) == -1)
    {
        throw EXCEPTION("Invalid Bluetooth Address", errno);
    }

    /*  Prepare BLE sockets */
    WRITELOG("Initialize RFCOMM\n");
    int rc = MAX_RFCOMM_CH;
    for (uint16_t i = 0; i < MAX_RFCOMM_CH; i++)
    {

        rc += _rfPorts[i].open(sa.getAddress(), i + 1);
    }
    if (rc == 0)
    {
        throw EXCEPTION("Can't open Bluetooth RFComms", errno);
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
 Class BleStack
 =========================================*/

RfcommPort::RfcommPort()
{
    _disconReq = false;
    _rfCommSock = 0;
    _listenSock = 0;
    _channel = 0;
}

RfcommPort::~RfcommPort()
{
    close();

    if (_listenSock > 0)
    {
        ::close(_listenSock);
    }
}

void RfcommPort::close(void)
{
    if (_rfCommSock > 0)
    {
        ::close(_rfCommSock);
        _rfCommSock = 0;
    }
}

int RfcommPort::open(bdaddr_t* devAddr, uint16_t channel)
{
    const int reuse = 1;

    if (channel < 1 || channel > 30)
    {
        D_NWSTACK("error Channel undefined in BlePort::open\n");
        return 0;
    }


    /*------ Create unicast socket --------*/
    _listenSock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (_listenSock < 0)
    {
        D_NWSTACK("error can't create Rfcomm socket in BlePort::open\n");
        return 0;
    }

    sockaddr_rc addru;
    addru.rc_family = AF_BLUETOOTH;
    addru.rc_channel = channel;
    bacpy(&addru.rc_bdaddr, devAddr);

    uint8_t buf[20];
    ba2str(devAddr, (char*) buf);
    setsockopt(_listenSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    errno = 0;
    if (::bind(_listenSock, (sockaddr*) &addru, sizeof(addru)) < 0)
    {
        WRITELOG("\033[0m\033[0;31mCan't bind RFCOMM CH = %d  %s\033[0m\033[0;37m\n", channel, strerror(errno));
        ::close(_listenSock);
        return 0;
    }
    _channel = channel;
    ::listen(_listenSock, 1);
    WRITELOG("Listen RFCOMM CH = %d\n", channel);
    return 1;
}

int RfcommPort::send(const uint8_t* buf, uint32_t length)
{
    return ::send(_rfCommSock, buf, length, 0);
}

int RfcommPort::recv(uint8_t* buf, uint16_t len)
{
    int rc = 0;
    errno = 0;

    rc = ::read(_rfCommSock, buf, len);
    if (rc < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno = %d in BlePort::recv\n", errno);
        return -1;
    }
    return rc;
}

int RfcommPort::accept(SensorNetAddress* addr)
{
    struct sockaddr_rc devAddr = { 0 };
    socklen_t opt = sizeof(devAddr);

    int sock = 0;
    errno = 0;

    sock = ::accept(_listenSock, (sockaddr *) &devAddr, &opt);
    if (sock < 0 && errno != EAGAIN)
    {
        D_NWSTACK("errno == %d in BlePort::recv\n", errno);
        return -1;
    }
    bdaddr_t bdAddr = devAddr.rc_bdaddr;
    addr->setAddress(bdAddr, _channel);
    return sock;
}

int RfcommPort::getSock(void)
{
    return _rfCommSock;
}
