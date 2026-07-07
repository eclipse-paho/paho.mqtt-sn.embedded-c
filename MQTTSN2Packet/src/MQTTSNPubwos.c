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
 * Serialization and deserialization of the MQTT-SN 2.0 PUBWOS packet
 * (Publish Without Session, Section 3.6.1, Figure 11).
 *
 * PUBWOS is the 2.0 successor to the MQTT-SN 1.2 "QoS -1" publish: a
 * fire-and-forget publish requiring no CONNECT/Session and no
 * acknowledgement, now a distinct packet type (0x12) rather than a PUBLISH
 * carrying a reserved QoS value. It is exchanged in both directions
 * (Client to Server and Server to Client), so both the serializer and
 * deserializer live in this one file rather than being split across
 * Client/Server sources.
 *
 * Wire format (Figure 11):
 *
 *   Length                          (1 or 3 bytes - variable, covers the entire packet)
 *   Packet Type                     (1 byte  - MQTTSN_PUBWOS)
 *   PUBWOS Flags                    (1 byte)               (Section 3.6.1.2)
 *   Topic Alias or Topic Name Length(2 bytes - MSB, LSB)    (Section 3.6.1.3)
 *   Topic Name                      (N bytes, only when Topic Type is Topic Name)
 *                                                           (Section 3.6.1.4)
 *   Payload                         (fills to end of packet) (Section 3.6.1.5)
 */

#include "MQTTSNPacket.h"   /* MQTTSN_topic, MQTTSN_topicTypes, writeInt16, readInt16, ... */
#include "MQTTSNPublish.h"
#include "StackTrace.h"

#include <string.h>

/*
 * MQTT-SN 2.0 PUBWOS Flags byte (Section 3.6.1.2)
 *
 * Bits 7-5: Reserved (must be 0)
 * Bit 4   : Retain
 * Bits 3-2: Reserved (must be 0)
 * Bits 1-0: Topic Type (MUST be Predefined Topic Alias or Topic Name,
 *           Section 3.6.1.2.1)
 */
#define PUBWOS_FLAGS_TOPICTYPE_SHIFT   0
#define PUBWOS_FLAGS_RETAIN_SHIFT      4


/**
  * Determines the length of the MQTT-SN PUBWOS packet body, excluding the
  * length field itself.
  * @param payloadlen length in bytes of the application payload
  * @param topic      the topic, determining type and presence of name string
  * @return the length in bytes of the packet body
  */
static int32_t MQTTSNSerialize_pubwosLength(int32_t payloadlen, const MQTTSN_topic* topic)
{
	/* type(1) + flags(1) + topic_alias_or_len(2) */
	int32_t len = 4;

	if (topic->type == MQTTSN_TOPIC_TYPE_NAME)
		len += topic->alt.string.len;   /* variable-length topic name */

	return len + payloadlen;
}


/**
  * Serializes the supplied PUBWOS data into the supplied buffer, ready for
  * sending.
  *
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param retained   the Retain flag (Section 3.6.1.2.2)
  * @param topic      MQTTSN_topic describing the topic type and value; type
  *                   MUST be MQTTSN_TOPIC_TYPE_PREDEFINED or
  *                   MQTTSN_TOPIC_TYPE_NAME (Section 3.6.1.2.1)
  * @param payload    pointer to the application payload bytes
  * @param payloadlen length in bytes of the payload; 0 is valid
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_pubwos(uint8_t* buf, int32_t buflen,
		uint8_t retained, const MQTTSN_topic* topic,
		uint8_t* payload, int32_t payloadlen)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(
	             MQTTSNSerialize_pubwosLength(payloadlen, topic))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_PUBWOS);                 /* write packet type */

	/* encode PUBWOS flags (Section 3.6.1.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-3  Reserved (must be 0)
	 *   bit  4    Retain
	 *   bits 5-7  Reserved (must be 0)
	 */
	flags = (uint8_t)(
		((retained    & 0x01u) << PUBWOS_FLAGS_RETAIN_SHIFT) |
		((topic->type & 0x03u) << PUBWOS_FLAGS_TOPICTYPE_SHIFT));
	writeChar(&ptr, flags);

	/* Topic Alias or Topic Name Length field (Section 3.6.1.3):
	 *   Topic Name: 2-byte topic name length, followed by the name string
	 *   Predefined Topic Alias: 2-byte topic alias
	 */
	if (topic->type == MQTTSN_TOPIC_TYPE_NAME)
	{
		writeInt16(&ptr, topic->alt.string.len); /* 2-byte Topic Name Length */
		memcpy(ptr, topic->alt.string.data, topic->alt.string.len);
		ptr += topic->alt.string.len;
	}
	else /* MQTTSN_TOPIC_TYPE_PREDEFINED */
		writeInt16(&ptr, topic->alt.alias);      /* 2-byte topic alias */

	memcpy(ptr, payload, (size_t)payloadlen);
	ptr += payloadlen;

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into PUBWOS data.
  *
  * @param retained   returned - the Retain flag (Section 3.6.1.2.2)
  * @param topic      returned - topic type plus alias (alt.alias) or name
  *                   string (alt.string, zero-copy pointer into buf)
  *                   (Section 3.6.1.2.1)
  * @param payload    returned - zero-copy pointer to payload bytes inside buf
  * @param payloadlen returned - length of the payload in bytes
  * @param buf        the raw buffer data of the correct length
  * @param buflen     the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_pubwos(uint8_t* retained, MQTTSN_topic* topic,
		uint8_t** payload, int32_t* payloadlen,
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
	if (!buf_avail(curdata, endbuffer, 4))              /* type(1)+flags(1)+topic_alias_or_len(2) */
		goto exit;

	if (readChar(&curdata) != MQTTSN_PUBWOS)
		goto exit;

	/* decode PUBWOS flags (Section 3.6.1.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-3  Reserved (must be 0)
	 *   bit  4    Retain
	 *   bits 5-7  Reserved (must be 0)
	 */
	flags       = (uint8_t)readChar(&curdata);
	topic->type = (MQTTSN_topicTypes)((flags >> PUBWOS_FLAGS_TOPICTYPE_SHIFT) & 0x03u);
	*retained   = (flags >> PUBWOS_FLAGS_RETAIN_SHIFT) & 0x01u;

	/* Topic Alias or Topic Name Length field (Section 3.6.1.3):
	 *   Topic Name: 2-byte topic name length, followed by the name string
	 *   Predefined Topic Alias: 2-byte topic alias
	 */
	if (topic->type == MQTTSN_TOPIC_TYPE_NAME)
	{
		topic->alt.string.len  = readInt16(&curdata);
		if (!buf_avail(curdata, endbuffer, (int32_t)topic->alt.string.len))
			goto exit;
		topic->alt.string.data = (char*)curdata;    /* zero-copy into buf */
		curdata += topic->alt.string.len;
	}
	else /* MQTTSN_TOPIC_TYPE_PREDEFINED */
		topic->alt.alias = readInt16(&curdata);     /* 2-byte topic alias */

	*payloadlen = (int32_t)(enddata - curdata);
	*payload    = curdata;
	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
