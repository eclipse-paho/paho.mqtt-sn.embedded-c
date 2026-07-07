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

#include "MQTTSNPacket.h"   /* MQTTSN_topic, MQTTSN_topicTypes, writeInt16, readInt16, ... */
#include "MQTTSNSubscribe.h"
#include "StackTrace.h"

/*
 * MQTT-SN 2.0 SUBSCRIBE Flags byte (Section 3.7.2)
 *
 * Bit 7   : No Local
 * Bits 6-5: QoS
 * Bit 4   : Retain as Published (RaP)
 * Bits 3-2: Retain Handling
 * Bits 1-0: Topic Type
 */
#define SUBSCRIBE_FLAGS_TOPICTYPE_SHIFT     0
#define SUBSCRIBE_FLAGS_RETAIN_HDL_SHIFT    2
#define SUBSCRIBE_FLAGS_RAP_SHIFT           4
#define SUBSCRIBE_FLAGS_QOS_SHIFT           5
#define SUBSCRIBE_FLAGS_NOLOCAL_SHIFT       7

/*
 * MQTT-SN 2.0 SUBACK Flags byte (Section 3.8.2)
 *
 * Bits 7-3: Reserved (must be 0)
 * Bit 2   : Topic Alias Flag
 * Bits 1-0: Topic Type
 */
#define SUBACK_FLAGS_TOPIC_ALIAS_SHIFT      2


/**
  * Determines the length of the MQTTSN subscribe packet that would be produced
  * using the supplied parameters, excluding the length field itself.
  * @param topic the MQTTSN_topic describing the topic type and value
  * @return the length in bytes needed to contain the serialized packet body
  */
static int32_t MQTTSNSerialize_subscribeLength(MQTTSN_topic* topic)
{
	int32_t len = 4; /* packet type(1) + flags(1) + packet identifier(2) */

	if (topic->type == MQTTSN_TOPIC_TYPE_FILTER || topic->type == MQTTSN_TOPIC_TYPE_NAME)
		len += topic->alt.string.len;   /* variable-length topic filter string */
	else /* MQTTSN_TOPIC_TYPE_SESSION or MQTTSN_TOPIC_TYPE_PREDEFINED */
		len += 2;                       /* 2-byte topic alias */

	return len;
}


/**
  * Serializes the supplied subscribe data into the supplied buffer, ready for sending.
  *
  * Changes from MQTT-SN 1.2:
  *   - dup flag removed (no longer present in SUBSCRIBE)
  *   - retain_handling, rap, and no_local flags added (Section 3.7.2)
  *   - short topic type replaced by Session Topic Alias (MQTTSN_TOPIC_TYPE_SESSION)
  *   - predefined and session topic types both carry a 2-byte alias in topic->alt.alias
  *   - filter/name topic types carry a variable-length UTF-8 string in topic->alt.string
  *
  * @param buf             the buffer into which the packet will be serialized
  * @param buflen          the length in bytes of the supplied buffer
  * @param qos             the requested maximum QoS level (0, 1, or 2)
  * @param retain_handling retain handling option: 0 = send retained on subscribe,
  *                        1 = send only if subscription is new, 2 = never send retained
  * @param rap             Retain as Published: 1 = forwarded messages keep their
  *                        RETAIN flag, 0 = RETAIN is cleared on forwarded messages
  * @param no_local        No Local: 1 = do not deliver messages published by this
  *                        Client back to itself
  * @param packetid        the MQTT-SN packet identifier
  * @param topic           MQTTSN_topic describing the topic type and value
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_subscribe(uint8_t* buf, int32_t buflen,
		int32_t qos, uint8_t retain_handling, uint8_t rap, uint8_t no_local,
		uint16_t packetid, MQTTSN_topic* topic)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNSerialize_subscribeLength(topic))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_SUBSCRIBE);              /* write packet type */

	/* encode SUBSCRIBE flags (Section 3.7.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-3  Retain Handling
	 *   bit  4    Retain as Published
	 *   bits 5-6  QoS
	 *   bit  7    No Local
	 */
	flags = (uint8_t)(
		((no_local        & 0x01u) << SUBSCRIBE_FLAGS_NOLOCAL_SHIFT)    |
		((qos             & 0x03u) << SUBSCRIBE_FLAGS_QOS_SHIFT)        |
		((rap             & 0x01u) << SUBSCRIBE_FLAGS_RAP_SHIFT)        |
		((retain_handling & 0x03u) << SUBSCRIBE_FLAGS_RETAIN_HDL_SHIFT) |
		((topic->type     & 0x03u) << SUBSCRIBE_FLAGS_TOPICTYPE_SHIFT));
	writeChar(&ptr, flags);

	writeInt16(&ptr, packetid);

	/* write topic alias or topic filter/name string according to topic type */
	if (topic->type == MQTTSN_TOPIC_TYPE_FILTER || topic->type == MQTTSN_TOPIC_TYPE_NAME)
		writeMQTTSNString(&ptr, topic->alt.string, false); /* length already in packet len */
	else /* MQTTSN_TOPIC_TYPE_SESSION or MQTTSN_TOPIC_TYPE_PREDEFINED */
		writeInt16(&ptr, topic->alt.alias);                /* 2-byte topic alias */

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into SUBACK data (client receive side).
  *
  * Changes from MQTT-SN 1.2:
  *   - qos parameter removed: granted QoS is now conveyed by the Reason Code
  *     (0x00 = QoS 0, 0x01 = QoS 1, 0x02 = QoS 2)
  *   - topic alias is now optional, indicated by the Topic Alias Flag in flags
  *   - reason code is optional; defaults to 0x00 (Success) if absent
  *
  * @param topicid             returned - the topic alias assigned by the Server
  *                            (valid only when topic_alias_present is non-zero)
  * @param topic_alias_present returned - 1 if a Topic Alias field is present, 0 otherwise
  * @param packetid            returned - the Packet Identifier from the corresponding SUBSCRIBE
  * @param returncode          returned - Reason Code (see Section 2.3):
  *                            0x00 = Granted QoS 0, 0x01 = Granted QoS 1,
  *                            0x02 = Granted QoS 2, >= 0x80 = failure
  * @param buf                 the raw buffer data of the correct length
  * @param buflen              the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_suback(uint16_t* topicid, uint8_t* topic_alias_present,
		uint16_t* packetid, uint8_t* returncode,
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

	if (readChar(&curdata) != MQTTSN_SUBACK)
		goto exit;

	/* decode SUBACK flags (Section 3.8.2):
	 *   bits 0-1  Topic Type (Predefined or Session; informational on receive)
	 *   bit  2    Topic Alias Flag: 1 = Topic Alias field is present
	 *   bits 3-7  Reserved (must be 0)
	 */
	flags = readChar(&curdata);
	*topic_alias_present = (flags >> SUBACK_FLAGS_TOPIC_ALIAS_SHIFT) & 0x01u;

	*packetid = readInt16(&curdata);

	/* topic alias field is present only when the Topic Alias Flag is set */
	if (*topic_alias_present)
	{
		if (!buf_avail(curdata, endbuffer, 2))
			goto exit;
		*topicid = readInt16(&curdata);
	}
	else
		*topicid = 0u;

	/* reason code is optional: infer its presence from remaining packet length
	 * (Section 3.8.5). If absent, 0x00 (Success / Granted QoS 0) is assumed.
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
