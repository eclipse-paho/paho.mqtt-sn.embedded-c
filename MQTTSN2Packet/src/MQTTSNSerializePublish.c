/*******************************************************************************
 * Copyright (c) 2014, 2025 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

#include "MQTTSNPacket.h"   /* MQTTSN_topic, MQTTSN_string, writeInt16, ... */
#include "MQTTSNPublish.h"
#include "StackTrace.h"

#include <string.h>

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
  * Determines the length of the MQTT-SN publish packet body, excluding the
  * length field itself.
  *
  * Changes from MQTT-SN 1.2:
  *   - QoS 0 packets no longer carry a Packet Identifier (Section 3.6.2)
  *   - QoS 1/2 packets carry the Packet Identifier before the topic field (Section 3.6.3)
  *   - Topic Name type carries a 2-byte length prefix then the name string
  *   - QoS -1 / PUBWOS is now a separate packet type; qos==3 removed
  *
  * @param payloadlen length in bytes of the application payload
  * @param topic      the topic, determining type and presence of name string
  * @param qos        the QoS level (0, 1, or 2)
  * @return the length in bytes of the packet body
  */
static int32_t MQTTSNSerialize_publishLength(int32_t payloadlen,
        const MQTTSN_topic* topic, int32_t qos)
{
	/* type(1) + flags(1) + topic_alias_or_len(2) */
	int32_t len = 4;

	/* QoS 1 and 2 add a Packet Identifier before the topic field (Section 3.6.3.3) */
	if (qos > 0)
		len += 2;

	/* Topic Name type carries the name string after the 2-byte length field */
	if (topic->type == MQTTSN_TOPIC_TYPE_NAME)
		len += topic->alt.string.len;

	return len + payloadlen;
}


/**
  * Serializes the supplied publish data into the supplied buffer, ready for sending.
  *
  * Changes from MQTT-SN 1.2:
  *   - MQTTSNFlags union replaced by explicit bit construction
  *   - MQTTSN_topicid replaced by MQTTSN_topic with alt.alias / alt.string
  *   - SHORT topic type removed; Session Topic Alias (type 0) is the equivalent
  *   - QoS 0 packets do not include a Packet Identifier on the wire (Section 3.6.2)
  *   - QoS 1/2 Packet Identifier is written before the topic field (Section 3.6.3.3)
  *   - Topic Name type writes a 2-byte length then the name string (Section 3.6.3.4-5)
  *   - QoS -1 / qos==3 PUBWOS special case removed (PUBWOS is now a distinct packet)
  *
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param dup        the DUP flag (meaningful for QoS 1/2 retransmissions only)
  * @param qos        the QoS level (0, 1, or 2)
  * @param retained   the Retain flag
  * @param packetid   the Packet Identifier (ignored and not written for QoS 0)
  * @param topic      MQTTSN_topic describing the topic type and value
  * @param payload    pointer to the application payload bytes
  * @param payloadlen length in bytes of the payload
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_publish(uint8_t* buf, int32_t buflen,
		uint8_t dup, int32_t qos, uint8_t retained, uint16_t packetid,
		const MQTTSN_topic* topic, uint8_t* payload, int32_t payloadlen)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(
	             MQTTSNSerialize_publishLength(payloadlen, topic, qos))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_PUBLISH);                /* write packet type */

	/* encode PUBLISH flags (Sections 3.6.2.2 / 3.6.3.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-3  Reserved (must be 0)
	 *   bit  4    Retain
	 *   bits 5-6  QoS
	 *   bit  7    DUP
	 */
	flags = (uint8_t)(
		((dup        & 0x01u) << PUBLISH_FLAGS_DUP_SHIFT)      |
		((qos        & 0x03u) << PUBLISH_FLAGS_QOS_SHIFT)      |
		((retained   & 0x01u) << PUBLISH_FLAGS_RETAIN_SHIFT)   |
		((topic->type & 0x03u) << PUBLISH_FLAGS_TOPICTYPE_SHIFT));
	writeChar(&ptr, flags);

	/* Packet Identifier is present for QoS 1 and 2 only (Section 3.6.3.3).
	 * For QoS 0 it is omitted entirely from the wire (Section 3.6.2).
	 */
	if (qos > 0)
		writeInt16(&ptr, packetid);

	/* Topic Alias or Topic Name Length field (Sections 3.6.2.3 / 3.6.3.4):
	 *   SESSION / PREDEFINED: 2-byte topic alias
	 *   NAME: 2-byte topic name length, followed by the name string
	 */
	if (topic->type == MQTTSN_TOPIC_TYPE_NAME)
	{
		writeInt16(&ptr, topic->alt.string.len); /* 2-byte Topic Name Length */
		memcpy(ptr, topic->alt.string.data, topic->alt.string.len);
		ptr += topic->alt.string.len;
	}
	else /* MQTTSN_TOPIC_TYPE_SESSION or MQTTSN_TOPIC_TYPE_PREDEFINED */
		writeInt16(&ptr, topic->alt.alias);      /* 2-byte topic alias */

	memcpy(ptr, payload, (size_t)payloadlen);
	ptr += payloadlen;

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a PUBACK packet into the supplied buffer.
  *
  * Changes from MQTT-SN 1.2:
  *   - topicid parameter removed: PUBACK no longer carries a Topic Alias (Section 3.6.4)
  *   - returncode is now optional: omitted from the wire when MQTT_SN_RC_SUCCESS,
  *     since the receiver infers success from the absence of a failure code (Section 3.6.4.3)
  *
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param packetid   the Packet Identifier from the PUBLISH being acknowledged
  * @param returncode Reason Code: MQTT_SN_RC_SUCCESS or a failure code >= 0x80
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_puback(uint8_t* buf, int32_t buflen,
		uint16_t packetid, uint8_t returncode)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	/* type(1) + packetid(2) + [reasoncode(1, if not success)] */
	int32_t  bodylen = 3 + (returncode != MQTT_SN_RC_SUCCESS ? 1 : 0);

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)bodylen)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_PUBACK);                 /* write packet type */

	writeInt16(&ptr, packetid);

	/* reason code is optional: omit when success (Section 3.6.4.3) */
	if (returncode != MQTT_SN_RC_SUCCESS)
		writeChar(&ptr, (char)returncode);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes an ack packet (PUBREC, PUBREL, or PUBCOMP) into the supplied buffer.
  *
  * Changes from MQTT-SN 1.2:
  *   - returncode added: Reason Code is now supported on all QoS 2 ack packets
  *     (Sections 3.6.5.3, 3.6.6.3, 3.6.7.3) and omitted from the wire when success
  *
  * @param buf         the buffer into which the packet will be serialized
  * @param buflen      the length in bytes of the supplied buffer
  * @param packet_type the MQTT-SN packet type (MQTTSN_PUBREC, PUBREL, or PUBCOMP)
  * @param packetid    the MQTT-SN packet identifier
  * @param returncode  Reason Code: MQTT_SN_RC_SUCCESS or a failure code >= 0x80
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int32_t MQTTSNSerialize_ack(uint8_t* buf, int32_t buflen,
		uint8_t packet_type, uint16_t packetid, uint8_t returncode)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	/* type(1) + packetid(2) + [reasoncode(1, if not success)] */
	int32_t  bodylen = 3 + (returncode != MQTT_SN_RC_SUCCESS ? 1 : 0);

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)bodylen)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, (char)packet_type);              /* write packet type */

	writeInt16(&ptr, packetid);

	/* reason code is optional: omit when success */
	if (returncode != MQTT_SN_RC_SUCCESS)
		writeChar(&ptr, (char)returncode);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a PUBREC packet into the supplied buffer.
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param packetid   the MQTT-SN packet identifier
  * @param returncode Reason Code: MQTT_SN_RC_SUCCESS or a failure code >= 0x80
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_pubrec(uint8_t* buf, int32_t buflen,
		uint16_t packetid, uint8_t returncode)
{
	return MQTTSNSerialize_ack(buf, buflen, MQTTSN_PUBREC, packetid, returncode);
}


/**
  * Serializes a PUBREL packet into the supplied buffer.
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param packetid   the MQTT-SN packet identifier
  * @param returncode Reason Code: MQTT_SN_RC_SUCCESS or a failure code >= 0x80
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_pubrel(uint8_t* buf, int32_t buflen,
		uint16_t packetid, uint8_t returncode)
{
	return MQTTSNSerialize_ack(buf, buflen, MQTTSN_PUBREL, packetid, returncode);
}


/**
  * Serializes a PUBCOMP packet into the supplied buffer.
  * @param buf        the buffer into which the packet will be serialized
  * @param buflen     the length in bytes of the supplied buffer
  * @param packetid   the MQTT-SN packet identifier
  * @param returncode Reason Code: MQTT_SN_RC_SUCCESS or a failure code >= 0x80
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_pubcomp(uint8_t* buf, int32_t buflen,
		uint16_t packetid, uint8_t returncode)
{
	return MQTTSNSerialize_ack(buf, buflen, MQTTSN_PUBCOMP, packetid, returncode);
}


/**
  * Determines the length of the MQTT-SN REGISTER packet body, excluding the
  * length field itself.
  * @param topicnamelen the length in bytes of the topic name string
  * @param topic_alias_present 1 if the Topic Alias field will be included
  * @return the length in bytes of the packet body
  */
static int32_t MQTTSNSerialize_registerLength(int32_t topicnamelen,
        uint8_t topic_alias_present)
{
	/* type(1) + flags(1) + packetid(2) + [alias(2)] + topic_name */
	return 4 + (topic_alias_present ? 2 : 0) + topicnamelen;
}


/**
  * Serializes the supplied REGISTER data into the supplied buffer, ready for sending.
  *
  * Changes from MQTT-SN 1.2:
  *   - Flags byte added (bit 0 = Topic Alias Flag) (Section 3.4.2)
  *   - Topic Alias is now conditional: present only when topic_alias_present is 1
  *     (Sections 3.4.2.1, 3.4.3)
  *   - MQTTSNString replaced by MQTTSN_string with len/data members
  *   - Topic Name string fills to end of packet (length inferred from packet length)
  *
  * @param buf                 the buffer into which the packet will be serialized
  * @param buflen              the length in bytes of the supplied buffer
  * @param topic_alias_present 1 = include the Topic Alias field, 0 = omit it
  * @param topicid             the Topic Alias to assign (used only when
  *                            topic_alias_present is 1; MUST be 0 when sent by a Client)
  * @param packetid            the Packet Identifier
  * @param topicname           the topic name string (len + data)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_register(uint8_t* buf, int32_t buflen,
		uint8_t topic_alias_present, uint16_t topicid, uint16_t packetid,
		MQTTSN_string* topicname)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(
	             MQTTSNSerialize_registerLength(topicname->len,
	                                            topic_alias_present))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_REGISTER);               /* write packet type */

	/* encode REGISTER flags (Section 3.4.2):
	 *   bit  0    Topic Alias Flag
	 *   bits 7-1  Reserved (must be 0)
	 */
	writeChar(&ptr, (char)((topic_alias_present & 0x01u) << REGISTER_FLAGS_TOPIC_ALIAS_SHIFT));

	writeInt16(&ptr, packetid);

	/* Topic Alias is present only when the Topic Alias Flag is set (Section 3.4.3) */
	if (topic_alias_present)
		writeInt16(&ptr, topicid);

	/* Topic Name fills to end of packet; length is inferred from packet length */
	writeMQTTSNString(&ptr, *topicname, false);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes the supplied REGACK data into the supplied buffer, ready for sending.
  *
  * Changes from MQTT-SN 1.2:
  *   - Flags byte added with Topic Type (bits 0-1) and Topic Alias Flag (bit 2) (Section 3.5.2)
  *   - Topic Alias is now conditional: present only when topic_alias_present is 1
  *     (Sections 3.5.2.2, 3.5.4)
  *   - return_code renamed returncode and is now optional: omitted from the wire
  *     when MQTT_SN_RC_SUCCESS and no topic alias is present (Section 3.5.5)
  *
  * @param buf                 the buffer into which the packet will be serialized
  * @param buflen              the length in bytes of the supplied buffer
  * @param topic_type          Topic Type to report in REGACK flags bits 0-1;
  *                            MUST be SESSION or PREDEFINED (Section 3.5.2.1)
  * @param topic_alias_present 1 = include the Topic Alias field, 0 = omit it
  * @param topicid             the Topic Alias assigned by the Server
  *                            (written only when topic_alias_present is 1)
  * @param packetid            the Packet Identifier from the REGISTER being acknowledged
  * @param returncode          Reason Code: MQTT_SN_RC_SUCCESS on success,
  *                            >= 0x80 on failure (see enum MQTTSN_reasonCodes)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_regack(uint8_t* buf, int32_t buflen,
		uint8_t topic_type, uint8_t topic_alias_present,
		uint16_t topicid, uint16_t packetid, uint8_t returncode)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	/* type(1) + flags(1) + packetid(2) + [alias(2)] + [reasoncode(1)] */
	int32_t  bodylen = 4
	                 + (topic_alias_present ? 2 : 0)
	                 + (returncode != MQTT_SN_RC_SUCCESS || topic_alias_present ? 1 : 0);

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)bodylen)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_REGACK);                 /* write packet type */

	/* encode REGACK flags (Section 3.5.2):
	 *   bits 0-1  Topic Type
	 *   bit  2    Topic Alias Flag
	 *   bits 7-3  Reserved (must be 0)
	 */
	writeChar(&ptr, (char)(
		((topic_alias_present & 0x01u) << REGACK_FLAGS_TOPIC_ALIAS_SHIFT) |
		((topic_type          & 0x03u) << REGACK_FLAGS_TOPICTYPE_SHIFT)));

	writeInt16(&ptr, packetid);

	/* Topic Alias is present only when the Topic Alias Flag is set (Section 3.5.4) */
	if (topic_alias_present)
		writeInt16(&ptr, topicid);

	/* Reason Code is optional: omit when success and no topic alias is present,
	 * since the receiver infers success from the absence of a failure code (Section 3.5.5)
	 */
	if (returncode != MQTT_SN_RC_SUCCESS || topic_alias_present)
		writeChar(&ptr, (char)returncode);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
