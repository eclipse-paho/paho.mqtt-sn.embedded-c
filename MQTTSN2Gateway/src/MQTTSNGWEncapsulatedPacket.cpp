/**************************************************************************************
 * Copyright (c) 2018, 2026 Tomoaki Yamaguchi, Ian Craggs
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
#include "MQTTSNGWPacket.h"
#include "MQTTSNGWEncapsulatedPacket.h"
#include "MQTTSNPacket.h"
#include <string.h>

using namespace MQTTSNGW;
using namespace std;

WirelessNodeId::WirelessNodeId() :
        _len { 0 }, _nodeId { 0 }
{

}

WirelessNodeId::~WirelessNodeId()
{
    if (_nodeId)
    {
        free(_nodeId);
    }
}

void WirelessNodeId::setId(uint8_t* id, uint8_t len)
{
    if (_nodeId)
    {
        free(_nodeId);
    }
    uint8_t* buf = (uint8_t*) malloc(len);
    if (buf)
    {
        memcpy(buf, id, len);
        _len = len;
        _nodeId = buf;
    }
    else
    {
        _nodeId = nullptr;
        _len = 0;
    }
}

void WirelessNodeId::setId(WirelessNodeId* id)
{
    setId(id->_nodeId, id->_len);
}

bool WirelessNodeId::operator ==(WirelessNodeId& id)
{
    if (_len == id._len)
    {
        return memcmp(_nodeId, id._nodeId, _len) == 0;
    }
    else
    {
        return false;
    }
}

/*
 *    Class MQTTSNGWEncapsulatedPacket
 */
MQTTSNGWEncapsulatedPacket::MQTTSNGWEncapsulatedPacket() :
        _mqttsn { 0 }, _ctrl { 0 }
{

}

MQTTSNGWEncapsulatedPacket::MQTTSNGWEncapsulatedPacket(MQTTSNPacket* packet) :
        _mqttsn { packet }, _ctrl { 0 }
{

}

MQTTSNGWEncapsulatedPacket::~MQTTSNGWEncapsulatedPacket()
{
    /*  Do not delete the MQTTSNPacket.  MQTTSNPacket is deleted by delete Event */
}

int MQTTSNGWEncapsulatedPacket::unicast(SensorNetwork* network, SensorNetAddress* sendTo)
{
    uint8_t buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = serialize(buf);
    return network->unicast(buf, len, sendTo);
}

int MQTTSNGWEncapsulatedPacket::serialize(uint8_t* buf)
{
    int len = 0;
    buf[0] = _id._len + 3;
    buf[1] = MQTTSN_FORWARDER;
    buf[2] = _ctrl;
    memcpy(buf + 3, _id._nodeId, _id._len);
    if (_mqttsn)
    {
        len = _mqttsn->getPacketLength();
        memcpy(buf + buf[0], _mqttsn->getPacketData(), len);
    }
    return buf[0] + len;
}

int MQTTSNGWEncapsulatedPacket::desirialize(unsigned char* buf, unsigned short len)
{
    if (_mqttsn)
    {
        delete _mqttsn;
        _mqttsn = nullptr;
    }

    _ctrl = buf[2];
    _id.setId(buf + 3, buf[0] - 3);

    _mqttsn = new MQTTSNPacket;
    _mqttsn->desirialize(buf + buf[0], len - buf[0]);
    return buf[0];
}

int MQTTSNGWEncapsulatedPacket::getType(void)
{
    return MQTTSN_FORWARDER;
}

const char* MQTTSNGWEncapsulatedPacket::getName()
{
    return MQTTSNPacket_name(MQTTSN_FORWARDER);
}

MQTTSNPacket* MQTTSNGWEncapsulatedPacket::getMQTTSNPacket(void)
{
    return _mqttsn;
}

WirelessNodeId* MQTTSNGWEncapsulatedPacket::getWirelessNodeId(void)
{
    return &_id;
}

void MQTTSNGWEncapsulatedPacket::setWirelessNodeId(WirelessNodeId* id)
{
    _id.setId(id);
}

char* MQTTSNGWEncapsulatedPacket::print(char* pbuf)
{
    char* ptr = pbuf;
    char** pptr = &pbuf;

    uint8_t buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = serialize(buf);
    int size = len > SIZE_OF_LOG_PACKET ? SIZE_OF_LOG_PACKET : len;

    for (int i = 1; i < size; i++)
    {
        snprintf(*pptr, 4, " %02X", *(buf + i));
        *pptr += 3;
    }
    **pptr = 0;
    return ptr;
}

