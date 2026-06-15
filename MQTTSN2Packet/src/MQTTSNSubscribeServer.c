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
#define SUBACK_FLAGS_TOPICTYPE_SHIFT        0
#define SUBACK_FLAGS_TOPIC_ALIAS_SHIFT      2


/**
  * Deserializes the supplied (wire) buffer into SUBSCRIBE data (server receive side).
  *
  * Changes from MQTT-SN 1.2:
  *   - dup flag removed (no longer present in SUBSCRIBE)
  *   - retain_handling, rap, and no_local flags added (Section 3.7.2)
  *   - short topic type replaced by Session Topic Alias (MQTTSN_TOPIC_TYPE_SESSION)
  *   - predefined and session topic types populate topic->alt.alias
  *   - filter/name topic types populate topic->alt.string (zero-copy: points into buf)
  *
  * @param qos             returned - the requested maximum QoS level
  * @param retain_handling returned - retain handling option (0, 1, or 2)
  * @param rap             returned - Retain as Published flag
  * @param no_local        returned - No Local flag
  * @param packetid        returned - the Packet Identifier
  * @param topic           returned - topic type plus alias (alt.alias) or
  *                        filter string (alt.string, zero-copy pointer into buf)
  * @param buf             the raw buffer data of the correct length
  * @param buflen          the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_subscribe(int32_t* qos, uint8_t* retain_handling,
        uint8_t* rap, uint8_t* no_local, uint16_t* packetid,
        MQTTSN_topic* topic, uint8_t* buf, int32_t buflen)
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

	if (readChar(&curdata) != MQTTSN_SUBSCRIBE)
		goto exit;

	/* decode SUBSCRIBE flags (Section 3.7.2):
	 *   bits 0-1  Topic Type
	 *   bits 2-3  Retain Handling
	 *   bit  4    Retain as Published
	 *   bits 5-6  QoS
	 *   bit  7    No Local
	 */
	flags = (uint8_t)readChar(&curdata);
	topic->type       = (MQTTSN_topicTypes)((flags >> SUBSCRIBE_FLAGS_TOPICTYPE_SHIFT) & 0x03u);
	*retain_handling  = (flags >> SUBSCRIBE_FLAGS_RETAIN_HDL_SHIFT) & 0x03u;
	*rap              = (flags >> SUBSCRIBE_FLAGS_RAP_SHIFT)         & 0x01u;
	*qos              = (flags >> SUBSCRIBE_FLAGS_QOS_SHIFT)         & 0x03u;
	*no_local         = (flags >> SUBSCRIBE_FLAGS_NOLOCAL_SHIFT)     & 0x01u;

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
  * Serializes the supplied SUBACK data into the supplied buffer, ready for sending.
  *
  * Changes from MQTT-SN 1.2:
  *   - qos parameter removed: granted QoS is conveyed by the Reason Code
  *     (MQTT_SN_RC_GRANTED_QOS_0/1/2), not the flags byte
  *   - topic_type added: reported in bits 0-1 of the flags byte
  *   - topic_alias_present added: when 0 the Topic Alias field is omitted entirely,
  *     reducing the fixed packet size from 7 to 5 bytes
  *   - reason code is optional: omitted when MQTT_SN_RC_SUCCESS and no topic alias
  *
  * @param buf                 the buffer into which the packet will be serialized
  * @param buflen              the length in bytes of the supplied buffer
  * @param topic_type          topic type to report in SUBACK flags bits 0-1
  * @param topic_alias_present 1 = include the Topic Alias field, 0 = omit it
  * @param topicid             topic alias assigned by the Server
  *                            (written only when topic_alias_present is 1)
  * @param packetid            Packet Identifier matching the SUBSCRIBE
  * @param returncode          Reason Code: MQTT_SN_RC_GRANTED_QOS_0/1/2 on success,
  *                            >= 0x80 on failure (see enum MQTTSN_reasonCodes)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_suback(uint8_t* buf, int32_t buflen,
		uint8_t topic_type, uint8_t topic_alias_present,
		uint16_t topicid, uint16_t packetid, uint8_t returncode)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = 0;

	/* fixed fields: packet type(1) + flags(1) + packetid(2) = 4 bytes
	 * optional:     topic alias(2) + reason code(1)
	 */
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
	writeChar(&ptr, MQTTSN_SUBACK);                 /* write packet type */

	/* encode SUBACK flags (Section 3.8.2):
	 *   bits 0-1  Topic Type
	 *   bit  2    Topic Alias Flag
	 *   bits 3-7  Reserved (must be 0)
	 */
	flags = (uint8_t)(
		((topic_alias_present & 0x01u) << SUBACK_FLAGS_TOPIC_ALIAS_SHIFT) |
		((topic_type          & 0x03u) << SUBACK_FLAGS_TOPICTYPE_SHIFT));
	writeChar(&ptr, flags);

	writeInt16(&ptr, packetid);

	if (topic_alias_present)
		writeInt16(&ptr, topicid);

	/* reason code is optional: omit when success and no topic alias is present,
	 * since the client can infer success from the absence of a failure code
	 */
	if (returncode != MQTT_SN_RC_SUCCESS || topic_alias_present)
		writeChar(&ptr, (char)returncode);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
