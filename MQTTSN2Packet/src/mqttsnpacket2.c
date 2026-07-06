/*******************************************************************************
 * Copyright (c) 2014, 2026 Ian Craggs
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
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

/*
 * Updated implementation of MQTTSNPacket helper functions for v2.0.
 *
 * Function name changes from v1.2:
 *   readInt()   → readInt16()   (returns uint16_t, advances by 2)
 *   writeInt()  → writeInt16()  (takes uint16_t)
 *   new:          writeInt32()  (takes uint32_t, advances by 4)
 *
 * Type changes:
 *   MQTTSNString  → MQTTSN_string  (len/data; no cstring/lenstring)
 *   int           → uint16_t/int32_t where appropriate
 *
 * Signature changes to MQTTSNPacket_len, MQTTSNPacket_encode:
 *   MQTTSNPacket_len    now takes and returns uint16_t
 *   MQTTSNPacket_encode now takes uint16_t length, returns int8_t
 *
 * New helpers:
 *   writeMQTTSNString(pptr, str, withLength) - writes len prefix when requested
 *   writeMQTTSNData(pptr, data, withLength)  - writes 2-byte len prefix when requested
 */

#include "StackTrace.h"
#include "MQTTSNPacket.h"

#include <string.h>
#include <stdbool.h>

static const char* packet_names[] =
{
	"RESERVED",
	"CONNECT", "CONNACK",
	"PUBLISH", "PUBACK", "PUBREC", "PUBREL", "PUBCOMP",
	"SUBSCRIBE", "SUBACK", "UNSUBSCRIBE", "UNSUBACK",
	"PINGREQ", "PINGRESP",
	"DISCONNECT",
	"AUTH", "REGISTER", "REGACK",
	"PUBWOS", "SLEEPREQ", "SLEEPRESP", "WAKEUP",
	"ADVERTISE", "SEARCHGW", "GWINFO",
	"FORWARDER", "SESSION", "PROTECTION"
};

/**
  * Returns a string name for a given packet type.
  * @param ptype the MQTT-SN packet type byte
  * @return the corresponding packet name, or "UNKNOWN"
  */
const char* MQTTSNPacket_name(uint8_t ptype)
{
	if (ptype == MQTTSN_FORWARDER || ptype == MQTTSN_SESSION ||
	    ptype == MQTTSN_PROTECTION)
	{
		return packet_names[25 + (ptype == MQTTSN_FORWARDER ? 0 :
		                          ptype == MQTTSN_SESSION    ? 1 : 2)];
	}
	return (ptype > 0 && ptype <= MQTTSN_GWINFO)
	       ? packet_names[ptype] : "UNKNOWN";
}


/**
  * Calculates the total packet length given the body length (excluding the
  * variable-length length field itself).
  *
  * @param length body length in bytes, not including the length field
  * @return total packet length including the length field
  */
uint16_t MQTTSNPacket_len(uint16_t length)
{
	/* length < 255: 1-byte length field; length >= 255: 3-byte length field */
	return (length >= 255u) ? length + 3u : length + 1u;
}


/**
  * Encodes the total packet length into the buffer.
  *
  * @param buf    the buffer at the start of the packet
  * @param length the TOTAL packet length (including this length field)
  * @return the number of bytes written (1 or 3)
  */
int8_t MQTTSNPacket_encode(unsigned char* buf, uint16_t length)
{
	int8_t rc;

	FUNC_ENTRY;
	if (length >= 255u)
	{
		*buf++ = 0x01u;
		*buf++ = (unsigned char)(length >> 8);
		*buf   = (unsigned char)(length & 0xFFu);
		rc = 3;
	}
	else
	{
		*buf = (unsigned char)length;
		rc = 1;
	}
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Decodes the MQTT-SN packet length from the start of a received buffer.
  *
  * @param buf    the buffer containing the received packet
  * @param buflen the number of bytes available in buf
  * @param value  the decoded total packet length returned here
  * @return the number of bytes consumed by the length field (1 or 3),
  *         or MQTTSNPACKET_READ_ERROR on error
  */
int MQTTSNPacket_decode(unsigned char* buf, int buflen, int* value)
{
	int len = MQTTSNPACKET_READ_ERROR;

	FUNC_ENTRY;
	if (buflen <= 0)
		goto exit;

	if (buf[0] == 0x01u)
	{
		if (buflen < 3)
			goto exit;
		*value = ((int)buf[1] << 8) | (int)buf[2];
		len = 3;
	}
	else
	{
		*value = (int)buf[0];
		len = 1;
	}
exit:
	FUNC_EXIT_RC(len);
	return len;
}


/**
  * Reads a 16-bit big-endian integer from the buffer.
  * @param pptr pointer to the input buffer — advanced by 2
  * @return the 16-bit value
  */
uint16_t readInt16(unsigned char** pptr)
{
	uint16_t val = (uint16_t)(((uint16_t)(uint8_t)(*pptr)[0] << 8) |
	                           (uint16_t)(uint8_t)(*pptr)[1]);
	*pptr += 2;
	return val;
}


/**
  * Reads one byte from the buffer.
  * @param pptr pointer to the input buffer — advanced by 1
  * @return the byte as a char
  */
char readChar(unsigned char** pptr)
{
	char c = (char)**pptr;
	(*pptr)++;
	return c;
}


/**
  * Writes one byte to the buffer.
  * @param pptr pointer to the output buffer — advanced by 1
  * @param c    the byte to write
  */
void writeChar(unsigned char** pptr, char c)
{
	**pptr = (unsigned char)c;
	(*pptr)++;
}


/**
  * Writes a 16-bit big-endian integer to the buffer.
  * @param pptr  pointer to the output buffer — advanced by 2
  * @param anInt the value to write
  */
void writeInt16(unsigned char** pptr, uint16_t anInt)
{
	*(*pptr)++ = (unsigned char)(anInt >> 8);
	*(*pptr)++ = (unsigned char)(anInt & 0xFFu);
}


/**
  * Writes a 32-bit big-endian integer to the buffer.
  * @param pptr  pointer to the output buffer — advanced by 4
  * @param anInt the value to write
  */
void writeInt32(unsigned char** pptr, uint32_t anInt)
{
	*(*pptr)++ = (unsigned char)((anInt >> 24) & 0xFFu);
	*(*pptr)++ = (unsigned char)((anInt >> 16) & 0xFFu);
	*(*pptr)++ = (unsigned char)((anInt >>  8) & 0xFFu);
	*(*pptr)++ = (unsigned char)( anInt        & 0xFFu);
}


/**
  * Reads a string from the buffer as a zero-copy pointer.
  * The string is assumed to fill to enddata (no inline length prefix).
  *
  * @param mqttstring the MQTTSN_string to fill
  * @param pptr       pointer to the input buffer — advanced to enddata
  * @param enddata    pointer to the byte past the last valid data byte
  * @return 1 on success
  */
int readMQTTSNString(MQTTSN_string* mqttstring, unsigned char** pptr,
                     unsigned char* enddata)
{
	mqttstring->len  = (uint16_t)(enddata - *pptr);
	mqttstring->data = (mqttstring->len > 0) ? (char*)*pptr : NULL;
	*pptr = enddata;
	return 1;
}


/**
  * Writes an MQTTSN_string to the buffer.
  *
  * @param pptr       pointer to the output buffer — advanced by bytes written
  * @param mqttstring the string to write
  * @param withLength if true, writes the length prefix before the data:
  *                   1-byte prefix when mqttstring.islen8 is set,
  *                   2-byte prefix otherwise
  */
void writeMQTTSNString(unsigned char** pptr, MQTTSN_string mqttstring,
                       bool withLength)
{
	if (withLength)
	{
		if (mqttstring.islen8)
			writeChar(pptr, (char)(uint8_t)mqttstring.len);
		else
			writeInt16(pptr, mqttstring.len);
	}
	if (mqttstring.len > 0 && mqttstring.data != NULL)
	{
		memcpy(*pptr, mqttstring.data, mqttstring.len);
		*pptr += mqttstring.len;
	}
}


/**
  * Writes an MQTTSN_data field to the buffer.
  *
  * @param pptr       pointer to the output buffer — advanced by bytes written
  * @param data       the binary data to write
  * @param withLength if true, writes a 2-byte big-endian length prefix first
  */
void writeMQTTSNData(unsigned char** pptr, MQTTSN_data data, bool withLength)
{
	if (withLength)
		writeInt16(pptr, data.len);
	if (data.len > 0 && data.data != NULL)
	{
		memcpy(*pptr, data.data, data.len);
		*pptr += data.len;
	}
}


/**
  * Reads a packet into buf using a caller-supplied read function.
  */
int MQTTSNPacket_read(unsigned char* buf, int buflen,
                      int (*getfn)(unsigned char*, int))
{
	int rc = MQTTSNPACKET_READ_ERROR;
	int len = 0;
	int mylen = 0;

	if ((len = (*getfn)(buf, buflen)) < 2)
		goto exit;
	if (MQTTSNPacket_decode(buf, len, &mylen) < 0)
		goto exit;
	if (mylen != len)
		goto exit;
	rc = (int)(uint8_t)buf[(buf[0] == 0x01u) ? 3 : 1];
exit:
	return rc;
}


/**
  * Parses a packet type from an already-read buffer.
  */
int MQTTSNPacket_read_nb(unsigned char* buf, int buflen)
{
	int rc = MQTTSNPACKET_READ_ERROR;
	int mylen = 0;

	if (MQTTSNPacket_decode(buf, buflen, &mylen) < 0)
		goto exit;
	if (mylen != buflen)
		goto exit;
	rc = (int)(uint8_t)buf[(buf[0] == 0x01u) ? 3 : 1];
exit:
	return rc;
}
