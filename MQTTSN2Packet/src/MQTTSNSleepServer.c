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

#include "MQTTSNPacket.h"
#include "MQTTSNSleep.h"
#include "StackTrace.h"

/*
 * MQTT-SN 2.0 SLEEPREQ Flags byte (Section 3.15.2)
 *
 * Bit 0   : Retain Topic Aliases
 * Bits 7-1: Reserved (must be 0)
 */
#define SLEEPREQ_FLAGS_RETAIN_SHIFT   0

/*
 * MQTT-SN 2.0 SLEEPRESP Flags byte (Section 3.16.2)
 *
 * Bit 0   : Sleep Duration Flag
 * Bits 7-1: Reserved (must be 0)
 */
#define SLEEPRESP_FLAGS_DURATION_SHIFT   0


/**
  * Deserializes the supplied (wire) buffer into SLEEPREQ data (server
  * receive side).
  *
  * @param retainTopicAliases returned - Retain Topic Aliases flag
  *                           (Section 3.15.2.1)
  * @param packetid           returned - Packet Identifier, used to match the
  *                           SLEEPRESP (Section 3.15.3)
  * @param duration           returned - Sleep Duration in seconds
  *                           (Section 3.15.4)
  * @param buf                the raw buffer data of the correct length
  * @param buflen             the length in bytes of the data in the supplied
  *                           buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_sleepreq(uint8_t* retainTopicAliases,
		uint16_t* packetid, uint32_t* duration,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  flags;
	uint8_t  *curdata = buf;
	uint8_t  *endbuffer = buf + buflen;
	int32_t  rc = 0;
	int32_t  mylen = 0;
	int32_t  lenlen;
	uint32_t sd;

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
	/* type(1) + flags(1) + packetid(2) + duration(4) = 8 bytes */
	if (!buf_avail(curdata, endbuffer, 8))
		goto exit;

	if (readChar(&curdata) != MQTTSN_SLEEPREQ)
		goto exit;

	/* decode SLEEPREQ flags (Section 3.15.2):
	 *   bit  0    Retain Topic Aliases
	 *   bits 1-7  Reserved (must be 0)
	 */
	flags                = (uint8_t)readChar(&curdata);
	*retainTopicAliases  = (flags >> SLEEPREQ_FLAGS_RETAIN_SHIFT) & 0x01u;

	*packetid = readInt16(&curdata);

	sd  = (uint32_t)((uint8_t)readChar(&curdata)) << 24;
	sd |= (uint32_t)((uint8_t)readChar(&curdata)) << 16;
	sd |= (uint32_t)((uint8_t)readChar(&curdata)) << 8;
	sd |= (uint32_t)((uint8_t)readChar(&curdata));
	*duration = sd;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes the supplied SLEEPRESP data into the supplied buffer, ready for
  * sending (server transmit side).
  *
  * @param buf             the buffer into which the packet will be serialized
  * @param buflen          the length in bytes of the supplied buffer
  * @param durationPresent 1 = include the Sleep Duration field (Section
  *                        3.16.2.1), 0 = omit it
  * @param packetid        Packet Identifier matching the SLEEPREQ being
  *                        acknowledged (Section 3.16.3)
  * @param duration        Sleep Duration in seconds (Section 3.16.4); written
  *                        only when durationPresent is 1
  * @param returncode      Reason Code: MQTT_SN_RC_SUCCESS or a failure code
  *                        >= 0x80 (see enum MQTTSN_reasonCodes in
  *                        MQTTSNPacket.h); omitted from the wire when
  *                        MQTT_SN_RC_SUCCESS (Section 3.16.5)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_sleepresp(uint8_t* buf, int32_t buflen,
		uint8_t durationPresent, uint16_t packetid, uint32_t duration,
		uint8_t returncode)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = 0;

	/* fixed fields: type(1) + flags(1) + packetid(2) = 4 bytes
	 * optional:     duration(4) + reasoncode(1)
	 */
	int32_t  bodylen = 4
	                 + (durationPresent ? 4 : 0)
	                 + (returncode != MQTT_SN_RC_SUCCESS ? 1 : 0);

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)bodylen)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_SLEEPRESP);              /* write packet type */

	/* encode SLEEPRESP flags (Section 3.16.2):
	 *   bit  0    Sleep Duration Flag
	 *   bits 1-7  Reserved (must be 0)
	 */
	flags = (uint8_t)((durationPresent & 0x01u) << SLEEPRESP_FLAGS_DURATION_SHIFT);
	writeChar(&ptr, (char)flags);

	writeInt16(&ptr, packetid);

	if (durationPresent)
		writeInt32(&ptr, duration);

	/* Reason Code is optional: omit when success (Section 3.16.5) */
	if (returncode != MQTT_SN_RC_SUCCESS)
		writeChar(&ptr, (char)returncode);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a WAKEUP packet into the supplied buffer, ready for sending
  * (server transmit side).
  *
  * The WAKEUP packet has no fields beyond the Length and Packet Type
  * (Section 3.14); the Client is not obliged to respond.
  *
  * @param buf    the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_wakeup(uint8_t* buf, int32_t buflen)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	/* body: type(1) = 1 byte */
	if ((len = MQTTSNPacket_len(1u)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_WAKEUP);                 /* write packet type */

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
