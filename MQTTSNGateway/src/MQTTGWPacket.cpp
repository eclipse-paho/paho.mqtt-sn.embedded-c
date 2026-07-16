/**************************************************************************************
 * Copyright (c) 2009, 2026 IBM Corp., Ian Craggs, Tomoaki YAMAGUCHI
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Tomoaki Yamaguchi - modify codes for MATT-SN Gateway
 **************************************************************************************/

#include "MQTTGWPacket.h"
#include <string>
#include <string.h>

using namespace MQTTSNGW;

int readInt(char** ptr);
void writeInt(unsigned char** pptr, int msgId);

#define MAX_NO_OF_REMAINING_LENGTH_BYTES    3
/**
 * List of the predefined MQTT v3 packet names.
 */
static const char* mqtt_packet_names[] = { "RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL", "PUBCOMP",
        "SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT" };

/**
 * Encodes the message length according to the MQTT algorithm
 * @param buf the buffer into which the encoded data is written
 * @param length the length to be encoded
 * @return the number of bytes written to buffer
 */
int MQTTPacket_encode(char* buf, int length)
{
    int rc = 0;
    do
    {
        char d = length % 128;
        length /= 128;
        /* if there are more digits to encode, set the top bit of this digit */
        if (length > 0)
            d |= 0x80;
        buf[rc++] = d;
    }
    while (length > 0);
    return rc;
}

/**
 * Calculates an integer from two bytes read from the input buffer
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the integer value calculated
 */
int readInt(char** pptr)
{
    char* ptr = *pptr;
    int len = 256 * ((unsigned char) (*ptr)) + (unsigned char) (*(ptr + 1));
    *pptr += 2;
    return len;
}

/**
 * Reads a "UTF" string from the input buffer.  UTF as in the MQTT v3 spec which really means
 * a length delimited string.  So it reads the two byte length then the data according to
 * that length.  The end of the buffer is provided too, so we can prevent buffer overruns caused
 * by an incorrect length.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the buffer not to be read beyond
 * @param len returns the calculcated value of the length bytes read
 * @return an allocated C string holding the characters read, or NULL if the length read would
 * have caused an overrun.
 *
 */
char* readUTFlen(char** pptr, char* enddata, int* len)
{
    char* string = NULL;

    if (enddata - (*pptr) > 1) /* enough length to read the integer? */
    {
        *len = readInt(pptr);
        if (&(*pptr)[*len] <= enddata)
        {
            string = (char*) calloc(*len + 1, 1);
            memcpy(string, *pptr, (size_t) *len);
            string[*len] = '\0';
            *pptr += *len;
        }
    }
    return string;
}

/**
 * Reads a "UTF" string from the input buffer.  UTF as in the MQTT v3 spec which really means
 * a length delimited string.  So it reads the two byte length then the data according to
 * that length.  The end of the buffer is provided too, so we can prevent buffer overruns caused
 * by an incorrect length.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the buffer not to be read beyond
 * @return an allocated C string holding the characters read, or NULL if the length read would
 * have caused an overrun.
 */
char* readUTF(char** pptr, char* enddata)
{
    int len;
    return readUTFlen(pptr, enddata, &len);
}

/**
 * Reads one character from the input buffer.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the character read
 */
unsigned char readChar(char** pptr)
{
    unsigned char c = **pptr;
    (*pptr)++;
    return c;
}

/**
 * Writes one character to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param c the character to write
 */
void writeChar(unsigned char** pptr, char c)
{
    **pptr = c;
    (*pptr)++;
}

/**
 * Writes an integer as 2 bytes to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param anInt the integer to write
 */
void writeInt(unsigned char** pptr, int anInt)
{
    **pptr = (unsigned char) (anInt / 256);
    (*pptr)++;
    **pptr = (unsigned char) (anInt % 256);
    (*pptr)++;
}

/**
 * Writes a "UTF" string to an output buffer.  Converts C string to length-delimited.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param string the C string to write
 */
void writeUTF(unsigned char** pptr, const char* string)
{
    int len = (int) strlen(string);
    writeInt(pptr, len);
    memcpy(*pptr, string, (size_t) len);
    *pptr += len;
}

/**
 * Lapper class of MQTTPacket
 *
 */
MQTTGWPacket::MQTTGWPacket()
{
    _data = 0;
    _header.byte = 0;
    _remainingLength = 0;
}

MQTTGWPacket::~MQTTGWPacket()
{
    if (_data)
    {
        free(_data);
    }
}

int MQTTGWPacket::recv(Network* network)
{
    int len = 0;
    int multiplier = 1;
    unsigned char c;

    /* read First Byte of Packet */
    int rc = network->recv((unsigned char*) &_header.byte, 1);
    if (rc <= 0)
    {
        return rc;
    }
    /* read RemainingLength */
    do
    {
        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            return -2;
        }
        if (network->recv(&c, 1) == -1)
        {
            return -1;
        }
        _remainingLength += (c & 127) * multiplier;
        multiplier *= 128;
    }
    while ((c & 128) != 0);

    if (_remainingLength > 0)
    {
        /* allocate buffer */
        _data = (unsigned char*) calloc(_remainingLength, 1);
        if (!_data)
        {
            return -3;
        }

        /* read Payload */
        int remlen = network->recv(_data, _remainingLength);

        if (remlen == -1)
        {
            return -1;
        }
        else if (remlen != _remainingLength)
        {
            return -2;
        }
    }
    return 1 + len + _remainingLength;
}

int MQTTGWPacket::send(Network* network)
{
    unsigned char buf[MQTTSNGW_MAX_PACKET_SIZE];
    memset(buf, 0, MQTTSNGW_MAX_PACKET_SIZE);
    int len = getPacketData(buf);
    return network->send(buf, len);

}

int MQTTGWPacket::getAck(Ack* ack)
{
    if (PUBACK != _header.bits.type && PUBREC != _header.bits.type && PUBREL != _header.bits.type
            && PUBCOMP != _header.bits.type && UNSUBACK != _header.bits.type)
    {
        return 0;
    }
    char* ptr = (char*) _data;
    ack->header.byte = _header.byte;
    ack->msgId = readInt((char**) &ptr);
    return 1;
}

int MQTTGWPacket::getCONNACK(Connack* resp)
{
    if (_header.bits.type != CONNACK)
    {
        return 0;
    }
    char* ptr = (char*) _data;
    resp->header.byte = _header.byte;
    resp->flags.all = *ptr++;
    resp->rc = readChar(&ptr);
    return 1;
}

int MQTTGWPacket::getSUBACK(unsigned short* msgId, unsigned char* rc)
{
    if (_header.bits.type != SUBACK)
    {
        return 0;
    }
    char *ptr = (char*) _data;
    *msgId = readInt((char**) &ptr);
    *rc = readChar(&ptr);
    return 1;
}

int MQTTGWPacket::getPUBLISH(Publish* pub)
{
    if (_header.bits.type != PUBLISH)
    {
        return 0;
    }
    char* ptr = (char*) _data;
    pub->header = _header;
    pub->topiclen = readInt((char**) &ptr);
    pub->topic = (char*) _data + 2;
    ptr += pub->topiclen;
    if (_header.bits.qos > 0)
    {
        pub->msgId = readInt(&ptr);
        pub->payloadlen = _remainingLength - pub->topiclen - 4;
    }
    else
    {
        pub->msgId = 0;
        pub->payloadlen = _remainingLength - pub->topiclen - 2;
    }
    pub->payload = ptr;
    return 1;
}

int MQTTGWPacket::setCONNECT(Connect* connect, unsigned char* username, unsigned char* password)
{
    clearData();
    _header = connect->header;

    _remainingLength = ((connect->version == 3) ? 12 : 10) + (int) strlen(connect->clientID) + 2;
    if (connect->flags.bits.will)
    {
        _remainingLength += (int) strlen(connect->willTopic) + 2 + (int) strlen(connect->willMsg) + 2;
    }
    if (connect->flags.bits.username)
    {
        _remainingLength += (int) strlen((char*) username) + 2;
    }
    if (connect->flags.bits.password)
    {
        _remainingLength += (int) strlen((char*) password) + 2;
    }

    _data = (unsigned char*) calloc(_remainingLength, 1);
    unsigned char* ptr = _data;

    if (connect->version == 3)
    {
        writeUTF(&ptr, "MQIsdp");
        writeChar(&ptr, (char) 3);
    }
    else if (connect->version == 4)
    {
        writeUTF(&ptr, "MQTT");
        writeChar(&ptr, (char) 4);
    }
    else
    {
        return 0;
    }

    writeChar(&ptr, connect->flags.all);
    writeInt(&ptr, connect->keepAliveTimer);
    writeUTF(&ptr, connect->clientID);
    if (connect->flags.bits.will)
    {
        writeUTF(&ptr, connect->willTopic);
        writeUTF(&ptr, connect->willMsg);
    }

    if (connect->flags.bits.username)
    {
        writeUTF(&ptr, (const char*) username);
    }
    if (connect->flags.bits.password)
    {
        writeUTF(&ptr, (const char*) password);
    }
    return 1;
}

int MQTTGWPacket::setSUBSCRIBE(const char* topic, unsigned char qos, unsigned short msgId)
{
    clearData();
    _header.byte = 0;
    _header.bits.type = SUBSCRIBE;
    _header.bits.qos = 1;          // Reserved
    _remainingLength = (int) strlen(topic) + 5;
    _data = (unsigned char*) calloc(_remainingLength, 1);
    if (_data)
    {
        unsigned char* ptr = _data;
        writeInt(&ptr, msgId);
        writeUTF(&ptr, topic);
        writeChar(&ptr, (char) qos);
        return 1;
    }
    clearData();
    return 0;
}

int MQTTGWPacket::setUNSUBSCRIBE(const char* topic, unsigned short msgid)
{
    clearData();
    _header.byte = 0;
    _header.bits.type = UNSUBSCRIBE;
    _header.bits.qos = 1;
    _remainingLength = (int) strlen(topic) + 4;
    _data = (unsigned char*) calloc(_remainingLength, 1);
    if (_data)
    {
        unsigned char* ptr = _data;
        writeInt(&ptr, msgid);
        writeUTF(&ptr, topic);
        return 1;
    }
    clearData();
    return 0;

}

int MQTTGWPacket::setPUBLISH(Publish* pub)
{
    clearData();
    _header.byte = pub->header.byte;
    _header.bits.type = PUBLISH;
    _remainingLength = 4 + pub->topiclen + pub->payloadlen;
    _data = (unsigned char*) calloc(_remainingLength, 1);
    if (_data)
    {
        unsigned char* ptr = _data;
        writeInt(&ptr, pub->topiclen);
        memcpy(ptr, pub->topic, pub->topiclen);
        ptr += pub->topiclen;
        if (_header.bits.qos > 0)
        {
            writeInt(&ptr, pub->msgId);
        }
        else
        {
            _remainingLength -= 2;
        }
        memcpy(ptr, pub->payload, pub->payloadlen);
        return 1;
    }
    else
    {
        clearData();
        return 0;
    }
}

int MQTTGWPacket::setAck(unsigned char msgType, unsigned short msgid)
{
    clearData();
    _remainingLength = 2;
    _header.bits.type = msgType;
    _header.bits.qos = (msgType == PUBREL) ? 1 : 0;

    _data = (unsigned char*) calloc(_remainingLength, 1);
    if (_data)
    {
        unsigned char* data = _data;
        writeInt(&data, msgid);
        return 1;
    }
    return 0;
}

int MQTTGWPacket::setHeader(unsigned char msgType)
{
    clearData();
    if (msgType < CONNECT || msgType > DISCONNECT)
    {
        return 0;
    }
    _header.bits.type = msgType;
    return 0;
}

int MQTTGWPacket::getType(void)
{
    return _header.bits.type;
}

const char* MQTTGWPacket::getName(void)
{
    return getType() > DISCONNECT ? "UNKNOWN" : mqtt_packet_names[getType()];
}

int MQTTGWPacket::getPacketData(unsigned char* buf)
{
    unsigned char* ptr = buf;
    *ptr++ = _header.byte;
    int len = MQTTPacket_encode((char*) ptr, _remainingLength);
    ptr += len;
    memcpy(ptr, _data, _remainingLength);
    return 1 + len + _remainingLength;
}

int MQTTGWPacket::getPacketLength(void)
{
    char buf[4];
    return 1 + MQTTPacket_encode(buf, _remainingLength) + _remainingLength;
}

void MQTTGWPacket::clearData(void)
{
    if (_data)
    {
        free(_data);
    }
    _header.byte = 0;
    _remainingLength = 0;
}

char* MQTTGWPacket::getMsgId(char* pbuf)
{
    int type = getType();

    switch (type)
    {
    case PUBLISH:
        Publish pub;
        pub.msgId = 0;
        getPUBLISH(&pub);
        if (_header.bits.dup)
        {
            snprintf(pbuf, 6, "+%04X", pub.msgId);
        }
        else
        {
            snprintf(pbuf, 6, " %04X", pub.msgId);
        }
        break;
    case SUBSCRIBE:
    case UNSUBSCRIBE:
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP:
    case SUBACK:
    case UNSUBACK:
        snprintf(pbuf, 6, " %02X%02X", _data[0], _data[1]);
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

int MQTTGWPacket::getMsgId(void)
{
    int type = getType();
    int msgId = 0;

    switch (type)
    {
    case PUBLISH:
        Publish pub;
        pub.msgId = 0;
        getPUBLISH(&pub);
        msgId = pub.msgId;
        break;
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP:
    case SUBSCRIBE:
    case UNSUBSCRIBE:
    case SUBACK:
    case UNSUBACK:
        msgId = 256 * (unsigned char) _data[0] + (unsigned char) _data[1];
        break;
    default:
        break;
    }
    return msgId;
}

void MQTTGWPacket::setMsgId(int msgId)
{
    int type = getType();
    unsigned char* ptr = 0;

    switch (type)
    {
    case PUBLISH:
        Publish pub;
        pub.topiclen = 0;
        pub.msgId = 0;
        getPUBLISH(&pub);
        pub.msgId = msgId;
        ptr = _data + pub.topiclen;
        writeInt(&ptr, pub.msgId);
        *ptr++ = (unsigned char) (msgId / 256);
        *ptr = (unsigned char) (msgId % 256);
        break;
    case SUBSCRIBE:
    case UNSUBSCRIBE:
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP:
    case SUBACK:
    case UNSUBACK:
        ptr = _data;
        *ptr++ = (unsigned char) (msgId / 256);
        *ptr = (unsigned char) (msgId % 256);
        break;
    default:
        break;
    }
}

char* MQTTGWPacket::print(char* pbuf)
{
    uint8_t packetData[MQTTSNGW_MAX_PACKET_SIZE];
    char* ptr = pbuf;
    char** pptr = &pbuf;
    int len = getPacketData(packetData);
    int size = len > SIZE_OF_LOG_PACKET ? SIZE_OF_LOG_PACKET : len;
    for (int i = 0; i < size; i++)
    {
        snprintf(*pptr, 4, " %02X", packetData[i]);
        *pptr += 3;
    }
    **pptr = 0;
    return ptr;
}

MQTTGWPacket& MQTTGWPacket::operator =(MQTTGWPacket& packet)
{
    clearData();
    this->_header.byte = packet._header.byte;
    this->_remainingLength = packet._remainingLength;
    _data = (unsigned char*) calloc(_remainingLength, 1);
    if (_data)
    {
        memcpy(this->_data, packet._data, _remainingLength);
    }
    else
    {
        clearData();
    }
    return *this;
}

UTF8String MQTTGWPacket::getTopic(void)
{
    UTF8String str = { 0, nullptr };
    if (_header.bits.type == SUBSCRIBE || _header.bits.type == UNSUBSCRIBE)
    {
        char* ptr = (char*) (_data + 2);
        str.len = readInt(&ptr);
        str.data = (char*) (_data + 4);
    }
    return str;
}
