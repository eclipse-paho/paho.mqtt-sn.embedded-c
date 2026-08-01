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
#ifdef RFCOMM

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
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "LMqttsnClientApp.h"
#include "LNetworkRfcomm.h"
#include "LTimer.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern uint16_t getUint16(const uint8_t* pos);
extern uint32_t getUint32(const uint8_t* pos);
extern LScreen* theScreen;
extern bool theClientMode;
extern LRfcommConfig theNetcon;
/*=========================================
 Class LNetwork
 =========================================*/
LNetwork::LNetwork()
{
    _sleepflg = false;
    _returnCode = 0;
}

LNetwork::~LNetwork()
{

}

int LNetwork::broadcast(const uint8_t* xmitData, uint16_t dataLen)
{
    return LRfcommPort::unicast(xmitData, dataLen);
}

int LNetwork::unicast(const uint8_t* xmitData, uint16_t dataLen)
{
    return LRfcommPort::unicast(xmitData, dataLen);
}

uint8_t* LNetwork::getMessage(int* len)
{
    *len = 0;
    if (checkRecvBuf())
    {
        uint16_t recvLen = LRfcommPort::recv(_rxDataBuf, MQTTSN_MAX_PACKET_SIZE, false);

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
}

void LNetwork::setFixedGwAddress(void)
{
    _channel = LRfcommPort::_channel;
    str2ba( theNetcon.gwAddress, (bdaddr_t*)_gwAddress);
}

bool LNetwork::initialize(LRfcommConfig* config)
{
    return LRfcommPort::open(config);
}

void LNetwork::setSleep()
{
    _sleepflg = true;
}

bool LNetwork::isBroadcastable()
{
    return false;
}

/*=========================================
 Class RFCOMM Stack
 =========================================*/
LRfcommPort::LRfcommPort()
{
    _disconReq = false;
    _sockRfcomm = 0;
    _channel = 0;
}

LRfcommPort::~LRfcommPort()
{
    close();
}

void LRfcommPort::close()
{
    if (_sockRfcomm > 0)
    {
        ::close(_sockRfcomm);
        _sockRfcomm = 0;
    }
}

bool LRfcommPort::open(LRfcommConfig* config)
{
    const int reuse = 1;
    str2ba(config->gwAddress, (bdaddr_t*)_gwAddress);
    _channel = config->channel;

    if (_channel == 0 || _gwAddress == 0 )
    {
        D_NWLOG("\033[0m\033[0;31merror Bluetooth Address in LRfcommPort::open\033[0m\033[0;37m\n");
        DISPLAY("\033[0m\033[0;31m\nerror Bluetooth Address in LRfcommPort::open\033[0m\033[0;37m\n");
        return false;
    }

    _sockRfcomm = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (_sockRfcomm < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror Can't create socket in LRfcommPort::open\033[0m\033[0;37m\n");
        DISPLAY("\033[0m\033[0;31m\nerror Can't create socket in LRfcommPort::open\033[0m\033[0;37m\n");
        return false;
    }

    setsockopt(_sockRfcomm, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_rc addru = { 0 };
    addru.rc_family = AF_BLUETOOTH;
    addru.rc_channel = _channel;
    memcpy(&addru.rc_bdaddr, _gwAddress, 6);

    char bufgw[30];
    ba2str(&addru.rc_bdaddr, bufgw);
    DISPLAY("GW MAC = %s  RFCOMM CH = %d\n", bufgw, addru.rc_channel);

    // connect to server
    errno = 0;
    int status = connect(_sockRfcomm, (struct sockaddr *) &addru, sizeof(addru));
    if (status < 0)
    {
        D_NWLOG("\033[0m\033[0;31merror = %d Can't connect to GW in LRfcommPort::open\033[0m\033[0;37m\n", errno);
        DISPLAY("\033[0m\033[0;31merror = %d Can't connect to GW Ble socket in LRfcommPort::open\033[0m\033[0;37m\n",errno);
        close();
        return false;
    }
    return true;
}

int LRfcommPort::unicast(const uint8_t* buf, uint32_t length)
{
    int status = ::write(_sockRfcomm, buf, length);
    if (status < 0)
    {
        D_NWLOG("errno == %d in LRfcommPort::unicast\n", errno);
        DISPLAY("errno == %d in LRfcommPort::unicast\n", errno);
    }
    else
    {
        D_NWLOG("sendto %-2d", _channel);
        for (uint16_t i = 0; i < length; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }
        D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34msendto %-2dch", _channel);
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

bool LRfcommPort::checkRecvBuf()
{
    uint8_t buf[2];
    if (::recv(_sockRfcomm, buf, 1, MSG_DONTWAIT | MSG_PEEK) > 0)
    {
        return true;
    }
    return false;
}

int LRfcommPort::recv(uint8_t* buf, uint16_t length, bool flg)
{
    int flags = flg ? MSG_DONTWAIT : 0;
    int status = ::recv(_sockRfcomm, buf, length, flags);

    if (status < 0 && errno != EAGAIN)
    {
        D_NWLOG("\033[0m\033[0;31merrno = %d in LRfcommPort::recv \033[0m\033[0;37m\n", errno);
        DISPLAY("\033[0m\033[0;31merrno = %d in LRfcommPort::recv \033[0m\033[0;37m\n", errno);
    }
    else if (status > 0)
    {
        D_NWLOG("\nrecved  ");
        for (uint16_t i = 0; i < status; i++)
        {
            D_NWLOG(" %02x", *(buf + i));
        }
        D_NWLOG("\n");

        if (!theClientMode)
        {
            char sbuf[SCREEN_BUFF_SIZE];
            int pos = 0;
            sprintf(sbuf, "\033[0;34mrecved ");
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

