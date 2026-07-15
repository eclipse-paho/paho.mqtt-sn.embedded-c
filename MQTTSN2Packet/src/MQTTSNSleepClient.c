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
  * Serializes the supplied SLEEPREQ data into the supplied buffer, ready for
  * sending (client transmit side).
  *
  * @param buf                the buffer into which the packet will be serialized
  * @param buflen             the length in bytes of the supplied buffer
  * @param retainTopicAliases Retain Topic Aliases flag (Section 3.15.2.1)
  * @param packetid           Packet Identifier, used to match the SLEEPRESP
  *                           (Section 3.15.3)
  * @param duration           Sleep Duration in seconds; MUST be greater than 0
  *                           (Section 3.15.4)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_sleepreq(uint8_t* buf, int32_t buflen,
		uint8_t retainTopicAliases, uint16_t packetid, uint32_t duration)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	/* body: type(1) + flags(1) + packetid(2) + duration(4) = 8 bytes */
	if ((len = MQTTSNPacket_len(8u)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_SLEEPREQ);               /* write packet type */

	/* encode SLEEPREQ flags (Section 3.15.2):
	 *   bit  0    Retain Topic Aliases
	 *   bits 1-7  Reserved (must be 0)
	 */
	flags = (uint8_t)((retainTopicAliases & 0x01u) << SLEEPREQ_FLAGS_RETAIN_SHIFT);
	writeChar(&ptr, (char)flags);

	writeInt16(&ptr, packetid);
	writeInt32(&ptr, duration);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into SLEEPRESP data (client
  * receive side).
  *
  * @param durationPresent returned - Sleep Duration Flag (Section 3.16.2.1):
  *                        1 = *duration was present on the wire and overrides
  *                        the value the Client sent in SLEEPREQ, 0 = absent
  * @param packetid        returned - the Packet Identifier from the SLEEPREQ
  *                        being acknowledged
  * @param duration        returned - Sleep Duration in seconds; only
  *                        meaningful when *durationPresent is non-zero
  * @param returncode      returned - Reason Code (Section 3.16.5);
  *                        MQTT_SN_RC_SUCCESS assumed when absent from the wire
  * @param buf             the raw buffer data of the correct length
  * @param buflen          the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_sleepresp(uint8_t* durationPresent,
		uint16_t* packetid, uint32_t* duration, uint8_t* returncode,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  flags;
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
	if (!buf_avail(curdata, endbuffer, 4))              /* type(1)+flags(1)+packetId(2) */
		goto exit;

	if (readChar(&curdata) != MQTTSN_SLEEPRESP)
		goto exit;

	/* decode SLEEPRESP flags (Section 3.16.2):
	 *   bit  0    Sleep Duration Flag
	 *   bits 1-7  Reserved (must be 0)
	 */
	flags            = (uint8_t)readChar(&curdata);
	*durationPresent = (flags >> SLEEPRESP_FLAGS_DURATION_SHIFT) & 0x01u;

	*packetid = readInt16(&curdata);

	/* Sleep Duration: present only when the Sleep Duration Flag is set
	 * (Section 3.16.2.1)
	 */
	if (*durationPresent)
	{
		uint32_t sd;

		if (!buf_avail(curdata, endbuffer, 4))
			goto exit;
		sd  = (uint32_t)((uint8_t)readChar(&curdata)) << 24;
		sd |= (uint32_t)((uint8_t)readChar(&curdata)) << 16;
		sd |= (uint32_t)((uint8_t)readChar(&curdata)) << 8;
		sd |= (uint32_t)((uint8_t)readChar(&curdata));
		*duration = sd;
	}
	else
		*duration = 0;

	/* Reason Code is optional: infer its presence from remaining packet length
	 * (Section 3.16.5). If absent, 0x00 (Success) is assumed.
	 */
	if (curdata < enddata)
		*returncode = (uint8_t)readChar(&curdata);
	else
		*returncode = MQTT_SN_RC_SUCCESS;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer as a WAKEUP packet (client receive
  * side).
  *
  * The WAKEUP packet has no fields beyond the Length and Packet Type
  * (Section 3.14); this function only validates that the buffer contains a
  * well-formed WAKEUP packet.
  *
  * @param buf    the raw buffer data of the correct length
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_wakeup(uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
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
	if (!buf_avail(curdata, endbuffer, 1))              /* type(1) */
		goto exit;

	if (readChar(&curdata) != MQTTSN_WAKEUP)
		goto exit;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
