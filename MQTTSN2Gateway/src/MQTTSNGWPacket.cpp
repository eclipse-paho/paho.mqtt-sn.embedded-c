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

#include "MQTTSNGateway.h"
#include "MQTTSNGWPacket.h"
#include "SensorNetwork.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace MQTTSNGW;

MQTTSNPacket::MQTTSNPacket(void)
{
    _buf = nullptr;
    _bufLen = 0;
}

MQTTSNPacket::MQTTSNPacket(MQTTSNPacket& packet)
{
    _buf = (unsigned char*) malloc(packet._bufLen);
    if (_buf)
    {
        _bufLen = packet._bufLen;
        memcpy(_buf, packet._buf, _bufLen);
    }
    else
    {
        _buf = nullptr;
        _bufLen = 0;
    }
}

MQTTSNPacket::~MQTTSNPacket()
{
    if (_buf)
    {
        free(_buf);
    }
}

int MQTTSNPacket::unicast(SensorNetwork* network, SensorNetAddress* sendTo)
{
    return network->unicast(_buf, _bufLen, sendTo);
}

int MQTTSNPacket::broadcast(SensorNetwork* network)
{
    return network->broadcast(_buf, _bufLen);
}

int MQTTSNPacket::serialize(uint8_t* buf)
{
    buf = _buf;
    return _bufLen;
}

int MQTTSNPacket::desirialize(unsigned char* buf, unsigned short len)
{
    if (_buf)
    {
        free(_buf);
    }

    _buf = (unsigned char*) calloc(len, sizeof(unsigned char));
    if (_buf)
    {
        memcpy(_buf, buf, len);
        _bufLen = len;
    }
    else
    {
        _bufLen = 0;
    }
    return _bufLen;
}

int MQTTSNPacket::recv(SensorNetwork* network)
{
    uint8_t buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = network->read((uint8_t*) buf, MQTTSNGW_MAX_PACKET_SIZE);
    if (len > 1)
    {
        len = desirialize(buf, len);
    }
    return len;
}

int MQTTSNPacket::getType(void)
{
    if (_bufLen == 0)
    {
        return 0;
    }
    int value = 0;
    int p = MQTTSNPacket_decode(_buf, _bufLen, &value);
    return _buf[p];
}

bool MQTTSNPacket::isPUBWOS(void)
{
    return getType() == MQTTSN_PUBWOS;
}

unsigned char* MQTTSNPacket::getPacketData(void)
{
    return _buf;
}

int MQTTSNPacket::getPacketLength(void)
{
    return _bufLen;
}

const char* MQTTSNPacket::getName()
{
    return MQTTSNPacket_name(getType());
}

/*--- Discovery ---*/

int MQTTSNPacket::setADVERTISE(uint8_t gatewayid, uint16_t duration)
{
    unsigned char buf[5];
    int len = MQTTSNSerialize_advertise(buf, sizeof(buf), gatewayid, duration);
    return desirialize(buf, len);
}

int MQTTSNPacket::setGWINFO(uint8_t gatewayId)
{
    unsigned char buf[4];
    MQTTSN_data noAddr = {0, nullptr};
    int len = MQTTSNSerialize_gwinfo(buf, sizeof(buf), gatewayId, &noAddr);
    return desirialize(buf, len);
}

int MQTTSNPacket::getSERCHGW(MQTTSN_data* networkInfo)
{
    return MQTTSNDeserialize_searchgw(networkInfo, _buf, _bufLen);
}

/*--- Connection ---*/

int MQTTSNPacket::setConnect(void)
{
    unsigned char buf[64];
    MQTTSNPacket_connectData data = MQTTSNPacket_connectData_initializer;
    MQTTSN_string clientId = {false, 8, (char*)"client01"};
    data.clientID = clientId;
    int len = MQTTSNSerialize_connect(buf, sizeof(buf), &data);
    return desirialize(buf, len);
}

int MQTTSNPacket::setCONNECT(MQTTSNPacket_connectData* options)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = MQTTSNSerialize_connect(buf, sizeof(buf), options);
    return desirialize(buf, len);
}

int MQTTSNPacket::getCONNECT(MQTTSNPacket_connectData* data)
{
    return MQTTSNDeserialize_connect(data, _buf, _bufLen);
}

int MQTTSNPacket::setCONNACK(MQTTSNPacket_connackData* data)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = MQTTSNSerialize_connack(buf, sizeof(buf), data);
    return desirialize(buf, len);
}

int MQTTSNPacket::getCONNACK(MQTTSNPacket_connackData* data)
{
    return MQTTSNDeserialize_connack(data, _buf, _bufLen);
}

bool MQTTSNPacket::isAccepted(void)
{
    if (getType() != MQTTSN_CONNACK)
    {
        return false;
    }
    MQTTSNPacket_connackData data;
    memset(&data, 0, sizeof(data));
    if (MQTTSNDeserialize_connack(&data, _buf, _bufLen) > 0)
    {
        return data.reasonCode == MQTT_SN_RC_SUCCESS;
    }
    return false;
}

int MQTTSNPacket::setPINGREQ(uint16_t packetid)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = MQTTSNSerialize_pingreq(buf, sizeof(buf), packetid);
    return desirialize(buf, len);
}

int MQTTSNPacket::getPINGREQ(uint16_t* packetid)
{
    return MQTTSNDeserialize_pingreq(packetid, _buf, _bufLen);
}

int MQTTSNPacket::setPINGRESP(uint16_t packetid, int messages_remaining)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = MQTTSNSerialize_pingresp(buf, sizeof(buf), packetid, messages_remaining);
    return desirialize(buf, len);
}

int MQTTSNPacket::setDISCONNECT(uint8_t reason_code)
{
    unsigned char buf[8];
    int len = MQTTSNSerialize_disconnect(buf, sizeof(buf), reason_code);
    return desirialize(buf, len);
}

int MQTTSNPacket::getDISCONNECT(MQTTSNPacket_disconnectData* data)
{
    return MQTTSNDeserialize_disconnect(data, _buf, _bufLen);
}

/*--- Sleep ---*/

int MQTTSNPacket::getSLEEPREQ(uint8_t* retainTopicAliases, uint16_t* packetid, uint32_t* duration)
{
    return MQTTSNDeserialize_sleepreq(retainTopicAliases, packetid, duration, _buf, _bufLen);
}

int MQTTSNPacket::setSLEEPRESP(uint8_t durationPresent, uint16_t packetid,
        uint32_t duration, uint8_t returncode)
{
    unsigned char buf[12];
    int len = MQTTSNSerialize_sleepresp(buf, sizeof(buf), durationPresent, packetid, duration, returncode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setWAKEUP(void)
{
    unsigned char buf[4];
    int len = MQTTSNSerialize_wakeup(buf, sizeof(buf));
    return desirialize(buf, len);
}

/*--- Publish ---*/

int MQTTSNPacket::setPUBLISH(uint8_t dup, int qos, uint8_t retained, uint16_t packetid,
        const MQTTSN_topic* topic, uint8_t* payload, int32_t payloadlen)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = MQTTSNSerialize_publish(buf, sizeof(buf), dup, qos, retained, packetid,
            topic, payload, payloadlen);
    return desirialize(buf, len);
}

int MQTTSNPacket::getPUBLISH(uint8_t* dup, int* qos, uint8_t* retained, uint16_t* packetid,
        MQTTSN_topic* topic, uint8_t** payload, int32_t* payloadlen)
{
    return MQTTSNDeserialize_publish(dup, qos, retained, packetid, topic,
            payload, payloadlen, _buf, _bufLen);
}

int MQTTSNPacket::setPUBACK(uint16_t packetid, uint8_t returncode)
{
    unsigned char buf[8];
    int len = MQTTSNSerialize_puback(buf, sizeof(buf), packetid, returncode);
    return desirialize(buf, len);
}

int MQTTSNPacket::getPUBACK(uint16_t* packetid, uint8_t* returncode)
{
    return MQTTSNDeserialize_puback(packetid, returncode, _buf, _bufLen);
}

int MQTTSNPacket::setPUBREC(uint16_t packetid, uint8_t returncode)
{
    unsigned char buf[8];
    int len = MQTTSNSerialize_pubrec(buf, sizeof(buf), packetid, returncode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPUBREL(uint16_t packetid, uint8_t returncode)
{
    unsigned char buf[8];
    int len = MQTTSNSerialize_pubrel(buf, sizeof(buf), packetid, returncode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPUBCOMP(uint16_t packetid, uint8_t returncode)
{
    unsigned char buf[8];
    int len = MQTTSNSerialize_pubcomp(buf, sizeof(buf), packetid, returncode);
    return desirialize(buf, len);
}

int MQTTSNPacket::getACK(uint16_t* packetid, uint8_t* returncode)
{
    uint8_t type;
    return MQTTSNDeserialize_ack(&type, packetid, returncode, _buf, _bufLen);
}

int MQTTSNPacket::setPUBWOS(uint8_t retained, const MQTTSN_topic* topic,
        uint8_t* payload, int32_t payloadlen)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = MQTTSNSerialize_pubwos(buf, sizeof(buf), retained, topic, payload, payloadlen);
    return desirialize(buf, len);
}

int MQTTSNPacket::getPUBWOS(uint8_t* retained, MQTTSN_topic* topic,
        uint8_t** payload, int32_t* payloadlen)
{
    return MQTTSNDeserialize_pubwos(retained, topic, payload, payloadlen, _buf, _bufLen);
}

/*--- Register / RegAck ---*/

int MQTTSNPacket::setREGISTER(uint8_t topic_alias_present, uint16_t topicid,
        uint16_t packetid, MQTTSN_string* topicname)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int len = MQTTSNSerialize_register(buf, sizeof(buf), topic_alias_present, topicid, packetid, topicname);
    return desirialize(buf, len);
}

int MQTTSNPacket::getREGISTER(uint8_t* topic_alias_present, uint16_t* topicid,
        uint16_t* packetid, MQTTSN_string* topicname)
{
    return MQTTSNDeserialize_register(topic_alias_present, topicid, packetid, topicname, _buf, _bufLen);
}

int MQTTSNPacket::setREGACK(uint8_t topic_type, uint8_t topic_alias_present,
        uint16_t topicid, uint16_t packetid, uint8_t returncode)
{
    unsigned char buf[12];
    int len = MQTTSNSerialize_regack(buf, sizeof(buf), topic_type, topic_alias_present, topicid, packetid, returncode);
    return desirialize(buf, len);
}

int MQTTSNPacket::getREGACK(uint8_t* topic_type, uint8_t* topic_alias_present,
        uint16_t* topicid, uint16_t* packetid, uint8_t* returncode)
{
    return MQTTSNDeserialize_regack(topic_type, topic_alias_present, topicid, packetid, returncode, _buf, _bufLen);
}

/*--- Subscribe / Suback ---*/

int MQTTSNPacket::getSUBSCRIBE(int* qos, uint8_t* retain_handling, uint8_t* rap,
        uint8_t* no_local, uint16_t* packetid, MQTTSN_topic* topic)
{
    int32_t qos32;
    int rc = MQTTSNDeserialize_subscribe(&qos32, retain_handling, rap, no_local, packetid, topic, _buf, _bufLen);
    *qos = (int) qos32;
    return rc;
}

int MQTTSNPacket::setSUBACK(uint8_t topic_type, uint8_t topic_alias_present,
        uint16_t topicid, uint16_t packetid, uint8_t returncode)
{
    unsigned char buf[12];
    int len = MQTTSNSerialize_suback(buf, sizeof(buf), topic_type, topic_alias_present, topicid, packetid, returncode);
    return desirialize(buf, len);
}

/*--- Unsubscribe / Unsuback ---*/

int MQTTSNPacket::getUNSUBSCRIBE(uint16_t* packetid, MQTTSN_topic* topic)
{
    return MQTTSNDeserialize_unsubscribe(packetid, topic, _buf, _bufLen);
}

int MQTTSNPacket::setUNSUBACK(uint16_t packetid, uint8_t returncode)
{
    unsigned char buf[8];
    int len = MQTTSNSerialize_unsuback(buf, sizeof(buf), packetid, returncode);
    return desirialize(buf, len);
}

/*--- Logging helpers ---*/

char* MQTTSNPacket::print(char* pbuf)
{
    char* ptr = pbuf;
    char** pptr = &pbuf;
    int size = _bufLen > SIZE_OF_LOG_PACKET ? SIZE_OF_LOG_PACKET : _bufLen;

    for (int i = 0; i < size; i++)
    {
        snprintf(*pptr, 4, " %02X", *(_buf + i));
        *pptr += 3;
    }
    **pptr = 0;
    return ptr;
}

char* MQTTSNPacket::getMsgId(char* pbuf)
{
    int value = 0;
    int p = 0;

    switch (getType())
    {
    case MQTTSN_PUBLISH:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        if (_buf[p + 1] & 0x80)
        {
            snprintf(pbuf, 6, "+%02X%02X", _buf[p + 4], _buf[p + 5]);
        }
        else
        {
            snprintf(pbuf, 6, " %02X%02X", _buf[p + 4], _buf[p + 5]);
        }
        break;
    case MQTTSN_PUBACK:
        snprintf(pbuf, 6, " %02X%02X", _buf[2], _buf[3]);
        break;
    case MQTTSN_REGISTER:
    case MQTTSN_REGACK:
        snprintf(pbuf, 6, " %02X%02X", _buf[4], _buf[5]);
        break;
    case MQTTSN_PUBREC:
    case MQTTSN_PUBREL:
    case MQTTSN_PUBCOMP:
    case MQTTSN_UNSUBACK:
        snprintf(pbuf, 6, " %02X%02X", _buf[2], _buf[3]);
        break;
    case MQTTSN_SUBSCRIBE:
    case MQTTSN_UNSUBSCRIBE:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        snprintf(pbuf, 6, " %02X%02X", _buf[p + 2], _buf[p + 3]);
        break;
    case MQTTSN_SUBACK:
        snprintf(pbuf, 6, " %02X%02X", _buf[4], _buf[5]);
        break;
    default:
        snprintf(pbuf, 5, "    ");
        break;
    }
    if (strcmp(pbuf, " 0000") == 0)
    {
        snprintf(pbuf, 5, "    ");
    }
    return pbuf;
}

int MQTTSNPacket::getMsgId(void)
{
    int value = 0;
    int p = 0;
    uint16_t msgId = 0;

    switch (getType())
    {
    case MQTTSN_PUBLISH:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        msgId = (uint16_t)((_buf[p + 4] << 8) | _buf[p + 5]);
        break;
    case MQTTSN_PUBACK:
        msgId = (uint16_t)((_buf[2] << 8) | _buf[3]);
        break;
    case MQTTSN_REGISTER:
    case MQTTSN_REGACK:
        msgId = (uint16_t)((_buf[4] << 8) | _buf[5]);
        break;
    case MQTTSN_PUBREC:
    case MQTTSN_PUBREL:
    case MQTTSN_PUBCOMP:
    case MQTTSN_UNSUBACK:
        msgId = (uint16_t)((_buf[2] << 8) | _buf[3]);
        break;
    case MQTTSN_SUBSCRIBE:
    case MQTTSN_UNSUBSCRIBE:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        msgId = (uint16_t)((_buf[p + 2] << 8) | _buf[p + 3]);
        break;
    case MQTTSN_SUBACK:
        msgId = (uint16_t)((_buf[4] << 8) | _buf[5]);
        break;
    default:
        break;
    }
    return (int) msgId;
}

void MQTTSNPacket::setMsgId(uint16_t msgId)
{
    int value = 0;
    int p = 0;

    switch (getType())
    {
    case MQTTSN_PUBLISH:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        _buf[p + 4] = (unsigned char)(msgId >> 8);
        _buf[p + 5] = (unsigned char)(msgId & 0xFF);
        break;
    case MQTTSN_PUBACK:
        _buf[2] = (unsigned char)(msgId >> 8);
        _buf[3] = (unsigned char)(msgId & 0xFF);
        break;
    case MQTTSN_REGISTER:
    case MQTTSN_REGACK:
        _buf[4] = (unsigned char)(msgId >> 8);
        _buf[5] = (unsigned char)(msgId & 0xFF);
        break;
    case MQTTSN_PUBREC:
    case MQTTSN_PUBREL:
    case MQTTSN_PUBCOMP:
    case MQTTSN_UNSUBACK:
        _buf[2] = (unsigned char)(msgId >> 8);
        _buf[3] = (unsigned char)(msgId & 0xFF);
        break;
    case MQTTSN_SUBSCRIBE:
    case MQTTSN_UNSUBSCRIBE:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        _buf[p + 2] = (unsigned char)(msgId >> 8);
        _buf[p + 3] = (unsigned char)(msgId & 0xFF);
        break;
    case MQTTSN_SUBACK:
        _buf[4] = (unsigned char)(msgId >> 8);
        _buf[5] = (unsigned char)(msgId & 0xFF);
        break;
    default:
        break;
    }
}

bool MQTTSNPacket::isDuplicate(void)
{
    int value = 0;
    int p = MQTTSNPacket_decode(_buf, _bufLen, &value);
    return (_buf[p + 1] & 0x80) != 0;
}
