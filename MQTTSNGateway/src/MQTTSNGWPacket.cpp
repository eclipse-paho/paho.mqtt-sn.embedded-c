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

#include "MQTTSNGateway.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNPacket.h"
#include "SensorNetwork.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace MQTTSNGW;
int readInt(char** pptr);
void writeInt(unsigned char** pptr, int msgId);

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

bool MQTTSNPacket::isQoSMinusPUBLISH(void)
{
    if (_bufLen == 0)
    {
        return false;;
    }
    int value = 0;
    int p = MQTTSNPacket_decode(_buf, _bufLen, &value);
    return ((_buf[p] == MQTTSN_PUBLISH) && ((_buf[p + 1] & 0x60) == 0x60));
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

int MQTTSNPacket::setADVERTISE(uint8_t gatewayid, uint16_t duration)
{
    unsigned char buf[5];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_advertise(buf, buflen, (unsigned char) gatewayid, (unsigned short) duration);
    return desirialize(buf, len);
}

int MQTTSNPacket::setGWINFO(uint8_t gatewayId)
{
    unsigned char buf[3];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_gwinfo(buf, buflen, (unsigned char) gatewayId, 0, 0);
    return desirialize(buf, len);
}

int MQTTSNPacket::setConnect(void)
{
    unsigned char buf[40];
    int buflen = sizeof(buf);
    MQTTSNPacket_connectData data;
    data.clientID.cstring = (char*) "client01";
    int len = MQTTSNSerialize_connect(buf, buflen, &data);
    return desirialize(buf, len);
}

bool MQTTSNPacket::isAccepted(void)
{
    return (getType() == MQTTSN_CONNACK) && (_buf[2] == MQTTSN_RC_ACCEPTED);
}

int MQTTSNPacket::setCONNACK(uint8_t returnCode)
{
    unsigned char buf[3];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_connack(buf, buflen, (int) returnCode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setWILLTOPICREQ(void)
{
    unsigned char buf[2];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_willtopicreq(buf, buflen);
    return desirialize(buf, len);
}

int MQTTSNPacket::setWILLMSGREQ(void)
{
    unsigned char buf[2];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_willmsgreq(buf, buflen);
    return desirialize(buf, len);
}

int MQTTSNPacket::setREGISTER(uint16_t topicId, uint16_t msgId, MQTTSNString* topicName)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_register(buf, buflen, (unsigned short) topicId, (unsigned short) msgId, topicName);
    return desirialize(buf, len);
}

int MQTTSNPacket::setREGACK(uint16_t topicId, uint16_t msgId, uint8_t returnCode)
{
    unsigned char buf[7];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_regack(buf, buflen, (unsigned short) topicId, (unsigned short) msgId, (unsigned char) returnCode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPUBLISH(uint8_t dup, int qos, uint8_t retained, uint16_t msgId, MQTTSN_topicid topic, uint8_t* payload,
        uint16_t payloadlen)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_publish(buf, buflen, (unsigned char) dup, qos, (unsigned char) retained, (unsigned short) msgId,
            topic, (unsigned char*) payload, (int) payloadlen);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPUBACK(uint16_t topicId, uint16_t msgId, uint8_t returnCode)
{
    unsigned char buf[7];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_puback(buf, buflen, (unsigned short) topicId, (unsigned short) msgId, (unsigned char) returnCode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPUBREC(uint16_t msgId)
{
    unsigned char buf[4];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_pubrec(buf, buflen, (unsigned short) msgId);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPUBREL(uint16_t msgId)
{
    unsigned char buf[4];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_pubrel(buf, buflen, (unsigned short) msgId);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPUBCOMP(uint16_t msgId)
{
    unsigned char buf[4];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_pubcomp(buf, buflen, (unsigned short) msgId);
    return desirialize(buf, len);
}

int MQTTSNPacket::setSUBACK(int qos, uint16_t topicId, uint16_t msgId, uint8_t returnCode)
{
    unsigned char buf[8];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_suback(buf, buflen, qos, (unsigned short) topicId, (unsigned short) msgId,
            (unsigned char) returnCode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setUNSUBACK(uint16_t msgId)
{
    unsigned char buf[4];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_unsuback(buf, buflen, (unsigned short) msgId);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPINGRESP(void)
{
    unsigned char buf[32];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_pingresp(buf, buflen);
    return desirialize(buf, len);
}

int MQTTSNPacket::setDISCONNECT(uint16_t duration)
{
    unsigned char buf[4];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_disconnect(buf, buflen, (int) duration);
    return desirialize(buf, len);
}

int MQTTSNPacket::setWILLTOPICRESP(uint8_t returnCode)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_willtopicresp(buf, buflen, (int) returnCode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setWILLMSGRESP(uint8_t returnCode)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_willmsgresp(buf, buflen, (int) returnCode);
    return desirialize(buf, len);
}

int MQTTSNPacket::setCONNECT(MQTTSNPacket_connectData* options)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_connect(buf, buflen, options);
    return desirialize(buf, len);
}

int MQTTSNPacket::setPINGREQ(MQTTSNString* clientId)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    int buflen = sizeof(buf);
    int len = MQTTSNSerialize_pingreq(buf, buflen, *clientId);
    return desirialize(buf, len);
}

int MQTTSNPacket::getSERCHGW(uint8_t* radius)
{
    return MQTTSNDeserialize_searchgw((unsigned char*) radius, (unsigned char*) _buf, _bufLen);
}

int MQTTSNPacket::getCONNECT(MQTTSNPacket_connectData* data)
{
    return MQTTSNDeserialize_connect(data, _buf, _bufLen);
}

int MQTTSNPacket::getCONNACK(uint8_t* returnCode)
{
    return MQTTSNSerialize_connack(_buf, _bufLen, (int) *returnCode);
}

int MQTTSNPacket::getWILLTOPIC(int* willQoS, uint8_t* willRetain, MQTTSNString* willTopic)
{
    return MQTTSNDeserialize_willtopic((int*) willQoS, (unsigned char*) willRetain, willTopic, _buf, _bufLen);
}

int MQTTSNPacket::getWILLMSG(MQTTSNString* willmsg)
{
    return MQTTSNDeserialize_willmsg(willmsg, _buf, _bufLen);
}

int MQTTSNPacket::getREGISTER(uint16_t* topicId, uint16_t* msgId, MQTTSNString* topicName)
{
    return MQTTSNDeserialize_register((unsigned short*) topicId, (unsigned short*) msgId, topicName, _buf, _bufLen);
}

int MQTTSNPacket::getREGACK(uint16_t* topicId, uint16_t* msgId, uint8_t* returnCode)
{
    return MQTTSNDeserialize_regack((unsigned short*) topicId, (unsigned short*) msgId, (unsigned char*) returnCode, _buf,
            _bufLen);
}

int MQTTSNPacket::getPUBLISH(uint8_t* dup, int* qos, uint8_t* retained, uint16_t* msgId, MQTTSN_topicid* topic,
        uint8_t** payload, int* payloadlen)
{
    return MQTTSNDeserialize_publish((unsigned char*) dup, qos, (unsigned char*) retained, (unsigned short*) msgId, topic,
            (unsigned char**) payload, (int*) payloadlen, _buf, _bufLen);
}

int MQTTSNPacket::getPUBACK(uint16_t* topicId, uint16_t* msgId, uint8_t* returnCode)
{
    return MQTTSNDeserialize_puback((unsigned short*) topicId, (unsigned short*) msgId, (unsigned char*) returnCode, _buf,
            _bufLen);
}

int MQTTSNPacket::getACK(uint16_t* msgId)
{
    unsigned char type;
    return MQTTSNDeserialize_ack(&type, (unsigned short*) msgId, _buf, _bufLen);
}

int MQTTSNPacket::getSUBSCRIBE(uint8_t* dup, int* qos, uint16_t* msgId, MQTTSN_topicid* topicFilter)
{
    return MQTTSNDeserialize_subscribe((unsigned char*) dup, qos, (unsigned short*) msgId, topicFilter, _buf, _bufLen);
}

int MQTTSNPacket::getUNSUBSCRIBE(uint16_t* msgId, MQTTSN_topicid* topicFilter)
{
    return MQTTSNDeserialize_unsubscribe((unsigned short*) msgId, topicFilter, _buf, _bufLen);
}

int MQTTSNPacket::getPINGREQ(void)
{
    if (getType() == MQTTSN_PINGRESP && _bufLen > 2)
    {
        return _bufLen - 2;
    }
    return 0;
}

int MQTTSNPacket::getDISCONNECT(uint16_t* duration)
{
    int dur = 0;
    int rc = MQTTSNDeserialize_disconnect(&dur, _buf, _bufLen);
    *duration = (uint16_t) dur;
    return rc;
}

int MQTTSNPacket::getWILLTOPICUPD(uint8_t* willQoS, uint8_t* willRetain, MQTTSNString* willTopic)
{
    return MQTTSNDeserialize_willtopicupd((int*) willQoS, (unsigned char*) willRetain, willTopic, _buf, _bufLen);
}

int MQTTSNPacket::getWILLMSGUPD(MQTTSNString* willMsg)
{
    return MQTTSNDeserialize_willmsgupd(willMsg, _buf, _bufLen);
}

char* MQTTSNPacket::print(char* pbuf)
{
    char* ptr = pbuf;
    char** pptr = &pbuf;
    int size = _bufLen > SIZE_OF_LOG_PACKET ? SIZE_OF_LOG_PACKET : _bufLen;

    for (int i = 0; i < size; i++)
    {
        sprintf(*pptr, " %02X", *(_buf + i));
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
            sprintf(pbuf, "+%02X%02X", _buf[p + 4], _buf[p + 5]);
        }
        else
        {
            sprintf(pbuf, " %02X%02X", _buf[p + 4], _buf[p + 5]);
        }
        break;
    case MQTTSN_PUBACK:
    case MQTTSN_REGISTER:
    case MQTTSN_REGACK:
        sprintf(pbuf, " %02X%02X", _buf[4], _buf[5]);
        break;
    case MQTTSN_PUBREC:
    case MQTTSN_PUBREL:
    case MQTTSN_PUBCOMP:
    case MQTTSN_UNSUBACK:
        sprintf(pbuf, " %02X%02X", _buf[2], _buf[3]);
        break;
    case MQTTSN_SUBSCRIBE:
    case MQTTSN_UNSUBSCRIBE:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        sprintf(pbuf, " %02X%02X", _buf[p + 2], _buf[p + 3]);
        break;
    case MQTTSN_SUBACK:
        sprintf(pbuf, " %02X%02X", _buf[5], _buf[6]);
        break;
    default:
        sprintf(pbuf, "    ");
        break;
    }
    if (strcmp(pbuf, " 0000") == 0)
    {
        sprintf(pbuf, "    ");
    }
    return pbuf;
}

int MQTTSNPacket::getMsgId(void)
{
    int value = 0;
    int p = 0;
    int msgId = 0;
    char* ptr = 0;

    switch (getType())
    {
    case MQTTSN_PUBLISH:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        ptr = (char*) _buf + p + 4;
        msgId = readInt((char**) &ptr);
        break;
    case MQTTSN_PUBACK:
    case MQTTSN_REGISTER:
    case MQTTSN_REGACK:
        ptr = (char*) _buf + 4;
        msgId = readInt((char**) &ptr);
        break;
    case MQTTSN_PUBREC:
    case MQTTSN_PUBREL:
    case MQTTSN_PUBCOMP:
    case MQTTSN_UNSUBACK:
        ptr = (char*) _buf + 2;
        msgId = readInt((char**) &ptr);
        break;
    case MQTTSN_SUBSCRIBE:
    case MQTTSN_UNSUBSCRIBE:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        ptr = (char*) _buf + p + 2;
        msgId = readInt((char**) &ptr);
        break;
    case MQTTSN_SUBACK:
        ptr = (char*) _buf + 5;
        msgId = readInt((char**) &ptr);
        break;
    default:
        break;
    }
    return msgId;
}

void MQTTSNPacket::setMsgId(uint16_t msgId)
{
    int value = 0;
    int p = 0;
    //unsigned char* ptr = 0;

    switch (getType())
    {
    case MQTTSN_PUBLISH:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        _buf[p + 4] = (unsigned char) (msgId / 256);
        _buf[p + 5] = (unsigned char) (msgId % 256);
        //ptr = _buf + p + 4;
        //writeInt(&ptr, msgId);
        break;
    case MQTTSN_PUBACK:
    case MQTTSN_REGISTER:
    case MQTTSN_REGACK:
        _buf[4] = (unsigned char) (msgId / 256);
        _buf[5] = (unsigned char) (msgId % 256);
        //ptr = _buf + 4;
        //writeInt(&ptr, msgId);
        break;
    case MQTTSN_PUBREC:
    case MQTTSN_PUBREL:
    case MQTTSN_PUBCOMP:
    case MQTTSN_UNSUBACK:
        _buf[2] = (unsigned char) (msgId / 256);
        _buf[3] = (unsigned char) (msgId % 256);
        //ptr = _buf + 2;
        //writeInt(&ptr, msgId);
        break;
    case MQTTSN_SUBSCRIBE:
    case MQTTSN_UNSUBSCRIBE:
        p = MQTTSNPacket_decode(_buf, _bufLen, &value);
        _buf[p + 2] = (unsigned char) (msgId / 256);
        _buf[p + 3] = (unsigned char) (msgId % 256);
        //ptr = _buf + p + 2;
        //writeInt(&ptr, msgId);
        break;
    case MQTTSN_SUBACK:
        _buf[5] = (unsigned char) (msgId / 256);
        _buf[6] = (unsigned char) (msgId % 256);
        //ptr = _buf + 5;
        //writeInt(&ptr, msgId);
        break;
    default:
        break;
    }
}

bool MQTTSNPacket::isDuplicate(void)
{
    int value = 0;
    int p = MQTTSNPacket_decode(_buf, _bufLen, &value);
    return (_buf[p + 1] & 0x80);
}
