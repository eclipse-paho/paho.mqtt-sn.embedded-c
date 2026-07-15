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
 * Serialization and deserialization of the MQTT-SN 2.0 Forwarder
 * Encapsulation packet (Section 3.19).
 *
 * Wire format (Section 3.19 / Figure 30):
 *
 *   Length                    (1 or 3 bytes — variable; unlike every other
 *                              envelope in this library, this covers ONLY
 *                              the Length field itself, the Packet Type and
 *                              the Client Addressing Information — NOT the
 *                              MQTT-SN Packet that follows)
 *   Packet Type               (1 byte  — 0xFD = MQTTSN_FORWARDER)
 *   Client Addressing Information (variable — fills to the end of the
 *                              outer Length, Section 3.19.2)
 *   MQTT-SN Packet            (variable — self-describing inner packet
 *                              with its own Length/Type prefix, fills to
 *                              the end of the physical buffer, Section 3.19.3)
 *
 * This library handles only the wire envelope; it does not interpret the
 * encapsulated MQTT-SN packet's contents.
 */

#include "MQTTSNPacket.h"
#include "MQTTSNForwarder.h"
#include "StackTrace.h"

#include <string.h>


/**
  * Serializes a Forwarder Encapsulation packet into the supplied buffer,
  * ready for sending.
  *
  * The caller is responsible for providing the already-serialized inner
  * packet in data->mqttsnPacket (including its own Length/Type prefix).
  *
  * @param buf    the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param data   the forwarder data to serialize
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_forwarder(uint8_t* buf, int32_t buflen,
		const MQTTSNPacket_forwarderData* data)
{
	uint8_t  *ptr = buf;
	int32_t  rc = 0;
	int32_t  headerlen; /* Length field's own scope: itself + type + Client Addressing Information */
	int32_t  totallen;  /* headerlen + the self-describing inner MQTT-SN packet */

	FUNC_ENTRY;
	/* body (of the outer Length field): type(1) + Client Addressing Information */
	headerlen = MQTTSNPacket_len((uint16_t)(1 + data->clientAddress.len));
	totallen  = headerlen + (int32_t)data->mqttsnPacket.len;
	if (totallen > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	/* write length - covers only up to the end of Client Addressing
	 * Information, not the inner MQTT-SN Packet (Section 3.19.1) */
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)headerlen);
	writeChar(&ptr, (unsigned int)MQTTSN_FORWARDER); /* write packet type */

	/* Client Addressing Information — opaque, fills to end of outer Length
	 * (Section 3.19.2) */
	if (data->clientAddress.len > 0)
		writeMQTTSNData(&ptr, data->clientAddress, false);

	/* MQTT-SN Packet — already fully serialized by the caller, including
	 * its own Length/Type prefix (Section 3.19.3) */
	if (data->mqttsnPacket.len > 0)
	{
		memcpy(ptr, data->mqttsnPacket.data, data->mqttsnPacket.len);
		ptr += data->mqttsnPacket.len;
	}

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes a Forwarder Encapsulation packet from the supplied wire buffer.
  *
  * Both fields in the output structure are set as zero-copy pointers into
  * buf. buf must remain valid for the lifetime of the structure.
  *
  * The inner MQTT-SN packet's length is determined from its own embedded
  * length field, not from the outer Length field (which does not cover it).
  * mqttsnPacket spans its own Length/Type prefix plus body, so it can be
  * handed directly to MQTTSNPacket_read()-style parsing.
  *
  * @param data   the structure to fill with parsed packet fields
  * @param buf    the raw buffer data
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int32_t MQTTSNDeserialize_forwarder(MQTTSNPacket_forwarderData* data,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	uint8_t  *endbuffer = buf + buflen;
	int32_t  rc = 0;
	int32_t  mylen = 0;
	int32_t  lenlen;
	int32_t  inner_lenlen;
	int32_t  inner_mylen = 0;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read outer length */
	if (lenlen < 0)
	{
		rc = MQTTSNPACKET_READ_ERROR;
		goto exit;
	}
	/* the outer Length only covers the header + Client Addressing
	 * Information (Section 3.19.1); it must still fit inside the supplied
	 * buffer, but the buffer may legitimately extend further to hold the
	 * inner MQTT-SN Packet */
	if (buflen < mylen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	curdata += lenlen;
	enddata = buf + mylen; /* end of Client Addressing Information */
	if (!buf_avail(curdata, endbuffer, 1))              /* type(1) */
		goto exit;

	if ((uint8_t)readChar(&curdata) != MQTTSN_FORWARDER)
		goto exit;

	/* Client Addressing Information: fills to the end of the outer Length,
	 * zero-copy (Section 3.19.2) */
	if (curdata > enddata)
		goto exit;
	data->clientAddress.len  = (uint16_t)(enddata - curdata);
	data->clientAddress.data = (data->clientAddress.len > 0) ? curdata : NULL;
	curdata = enddata;

	/* MQTT-SN Packet: self-describing inner packet (Section 3.19.3).
	 *
	 * Its length is not covered by the outer Length field, so it is decoded
	 * from its own leading length field and bounds-checked against the
	 * physical end of the supplied buffer.
	 */
	if (!buf_avail(curdata, endbuffer, 1))
		goto exit; /* the MQTT-SN Packet is mandatory */

	inner_lenlen = MQTTSNPacket_decode(curdata, (int32_t)(endbuffer - curdata), &inner_mylen);
	if (inner_lenlen < 0)
	{
		rc = MQTTSNPACKET_READ_ERROR;
		goto exit;
	}
	if (!buf_avail(curdata, endbuffer, inner_mylen))
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}

	data->mqttsnPacket.len  = (uint16_t)inner_mylen;
	data->mqttsnPacket.data = curdata;
	curdata += inner_mylen;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
