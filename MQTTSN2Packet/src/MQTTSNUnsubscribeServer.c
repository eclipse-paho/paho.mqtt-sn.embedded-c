/*******************************************************************************
* Copyright (c) 2014, 2026 IBM Corp., Ian Craggs
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
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

/*
 * Changes from MQTT-SN 1.2:
 *
 * MQTTSNDeserialize_unsubscribe
 *   - MQTTSN_topicid replaced by MQTTSN_topic with alt.alias / alt.string
 *   - MQTTSNFlags union replaced by explicit bit extraction; only bits 0-1
 *     (Topic Type) are defined in the UNSUBSCRIBE flags (Section 3.9.2)
 *   - SHORT topic type removed; SESSION and PREDEFINED both populate alt.alias
 *   - NORMAL type renamed: FILTER/NAME (0b11) populates alt.string zero-copy
 *   - readInt() → readInt16()
 *   - All types updated to uint8_t / uint16_t / int32_t
 *
 * MQTTSNSerialize_unsuback
 *   - returncode parameter added: UNSUBACK carries an optional Reason Code in
 *     v2.0 (Section 3.10.3); omitted from the wire when MQTT_SN_RC_SUCCESS
 *   - Note: UNSUBACK has NO flags byte — the wire format is simply
 *     length + type + packetid(2) + [reasoncode(1, optional)]
 *   - Hardcoded body length 3 replaced by dynamic bodylen calculation
 *   - writeInt() → writeInt16()
 *   - All types updated
 */

#include "MQTTSNPacket.h"
#include "MQTTSNSubscribe.h"
#include "StackTrace.h"

/*
 * MQTT-SN 2.0 UNSUBSCRIBE Flags byte (Section 3.9.2)
 *
 * Bits 7-2: Reserved (must be 0)
 * Bits 1-0: Topic Type
 */
#define UNSUBSCRIBE_FLAGS_TOPICTYPE_SHIFT   0


/**
  * Deserializes the supplied (wire) buffer into UNSUBSCRIBE data (server
  * receive side).
  *
  * @param packetid   returned - the Packet Identifier
  * @param topic      returned - topic type plus alias (alt.alias) or filter
  *                   string (alt.string, zero-copy pointer into buf)
  * @param buf        the raw buffer data of the correct length
  * @param buflen     the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_unsubscribe(uint16_t* packetid,
		MQTTSN_topic* topic, uint8_t* buf, int32_t buflen)
{
	uint8_t  flags;
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	int32_t lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
		goto exit;
	curdata += lenlen;
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_UNSUBSCRIBE)
		goto exit;

	/* decode UNSUBSCRIBE flags (Section 3.9.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-7  Reserved (must be 0)
	 */
	flags       = (uint8_t)readChar(&curdata);
	topic->type = (MQTTSN_topicTypes)((flags >> UNSUBSCRIBE_FLAGS_TOPICTYPE_SHIFT) & 0x03u);

	*packetid = readInt16(&curdata);

	/* read topic alias or topic filter/name string according to topic type */
	if (topic->type == MQTTSN_TOPIC_TYPE_FILTER || topic->type == MQTTSN_TOPIC_TYPE_NAME)
	{
		/* zero-copy: point directly into the receive buffer */
		topic->alt.string.len  = (uint16_t)(enddata - curdata);
		topic->alt.string.data = (char*)curdata;
	}
	else /* MQTTSN_TOPIC_TYPE_SESSION or MQTTSN_TOPIC_TYPE_PREDEFINED */
		topic->alt.alias = readInt16(&curdata); /* 2-byte topic alias */

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes the supplied UNSUBACK data into the supplied buffer, ready for
  * sending (server transmit side).
  *
  * Changes from MQTT-SN 1.2:
  *   - returncode added: Reason Code is now supported on UNSUBACK (Section
  *     3.10.3) and omitted from the wire when MQTT_SN_RC_SUCCESS
  *   - UNSUBACK has NO flags byte — the wire format is simply:
  *     length + type + packetid(2) + [reasoncode(1, optional)]
  *
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param packetid   the Packet Identifier from the UNSUBSCRIBE being
  *                   acknowledged
  * @param returncode Reason Code: MQTT_SN_RC_SUCCESS or a failure code >= 0x80
  *                   (see enum MQTTSN_reasonCodes in MQTTSNPacket.h);
  *                   omitted from the wire when MQTT_SN_RC_SUCCESS
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_unsuback(uint8_t* buf, int32_t buflen,
		uint16_t packetid, uint8_t returncode)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	/* body: type(1) + packetid(2) + [reasoncode(1, if not success)] */
	int32_t  bodylen = 3 + (returncode != MQTT_SN_RC_SUCCESS ? 1 : 0);

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)bodylen)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_UNSUBACK);               /* write packet type */

	writeInt16(&ptr, packetid);

	/* Reason Code is optional: omit when success (Section 3.10.3) */
	if (returncode != MQTT_SN_RC_SUCCESS)
		writeChar(&ptr, (char)returncode);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
