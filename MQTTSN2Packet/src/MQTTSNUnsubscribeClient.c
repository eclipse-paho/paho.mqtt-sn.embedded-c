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
 * MQTTSNSerialize_unsubscribe
 *   - MQTTSN_topicid replaced by MQTTSN_topic with alt.alias / alt.string
 *   - MQTTSNFlags union replaced by explicit bit construction; only bits 0-1
 *     (Topic Type) are defined — bits 7-2 are reserved (Section 3.9.2)
 *   - SHORT topic type removed; Session Topic Alias (MQTTSN_TOPIC_TYPE_SESSION)
 *     and Predefined Topic Alias (MQTTSN_TOPIC_TYPE_PREDEFINED) both use
 *     alt.alias; Topic Filter/Name type uses alt.string fills to end of packet
 *   - memcpy for the topic name replaced by writeMQTTSNString(..., false)
 *   - writeInt() → writeInt16()
 *   - MQTTSNSerialize_unsubscribeLength made static
 *   - All types updated to uint8_t / uint16_t / int32_t
 *   - #include <string.h> removed
 *
 * MQTTSNDeserialize_unsuback
 *   - returncode output parameter added: UNSUBACK carries an optional Reason
 *     Code in v2.0 (Section 3.10.3); defaults to MQTT_SN_RC_SUCCESS when absent
 *   - Note: UNSUBACK has NO flags byte in v2.0 — the format is simply
 *     length + type + packetid(2) + [reasoncode(1, optional)]
 *   - readInt() → readInt16()
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
  * Determines the body length of the MQTTSN UNSUBSCRIBE packet that would be
  * produced using the supplied parameters, excluding the length field itself.
  * @param topic the MQTTSN_topic describing the topic type and value
  * @return the body length in bytes
  */
static int32_t MQTTSNSerialize_unsubscribeLength(MQTTSN_topic* topic)
{
	/* type(1) + flags(1) + packetid(2) = 4 */
	int32_t len = 4;

	if (topic->type == MQTTSN_TOPIC_TYPE_FILTER || topic->type == MQTTSN_TOPIC_TYPE_NAME)
		len += topic->alt.string.len;   /* variable-length topic filter/name */
	else /* MQTTSN_TOPIC_TYPE_SESSION or MQTTSN_TOPIC_TYPE_PREDEFINED */
		len += 2;                       /* 2-byte topic alias */

	return len;
}


/**
  * Serializes the supplied unsubscribe data into the supplied buffer, ready for
  * sending.
  *
  * @param buf      the buffer into which the packet will be serialized
  * @param buflen   the length in bytes of the supplied buffer
  * @param packetid the MQTT-SN Packet Identifier
  * @param topic    MQTTSN_topic describing the topic type and value:
  *                 - FILTER / NAME: alt.string contains the topic filter string,
  *                   which fills to the end of the packet (no length prefix)
  *                 - SESSION / PREDEFINED: alt.alias contains the 2-byte alias
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_unsubscribe(uint8_t* buf, int32_t buflen,
		uint16_t packetid, MQTTSN_topic* topic)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(
	             (uint16_t)MQTTSNSerialize_unsubscribeLength(topic))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_UNSUBSCRIBE);            /* write packet type */

	/* encode UNSUBSCRIBE flags (Section 3.9.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-7  Reserved (must be 0)
	 */
	flags = (uint8_t)((topic->type & 0x03u) << UNSUBSCRIBE_FLAGS_TOPICTYPE_SHIFT);
	writeChar(&ptr, (char)flags);

	writeInt16(&ptr, packetid);

	/* write topic alias or topic filter/name string according to topic type */
	if (topic->type == MQTTSN_TOPIC_TYPE_FILTER || topic->type == MQTTSN_TOPIC_TYPE_NAME)
		writeMQTTSNString(&ptr, topic->alt.string, false); /* fills to end of packet */
	else /* MQTTSN_TOPIC_TYPE_SESSION or MQTTSN_TOPIC_TYPE_PREDEFINED */
		writeInt16(&ptr, topic->alt.alias);                /* 2-byte topic alias */

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into UNSUBACK data (client receive
  * side).
  *
  * Changes from MQTT-SN 1.2:
  *   - returncode output parameter added: UNSUBACK now carries an optional
  *     Reason Code (Section 3.10.3); defaults to MQTT_SN_RC_SUCCESS when absent
  *   - UNSUBACK has NO flags byte in v2.0; the wire format is simply:
  *     length + type + packetid(2) + [reasoncode(1, optional)]
  *   - readInt() → readInt16()
  *
  * @param packetid   returned - the Packet Identifier from the UNSUBSCRIBE
  * @param returncode returned - Reason Code (Section 2.3):
  *                   0x00 = Success, >= 0x80 = failure;
  *                   MQTT_SN_RC_SUCCESS assumed when absent from the wire
  * @param buf        the raw buffer data of the correct length
  * @param buflen     the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_unsuback(uint16_t* packetid, uint8_t* returncode,
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
	if (!buf_avail(curdata, endbuffer, 3))              /* type(1) + packetId(2) */
		goto exit;

	if (readChar(&curdata) != MQTTSN_UNSUBACK)
		goto exit;

	*packetid = readInt16(&curdata);

	/* Reason Code is optional: infer its presence from remaining packet length
	 * (Section 3.10.3). If absent, 0x00 (Success) is assumed.
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
