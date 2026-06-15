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

#include "MQTTSNPacket.h"   /* MQTTSN_topic, MQTTSN_string, readInt16, ... */
#include "MQTTSNPublish.h"
#include "StackTrace.h"

/*
 * MQTT-SN 2.0 PUBLISH Flags byte (Sections 3.6.2.2 and 3.6.3.2)
 *
 * Bit 7   : DUP   (QoS 1 and 2 only; reserved/must be 0 for QoS 0)
 * Bits 6-5: QoS
 * Bit 4   : Retain
 * Bits 3-2: Reserved (must be 0)
 * Bits 1-0: Topic Type
 */
#define PUBLISH_FLAGS_TOPICTYPE_SHIFT   0
#define PUBLISH_FLAGS_RETAIN_SHIFT      4
#define PUBLISH_FLAGS_QOS_SHIFT         5
#define PUBLISH_FLAGS_DUP_SHIFT         7

/*
 * MQTT-SN 2.0 REGISTER Flags byte (Section 3.4.2)
 *
 * Bits 7-1: Reserved (must be 0)
 * Bit 0   : Topic Alias Flag
 */
#define REGISTER_FLAGS_TOPIC_ALIAS_SHIFT    0

/*
 * MQTT-SN 2.0 REGACK Flags byte (Section 3.5.2)
 *
 * Bits 7-3: Reserved (must be 0)
 * Bit 2   : Topic Alias Flag
 * Bits 1-0: Topic Type
 */
#define REGACK_FLAGS_TOPICTYPE_SHIFT        0
#define REGACK_FLAGS_TOPIC_ALIAS_SHIFT      2


/**
  * Deserializes the supplied (wire) buffer into publish data.
  *
  * Changes from MQTT-SN 1.2:
  *   - MQTTSNFlags union replaced by explicit per-field bit extraction
  *   - MQTTSN_topicid replaced by MQTTSN_topic with alt.alias / alt.string
  *   - SHORT topic type removed; Session Topic Alias (type 0) is the equivalent
  *   - QoS 0 packets do not carry a Packet Identifier on the wire (Section 3.6.2)
  *   - QoS 1/2 Packet Identifier is read before the topic field (Section 3.6.3.3)
  *   - Topic Name type reads a 2-byte length then a zero-copy pointer (Section 3.6.3.4-5)
  *   - QoS -1 / qos==3 PUBWOS special case removed (PUBWOS is now a distinct packet)
  *
  * @param dup        returned - the DUP flag
  * @param qos        returned - the QoS level (0, 1, or 2)
  * @param retained   returned - the Retain flag
  * @param packetid   returned - the Packet Identifier (0 for QoS 0; not on wire)
  * @param topic      returned - topic type plus alias (alt.alias) or name string
  *                   (alt.string, zero-copy pointer into buf) (Section 3.6.3.4-5)
  * @param payload    returned - zero-copy pointer to payload bytes inside buf
  * @param payloadlen returned - length of the payload in bytes
  * @param buf        the raw buffer data of the correct length
  * @param buflen     the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_publish(uint8_t* dup, int32_t* qos, uint8_t* retained,
		uint16_t* packetid, MQTTSN_topic* topic,
		uint8_t** payload, int32_t* payloadlen,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  flags;
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_PUBLISH)
		goto exit;

	/* decode PUBLISH flags (Sections 3.6.2.2 / 3.6.3.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-3  Reserved (must be 0)
	 *   bit  4    Retain
	 *   bits 5-6  QoS
	 *   bit  7    DUP
	 */
	flags       = (uint8_t)readChar(&curdata);
	topic->type = (MQTTSN_topicTypes)((flags >> PUBLISH_FLAGS_TOPICTYPE_SHIFT) & 0x03u);
	*retained   = (flags >> PUBLISH_FLAGS_RETAIN_SHIFT) & 0x01u;
	*qos        = (flags >> PUBLISH_FLAGS_QOS_SHIFT)    & 0x03u;
	*dup        = (flags >> PUBLISH_FLAGS_DUP_SHIFT)    & 0x01u;

	/* Packet Identifier is present for QoS 1 and 2 only (Section 3.6.3.3).
	 * For QoS 0 it is absent from the wire; return 0 to the caller.
	 */
	if (*qos > 0)
		*packetid = readInt16(&curdata);
	else
		*packetid = 0;

	/* Topic Alias or Topic Name (Sections 3.6.2.3 / 3.6.3.4):
	 *   SESSION / PREDEFINED: 2-byte topic alias
	 *   NAME: 2-byte topic name length, followed by the name string (zero-copy)
	 */
	if (topic->type == MQTTSN_TOPIC_TYPE_NAME)
	{
		topic->alt.string.len  = readInt16(&curdata);
		topic->alt.string.data = (char*)curdata;    /* zero-copy into buf */
		curdata += topic->alt.string.len;
	}
	else /* MQTTSN_TOPIC_TYPE_SESSION or MQTTSN_TOPIC_TYPE_PREDEFINED */
		topic->alt.alias = readInt16(&curdata);     /* 2-byte topic alias */

	*payloadlen = (int32_t)(enddata - curdata);
	*payload    = curdata;
	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into PUBACK data.
  *
  * Changes from MQTT-SN 1.2:
  *   - topicid parameter removed: PUBACK no longer carries a Topic Alias (Section 3.6.4)
  *   - returncode is now optional: inferred as MQTT_SN_RC_SUCCESS when absent (Section 3.6.4.3)
  *
  * @param packetid   returned - the Packet Identifier
  * @param returncode returned - Reason Code; MQTT_SN_RC_SUCCESS if absent from the wire
  * @param buf        the raw buffer data of the correct length
  * @param buflen     the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_puback(uint16_t* packetid, uint8_t* returncode,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_PUBACK)
		goto exit;

	*packetid = readInt16(&curdata);

	/* reason code is optional: infer success when absent (Section 3.6.4.3) */
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
  * Deserializes a QoS 2 ack packet (PUBREC, PUBREL, or PUBCOMP) from the
  * supplied (wire) buffer.
  *
  * Changes from MQTT-SN 1.2:
  *   - returncode output added: Reason Code is now supported on all QoS 2 ack
  *     packets (Sections 3.6.5.3, 3.6.6.3, 3.6.7.3); inferred as
  *     MQTT_SN_RC_SUCCESS when absent from the wire
  *
  * @param packettype returned - the packet type byte (MQTTSN_PUBREC/PUBREL/PUBCOMP)
  * @param packetid   returned - the Packet Identifier
  * @param returncode returned - Reason Code; MQTT_SN_RC_SUCCESS if absent from wire
  * @param buf        the raw buffer data of the correct length
  * @param buflen     the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_ack(uint8_t* packettype, uint16_t* packetid,
		uint8_t* returncode, uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	*packettype = (uint8_t)readChar(&curdata);
	if (*packettype != MQTTSN_PUBREC &&
	    *packettype != MQTTSN_PUBREL &&
	    *packettype != MQTTSN_PUBCOMP)
		goto exit;

	*packetid = readInt16(&curdata);

	/* reason code is optional: infer success when absent */
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
  * Deserializes the supplied (wire) buffer into REGISTER data.
  *
  * Changes from MQTT-SN 1.2:
  *   - Flags byte added (bit 0 = Topic Alias Flag) (Section 3.4.2)
  *   - Topic Alias is now conditional: read only when the Topic Alias Flag is set
  *     (Section 3.4.3)
  *   - MQTTSNString replaced by MQTTSN_string with len/data members
  *   - Topic Name fills to end of packet; length inferred (not read from wire)
  *
  * @param topic_alias_present returned - 1 if a Topic Alias field is present, 0 otherwise
  * @param topicid             returned - the Topic Alias (valid when topic_alias_present is 1)
  * @param packetid            returned - the Packet Identifier
  * @param topicname           returned - Topic Name (zero-copy pointer into buf)
  * @param buf                 the raw buffer data of the correct length
  * @param buflen              the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_register(uint8_t* topic_alias_present, uint16_t* topicid,
		uint16_t* packetid, MQTTSN_string* topicname,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  flags;
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_REGISTER)
		goto exit;

	/* decode REGISTER flags (Section 3.4.2):
	 *   bit  0    Topic Alias Flag
	 *   bits 7-1  Reserved (must be 0)
	 */
	flags                 = (uint8_t)readChar(&curdata);
	*topic_alias_present  = (flags >> REGISTER_FLAGS_TOPIC_ALIAS_SHIFT) & 0x01u;

	*packetid = readInt16(&curdata);

	/* Topic Alias is present only when the Topic Alias Flag is set (Section 3.4.3) */
	if (*topic_alias_present)
		*topicid = readInt16(&curdata);
	else
		*topicid = 0;

	/* Topic Name fills to end of packet (zero-copy) */
	topicname->len  = (uint16_t)(enddata - curdata);
	topicname->data = (char*)curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into REGACK data.
  *
  * Changes from MQTT-SN 1.2:
  *   - Flags byte added with Topic Type (bits 0-1) and Topic Alias Flag (bit 2) (Section 3.5.2)
  *   - Topic Alias is now conditional: read only when the Topic Alias Flag is set (Section 3.5.4)
  *   - return_code renamed returncode and is now optional: inferred as MQTT_SN_RC_SUCCESS
  *     when absent (Section 3.5.5)
  *
  * @param topic_type          returned - Topic Type from flags bits 0-1
  * @param topic_alias_present returned - 1 if a Topic Alias field is present, 0 otherwise
  * @param topicid             returned - the Topic Alias (valid when topic_alias_present is 1)
  * @param packetid            returned - the Packet Identifier
  * @param returncode          returned - Reason Code; MQTT_SN_RC_SUCCESS if absent from wire
  * @param buf                 the raw buffer data of the correct length
  * @param buflen              the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_regack(uint8_t* topic_type, uint8_t* topic_alias_present,
		uint16_t* topicid, uint16_t* packetid, uint8_t* returncode,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  flags;
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_REGACK)
		goto exit;

	/* decode REGACK flags (Section 3.5.2):
	 *   bits 0-1  Topic Type
	 *   bit  2    Topic Alias Flag
	 *   bits 7-3  Reserved (must be 0)
	 */
	flags                 = (uint8_t)readChar(&curdata);
	*topic_type           = (flags >> REGACK_FLAGS_TOPICTYPE_SHIFT)   & 0x03u;
	*topic_alias_present  = (flags >> REGACK_FLAGS_TOPIC_ALIAS_SHIFT) & 0x01u;

	*packetid = readInt16(&curdata);

	/* Topic Alias is present only when the Topic Alias Flag is set (Section 3.5.4) */
	if (*topic_alias_present)
		*topicid = readInt16(&curdata);
	else
		*topicid = 0;

	/* Reason Code is optional: infer success when absent (Section 3.5.5) */
	if (curdata < enddata)
		*returncode = (uint8_t)readChar(&curdata);
	else
		*returncode = MQTT_SN_RC_SUCCESS;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
