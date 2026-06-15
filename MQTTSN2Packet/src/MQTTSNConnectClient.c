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
 * Changes from the partially-updated MQTT-SN 2.0 intermediate file:
 *
 * MQTTSNSerialize_connectLength (now static)
 *   - Removed the erroneous `len += (len > 255) ? 3 : 1` at the end, which
 *     double-counted the length field since MQTTSNPacket_len() is called on
 *     the result in MQTTSNSerialize_connect.  The function now returns the
 *     body length only, consistent with all other *Length helpers.
 *   - options parameter made const.
 *
 * MQTTSNSerialize_connect
 *   - Auth method: replaced the mutation `options->auth.method.islen8 = 1`
 *     with an explicit writeChar of the 1-byte length, leaving the input
 *     struct unchanged.  options parameter made const.
 *   - All types updated to uint8_t* / int32_t.
 *   - (int32_t)(ptr - buf) narrowing cast added.
 *
 * MQTTSNDeserialize_connack
 *   - Optional fields now read after the fixed fields, mirroring the server
 *     serializer field order:
 *       sessionExpiryInterval (4 bytes, if flag set)
 *       serverKeepAlive       (2 bytes, if flag set)
 *       auth method (1-byte length prefix) and auth data (2-byte length prefix)
 *       assignedClientID      (fills to end; zero-copy)
 *   - All types updated.
 *
 * MQTTSNSerialize_disconnect
 *   - `int duration` replaced by `uint8_t reason_code`.  The v1.2 sleep
 *     duration is gone; sleep is now handled by the separate SLEEPREQ packet.
 *   - Flags byte added as the first field after the packet type (Section 3.13.2):
 *     bit 2 (Reason Code Flag) is set when reason_code != MQTT_SN_RC_SUCCESS.
 *   - MQTTSNSerialize_disconnectLength removed (absorbed here).
 *
 * MQTTSNSerialize_pingreq
 *   - writeMQTTSNString call uncommented and corrected: clientid is written
 *     without a length prefix (fills to end), only when len > 0.
 *   - All types updated.
 *
 * MQTTSNDeserialize_pingresp
 *   - All types updated.
 *
 * General
 *   - #include <string.h> removed (no longer needed).
 *   - #include "MQTTSNConnect.h" added so the file includes its own header.
 *   - Copyright year updated to 2026.
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"
#include "StackTrace.h"

/*
 * DISCONNECT Flags bit positions (Section 3.13.2)
 *
 * Bit 0: Packet Identifier Flag
 * Bit 1: Session Expiry Interval Flag
 * Bit 2: Reason Code Flag
 * Bits 7-3: Reserved (must be 0)
 */
#define DISCONNECT_FLAGS_REASONCODE_SHIFT   2u


/**
  * Determines the body length of the CONNECT packet that would be produced
  * using the supplied options, excluding the variable-length length field.
  * Called by MQTTSNSerialize_connect which passes the result to MQTTSNPacket_len.
  *
  * @param options the options to be used to build the connect packet
  * @return body length in bytes, not including the length field itself
  */
static int32_t MQTTSNSerialize_connectLength(const MQTTSNPacket_connectData* options)
{
	/* Fixed fields: type(1) + connectFlags(1) + packetId(2) + protocolVersion(1)
	 *               + keepAlive(2) + maxPacketSize(2) = 9
	 */
	int32_t len = 9;

	if (options->flags.bits.will)
		len += 1;                                /* WillFlags byte (Section 3.1.3) */
	if (options->flags.bits.defaultAwakeMessages)
		len += 1;                                /* Section 3.1.8 */
	if (options->flags.bits.sessionExpiryInterval)
		len += 4;                                /* Section 3.1.9 */

	if (options->flags.bits.will)
	{
		/* Will Topic Alias or Length field(2) + Will Payload Length field(2) */
		len += 4;
		if (options->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_NAME)
			len += options->will.topic.alt.string.len; /* Section 3.1.11 */
		len += options->will.payload.len;              /* Section 3.1.13 */
	}

	if (options->flags.bits.auth)
		/* method: 1-byte length prefix + data; auth data: 2-byte length + data */
		len += 1 + options->auth.method.len + 2 + options->auth.data.len;

	len += options->clientID.len;                    /* Section 3.1.18 */

	/* NOTE: the length field itself is NOT included here; MQTTSNPacket_len()
	 * adds it in MQTTSNSerialize_connect.
	 */
	return len;
}


/**
  * Serializes the connect options into the supplied buffer, ready for sending.
  *
  * @param buf     the buffer into which the packet will be serialized
  * @param buflen  the length in bytes of the supplied buffer
  * @param options the options to be used to build the connect packet
  * @return serialized length, or error if <= 0
  */
int32_t MQTTSNSerialize_connect(uint8_t* buf, int32_t buflen,
		const MQTTSNPacket_connectData* options)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = -1;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(
	             (uint16_t)MQTTSNSerialize_connectLength(options))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_CONNECT);                /* write packet type */

	writeChar(&ptr, (char)options->flags.all);      /* Connect Flags (Section 3.1.2) */

	/* Will Flags follow Connect Flags when the Will bit is set (Section 3.1.3) */
	if (options->flags.bits.will)
		writeChar(&ptr, (char)options->willFlags.all);

	writeInt16(&ptr, options->packetId);                 /* Section 3.1.4 */
	writeChar(&ptr,  MQTTSN_PROTOCOL_VERSION);           /* Section 3.1.5: 0x02 */
	writeInt16(&ptr, options->keepAlive);                /* Section 3.1.6 */
	writeInt16(&ptr, options->maxPacketSize);            /* Section 3.1.7 */

	if (options->flags.bits.defaultAwakeMessages)
		writeChar(&ptr, (char)options->defaultAwakeMessages); /* Section 3.1.8 */

	if (options->flags.bits.sessionExpiryInterval)
		writeInt32(&ptr, options->sessionExpiryInterval);     /* Section 3.1.9 */

	if (options->flags.bits.will)
	{
		if (options->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_SESSION ||
		    options->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_PREDEFINED)
			writeInt16(&ptr, options->will.topic.alt.alias);  /* Section 3.1.10 */
		else /* MQTTSN_TOPIC_TYPE_NAME: 2-byte length then name */
			writeMQTTSNString(&ptr, options->will.topic.alt.string, true);
		writeMQTTSNData(&ptr, options->will.payload, true);   /* Sections 3.1.12-3.1.13 */
	}

	if (options->flags.bits.auth)
	{
		/* Auth method has a 1-byte length prefix (Section 3.1.14-3.1.15).
		 * Write the length explicitly to avoid mutating the input struct
		 * (the islen8 field must not be set by the serializer).
		 */
		writeChar(&ptr, (char)(uint8_t)options->auth.method.len);
		writeMQTTSNString(&ptr, options->auth.method, false);

		/* Auth data has a 2-byte length prefix (Section 3.1.16-3.1.17) */
		writeMQTTSNData(&ptr, options->auth.data, true);
	}

	/* Client Identifier fills to end of packet; no length prefix (Section 3.1.18) */
	writeMQTTSNString(&ptr, options->clientID, false);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into CONNACK data (client receive side).
  *
  * All optional fields are read when their corresponding flag is set, mirroring
  * the field order written by MQTTSNSerialize_connack on the server side:
  *   sessionExpiryInterval (4 bytes, if flag bit 1)
  *   serverKeepAlive       (2 bytes, if flag bit 2)
  *   auth method (1-byte length prefix + data) and auth data (2-byte length + data)
  *   assignedClientID (fills to end; zero-copy pointer into buf)
  *
  * @param data   the connack data structure to be filled out
  * @param buf    the raw buffer data
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int32_t MQTTSNDeserialize_connack(MQTTSNPacket_connackData* data,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - buf < 3)
		goto exit;

	if (readChar(&curdata) != MQTTSN_CONNACK)                /* packet type */
		goto exit;

	data->flags.all  = (uint8_t)readChar(&curdata);          /* Section 3.2.2 */
	data->packetId   = readInt16(&curdata);                   /* Section 3.2.3 */
	data->reasonCode = (uint8_t)readChar(&curdata);           /* Section 3.2.4 */

	/* Optional fields (Sections 3.2.5-3.2.11) */

	if (data->flags.bits.sessionExpiryInterval)
	{
		uint32_t sei = (uint32_t)((uint8_t)readChar(&curdata)) << 24;
		sei |= (uint32_t)((uint8_t)readChar(&curdata)) << 16;
		sei |= (uint32_t)((uint8_t)readChar(&curdata)) << 8;
		sei |= (uint32_t)((uint8_t)readChar(&curdata));
		data->sessionExpiryInterval = sei;
	}

	if (data->flags.bits.serverKeepAlive)
		data->serverKeepAlive = readInt16(&curdata);

	if (data->flags.bits.auth)
	{
		/* Auth method: 1-byte length prefix then string data */
		data->auth.method.islen8 = 1;
		data->auth.method.len    = (uint16_t)(uint8_t)readChar(&curdata);
		data->auth.method.data   = (char*)curdata;
		curdata += data->auth.method.len;

		/* Auth data: 2-byte length prefix then binary data */
		data->auth.data.len  = readInt16(&curdata);
		data->auth.data.data = curdata;
		curdata += data->auth.data.len;
	}

	/* Assigned Client Identifier fills to end; zero-copy (Section 3.2.11) */
	data->assignedClientID.len  = (uint16_t)(enddata - curdata);
	data->assignedClientID.data = (char*)curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a DISCONNECT packet into the supplied buffer, ready for sending.
  *
  * Changes from MQTT-SN 1.2:
  *   - `int duration` (sleep timeout) replaced by `uint8_t reason_code`.
  *     Sleep is now handled by the separate SLEEPREQ packet.
  *   - Flags byte added as the first field after the packet type (Section 3.13.2).
  *     Bit 2 (Reason Code Flag) is set when reason_code != MQTT_SN_RC_SUCCESS.
  *   - MQTTSNSerialize_disconnectLength removed; body size computed inline.
  *
  * @param buf         the buffer into which the packet will be serialized
  * @param buflen      the length in bytes of the supplied buffer
  * @param reason_code Reason Code to include; 0x00 (MQTT_SN_RC_SUCCESS) sends a
  *                    minimal packet with no Reason Code field on the wire
  * @return serialized length, or error if <= 0
  */
int32_t MQTTSNSerialize_disconnect(uint8_t* buf, int32_t buflen,
		uint8_t reason_code)
{
	uint8_t  *ptr = buf;
	uint8_t  flags = 0;
	int32_t  len = 0;
	int32_t  rc = -1;

	/* body: type(1) + flags(1) + [reasonCode(1, if not success)] */
	int32_t  bodylen = 2 + (reason_code != MQTT_SN_RC_SUCCESS ? 1 : 0);

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)bodylen)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_DISCONNECT);              /* write packet type */

	/* DISCONNECT Flags (Section 3.13.2): set Reason Code Flag when non-zero */
	if (reason_code != MQTT_SN_RC_SUCCESS)
		flags |= (uint8_t)(1u << DISCONNECT_FLAGS_REASONCODE_SHIFT);
	writeChar(&ptr, (char)flags);

	/* Reason Code is optional: omit when MQTT_SN_RC_SUCCESS (Section 3.13.4) */
	if (reason_code != MQTT_SN_RC_SUCCESS)
		writeChar(&ptr, (char)reason_code);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a PINGREQ packet into the supplied buffer, ready for sending.
  *
  * A sleeping Client MUST include its Client Identifier in the PINGREQ packet
  * to identify itself to the Server [MQTT-SN-3.11.3-1].  Connected Clients
  * send PINGREQ with a zero-length Client Identifier.
  *
  * @param buf      the buffer into which the packet will be serialized
  * @param buflen   the length in bytes of the supplied buffer
  * @param clientid Client Identifier string; zero-length for connected Clients
  * @return serialized length, or error if <= 0
  */
int32_t MQTTSNSerialize_pingreq(uint8_t* buf, int32_t buflen,
		MQTTSN_string clientid)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = -1;

	FUNC_ENTRY;
	/* body: type(1) + clientid (optional, fills to end) */
	if ((len = MQTTSNPacket_len((uint16_t)(1 + clientid.len))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_PINGREQ);                /* write packet type */

	/* Client Identifier fills to end; no length prefix; omit when empty */
	if (clientid.len > 0)
		writeMQTTSNString(&ptr, clientid, false);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer, verifying it is a valid PINGRESP.
  *
  * @param buf    the raw buffer data
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int32_t MQTTSNDeserialize_pingresp(uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - curdata < 1)
		goto exit;

	if (readChar(&curdata) != MQTTSN_PINGRESP)
		goto exit;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
