/*******************************************************************************
 * Copyright (c) 2026 Ian Craggs
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Added for MQTT-SN 2.0 (Committee Specification Draft 02, February 2026)
 *******************************************************************************/

/*
 * Serialization and deserialization of the MQTT-SN 2.0 AUTH packet
 * (Section 3.3, Figure 8).
 *
 * AUTH is exchanged in both directions (Client to Server and Server to
 * Client) as part of an authentication handshake started by CONNECT, so
 * both the serializer and deserializer live in this one file rather than
 * being split across Client/Server sources.
 *
 * Wire format (Figure 8):
 *
 *   Length                    (1 or 3 bytes - variable, covers the entire packet)
 *   Packet Type               (1 byte  - MQTTSN_AUTH)
 *   Packet Identifier         (2 bytes - MSB, LSB) (Section 3.3.2)
 *   Reason Code               (1 byte)             (Section 3.3.3)
 *   Authentication Method Len (1 byte)             (Section 3.3.4)
 *   Authentication Method     (M bytes)            (Section 3.3.5)
 *   Authentication Data       (fills to end of packet) (Section 3.3.6)
 */

#include "MQTTSNPacket.h"   /* MQTTSN_string, MQTTSN_data, readInt16, ... */
#include "MQTTSNAuth.h"
#include "StackTrace.h"


/**
  * Determines the length of the MQTT-SN AUTH packet body, excluding the
  * length field itself.
  * @param authMethod the Authentication Method string
  * @param authData   the Authentication Data
  * @return the length in bytes of the packet body
  */
static int32_t MQTTSNSerialize_authLength(const MQTTSN_string* authMethod,
        const MQTTSN_data* authData)
{
	/* type(1) + packetid(2) + reasonCode(1) + methodLen(1) + method + data */
	return 5 + authMethod->len + authData->len;
}


/**
  * Serializes the supplied AUTH data into the supplied buffer, ready for sending.
  *
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param packetid   identifies the corresponding CONNECT or AUTH packet
  *                   (Section 3.3.2)
  * @param reasonCode Authentication Reason Code (Section 3.3.3)
  * @param authMethod the Authentication Method name; written with a 1-byte
  *                   length prefix (Sections 3.3.4-3.3.5)
  * @param authData   the Authentication Data; fills to the end of the packet,
  *                   no length prefix (Section 3.3.6)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_auth(uint8_t* buf, int32_t buflen,
		uint16_t packetid, uint8_t reasonCode,
		const MQTTSN_string* authMethod, const MQTTSN_data* authData)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(
	             (uint16_t)MQTTSNSerialize_authLength(authMethod, authData))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_AUTH);                   /* write packet type */

	writeInt16(&ptr, packetid);                     /* Section 3.3.2 */
	writeChar(&ptr, (char)reasonCode);              /* Section 3.3.3 */

	/* Authentication Method: 1-byte length prefix then the string
	 * (Sections 3.3.4-3.3.5)
	 */
	writeChar(&ptr, (char)(uint8_t)authMethod->len);
	writeMQTTSNString(&ptr, *authMethod, false);

	/* Authentication Data fills to end of packet; no length prefix (Section 3.3.6) */
	writeMQTTSNData(&ptr, *authData, false);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into AUTH data.
  *
  * @param packetid   returned - identifies the corresponding CONNECT or AUTH
  *                   packet (Section 3.3.2)
  * @param reasonCode returned - Authentication Reason Code (Section 3.3.3)
  * @param authMethod returned - the Authentication Method name (zero-copy
  *                   pointer into buf) (Sections 3.3.4-3.3.5)
  * @param authData   returned - the Authentication Data; fills to the end of
  *                   the packet (zero-copy pointer into buf) (Section 3.3.6)
  * @param buf        the raw buffer data of the correct length
  * @param buflen     the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_auth(uint16_t* packetid, uint8_t* reasonCode,
		MQTTSN_string* authMethod, MQTTSN_data* authData,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	uint8_t  *endbuffer = buf + buflen;
	int32_t  rc = 0;
	int32_t  mylen = 0;
	int32_t  lenlen;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
	{
		rc = MQTTSNPACKET_READ_ERROR;
		goto exit;
	}
	if (buflen < mylen)              /* packet longer than the supplied buffer */
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	curdata += lenlen;
	enddata = buf + mylen;
	/* type(1) + packetid(2) + reasonCode(1) + methodLen(1) */
	if (!buf_avail(curdata, endbuffer, 5))
		goto exit;

	if (readChar(&curdata) != MQTTSN_AUTH)
		goto exit;

	*packetid   = readInt16(&curdata);              /* Section 3.3.2 */
	*reasonCode = (uint8_t)readChar(&curdata);       /* Section 3.3.3 */

	/* Authentication Method: 1-byte length prefix then string
	 * (Sections 3.3.4-3.3.5)
	 */
	authMethod->islen8 = 1;
	authMethod->len    = (uint16_t)(uint8_t)readChar(&curdata);
	if (!buf_avail(curdata, endbuffer, (int32_t)authMethod->len))
		goto exit;
	authMethod->data = (char*)curdata;
	curdata += authMethod->len;

	/* Authentication Data fills to end of packet; zero-copy (Section 3.3.6) */
	authData->len  = (uint16_t)(enddata - curdata);
	authData->data = curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
