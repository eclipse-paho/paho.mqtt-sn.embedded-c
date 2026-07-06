/*******************************************************************************
 * Copyright (c) 2014, 2026 Ian Craggs, IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial implementation based on MQTT-SN 1.0 code
 *******************************************************************************/

#include "StackTrace.h"
#include "MQTTSNPacket.h"

#include <string.h>

static const char* packet_names[] =
{
		"RESERVED", "CONNECT", "CONNACK", "PUBLISH", "PUBACK", "PUBREC", "PUBREL", "PUBCOMP",
		"SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP", "DISCONNECT",
		"AUTH", "REGISTER", "REGACK", "PUBWOS", "SLEEPREQ", "SLEEPRESP", "WAKEUP",
		"ADVERTISE", "SEARCHGW", "GWINFO"
};

static const char* forwarder_packet_name = "FORWARDER_ENCAPSULATED";
static const char* session_packet_name = "SESSION_ENCAPSULATED";
static const char* protection_packet_name = "PROTECTION_ENCAPSULATED";

/**
 * Returns a character string representing the packet name given a Packet Type
 * @param packet_type numerical packet type
 * @return the corresponding packet name
 */
const char* MQTTSNPacket_name(uint8_t packet_type)
{
	const char* name = NULL;

	switch(packet_type)
	{
    case MQTTSN_FORWARDER:
        name = forwarder_packet_name;
	case MQTTSN_SESSION:
		name = session_packet_name;
	case MQTTSN_PROTECTION:
		name = protection_packet_name;
    default:
		name = (packet_type >= 0 && packet_type <= MQTTSN_GWINFO) ? packet_names[packet_type] : "UNKNOWN";
	}
	return name;
}


/**
 * Calculates the full packet length including length field
 * @param length the length of the MQTT-SN packet without the length field
 * @return the total length of the MQTT-SN packet including the length field
 */
uint16_t MQTTSNPacket_len(uint16_t length)
{
	return (length >= 255) ? length + 3 : length + 1;
}

/**
 * Encodes the MQTT-SN message length
 * @param buf the buffer into which the encoded data is written
 * @param length the length to be encoded
 * @return the number of bytes written to the buffer
 */
int8_t MQTTSNPacket_encode(unsigned char* buf, uint16_t length)
{
	int bytes_written = 0;

	FUNC_ENTRY;
	if (length >= 255)
	{
		writeChar(&buf, 0x01);
		writeInt16(&buf, length);
		bytes_written += 3;
	}
	else
		buf[bytes_written++] = length;

	FUNC_EXIT_RC(bytes_written);
	return bytes_written;
}


/**
 * Obtains the MQTT-SN packet length from received data
 * @param buf the buffer that contains the MQTT-SN packet
 * @param buflen the length in bytes of the supplied buffer
 * @param value the decoded length returned
 * @return the number of bytes read from the socket, <0 on error
 */
int MQTTSNPacket_decode(unsigned char* buf, int buflen, int* value)
{
	int len = MQTTSNPACKET_READ_ERROR;
#define MAX_NO_OF_LENGTH_BYTES 3

	FUNC_ENTRY;
	if (buflen <= 0)
		goto exit;

	if (buf[0] == 1)
	{
		unsigned char* bufptr = &buf[1];
		if (buflen < MAX_NO_OF_LENGTH_BYTES)
			goto exit;
		*value = readInt16(&bufptr);
		len = 3;
	}
	else
	{
		*value = buf[0];
		len = 1;
	}
exit:
	FUNC_EXIT_RC(len);
	return len;
}


/**
 * Calculates an integer from two bytes read from the input buffer
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the integer value calculated
 */
uint16_t readInt16(unsigned char** pptr)
{
	unsigned char* ptr = *pptr;
	uint16_t len = 256*((unsigned char)(*ptr)) + (unsigned char)(*(ptr+1));
	*pptr += 2;
	return len;
}


/**
 * Reads one character from the input buffer.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the character read
 */
char readChar(unsigned char** pptr)
{
	char c = **pptr;
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
	**pptr = (unsigned char)c;
	(*pptr)++;
}


/**
 * Writes an integer as 2 bytes to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param anInt the integer to write: 0 to 65535
 */
void writeInt8(unsigned char** pptr, uint8_t anInt)
{
	**pptr = (unsigned char)(anInt);
	(*pptr)++;
}


/**
 * Writes an integer as 2 bytes to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param anInt the integer to write: 0 to 65535
 */
void writeInt16(unsigned char** pptr, uint16_t anInt)
{
	**pptr = (unsigned char)(anInt / 256);
	(*pptr)++;
	**pptr = (unsigned char)(anInt % 256);
	(*pptr)++;
}


/**
 * Writes an integer as 4 bytes to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param anInt the integer to write
 */
void writeInt32(uint8_t** pptr, uint32_t anInt)
{
	**pptr = (uint8_t)(anInt / 16777216);
	(*pptr)++;
	anInt %= 16777216;
	**pptr = (uint8_t)(anInt / 65536);
	(*pptr)++;
	anInt %= 65536;
	**pptr = (uint8_t)(anInt / 256);
	(*pptr)++;
	**pptr = (uint8_t)(anInt % 256);
	(*pptr)++;
}


/**
 * Writes an MQTT-SN string to an output buffer for network transmission.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param mqttsnstring the MQTT-SN string to write
 */
void writeMQTTSNString(unsigned char** pptr, MQTTSN_string mqttsnstring, bool withLength)
{
	if (withLength)
		writeInt16(pptr, mqttsnstring.len);
	if (mqttsnstring.len > 0) /* if len is 0 then this is a noop */
	{
		memcpy(*pptr, (const char*)mqttsnstring.data, mqttsnstring.len);
		*pptr += mqttsnstring.len;
	}
}


void writeMQTTSNData(unsigned char** pptr, MQTTSN_data mqttsndata, bool withLength)
{
	if (withLength)
		writeInt16(pptr, mqttsndata.len);
	if (mqttsndata.len > 0)
	{
		memcpy(*pptr, (const unsigned char*)mqttsndata.data, mqttsndata.len);
		*pptr += mqttsndata.len;
	}
}


/**
 * @param MQTTSNString the MQTTSNString structure into which the data is to be read
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the data: do not read beyond
 * @return 1 if successful, 0 if not
 */
int readMQTTSNString(MQTTSN_string* MQTTSNString, unsigned char** pptr, unsigned char* enddata)
{
	int rc = 0;

	FUNC_ENTRY;
	MQTTSNString->len = enddata - *pptr;
	if (MQTTSNString->len > 0)
	{
		MQTTSNString->data = (char*)*pptr;
		*pptr += MQTTSNString->len;
	}
	else
		MQTTSNString->data = NULL;
	rc = 1;
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
 * Helper function to read packet data from some source into a buffer
 * @param buf the buffer into which the packet will be serialized
 * @param buflen the length in bytes of the supplied buffer
 * @param getfn pointer to a function which will read any number of bytes from the needed source
 * @return integer MQTT packet type, or MQTTSNPACKET_READ_ERROR on error
 */
int MQTTSNPacket_read(unsigned char* buf, int buflen, int (*getfn)(unsigned char*, int))
{
	int rc = MQTTSNPACKET_READ_ERROR;
	const int MQTTSN_MIN_PACKET_LENGTH = 2;
	int len = 0;  /* the length of the whole packet including length field */
	int lenlen = 0;
	int datalen = 0;

	/* 1. read a packet - UDP style */
	if ((len = (*getfn)(buf, buflen)) < MQTTSN_MIN_PACKET_LENGTH)
		goto exit;

	/* 2. read the length.  This is variable in itself */
	lenlen = MQTTSNPacket_decode(buf, len, &datalen);
	if (datalen != len)
		goto exit; /* there was an error */

	rc = buf[lenlen]; /* return the packet type */
exit:
	return rc;
}


