/*******************************************************************************
 * Copyright (c) 2014, 2026 Ian Craggs, IBM Corp.
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
 * MQTTSNDeserialize_connect
 *   - MQTTSNFlags union replaced by MQTTSNPacket_connectFlags with named bits
 *   - WillFlags byte is now read conditionally when the Will bit is set
 *   - Protocol version checked against 0x02 (was 0x01)
 *   - packetId, maxPacketSize read as new fixed fields
 *   - defaultAwakeMessages and sessionExpiryInterval read conditionally on flags
 *   - Will topic and payload are now embedded in CONNECT (not negotiated via a
 *     separate WILLTOPIC/WILLMSG exchange): topic type determines whether a
 *     2-byte alias or a length-prefixed name string follows
 *   - Auth method (1-byte length) and auth data (2-byte length) read
 *     conditionally when the Auth flag is set
 *   - Client Identifier fills to end of packet (zero-copy pointer into buf)
 *   - MQTTSNString replaced throughout by MQTTSN_string
 *   - readInt() → readInt16(); new readInt32() for sessionExpiryInterval
 *
 * MQTTSNSerialize_connack
 *   - Signature changed from (int connack_rc) to (MQTTSNPacket_connackData*)
 *   - Flags byte added (sessionPresent, sessionExpiryInterval, serverKeepAlive,
 *     auth) per Section 3.2.2
 *   - packetId field added
 *   - Optional fields written conditionally based on flags:
 *     sessionExpiryInterval (4), serverKeepAlive (2), auth method/data
 *   - Assigned Client Identifier fills to end when non-zero length
 *
 * MQTTSNDeserialize_disconnect
 *   - Signature changed: duration field replaced by MQTTSNPacket_disconnectData*
 *   - Flags byte added (packetId, sessionExpiryInterval, reasonCode) per §3.13.2
 *   - Packet Identifier, Reason Code, Session Expiry Interval and Reason String
 *     all read conditionally; Reason Code defaults to 0x00 when absent
 *
 * MQTTSNDeserialize_pingreq
 *   - MQTTSNString* → MQTTSN_string*
 *
 * MQTTSNSerialize_pingresp
 *   - uint8_t/int32_t types applied
 *
 * Removed entirely (will negotiation exchange abolished in v2.0):
 *   MQTTSNSerialize_willtopicreq, MQTTSNSerialize_willmsgreq,
 *   MQTTSNDeserialize_willtopic{,upd}, MQTTSNDeserialize_willmsg{,upd},
 *   MQTTSNSerialize_willtopicresp, MQTTSNSerialize_willmsgresp,
 *   and their two private helpers MQTTSNDeserialize_willtopic1 / willmsg1
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"
#include "StackTrace.h"

#include <string.h>


/**
  * Deserializes the supplied (wire) buffer into a connect data structure.
  *
  * @param data   the connect data structure to be filled out (zero-copy: string
  *               and data fields point directly into buf)
  * @param buf    the raw buffer data
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int32_t MQTTSNDeserialize_connect(MQTTSNPacket_connectData* data,
		uint8_t* buf, int32_t buflen)
{
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
	if (enddata - curdata < 2)
		goto exit;

	if (readChar(&curdata) != MQTTSN_CONNECT)
		goto exit;

	/* Connect Flags (Section 3.1.2) */
	data->flags.all = (uint8_t)readChar(&curdata);

	/* Will Flags: present only when the Will bit is set (Section 3.1.3) */
	if (data->flags.bits.will)
		data->willFlags.all = (uint8_t)readChar(&curdata);

	data->packetId = readInt16(&curdata);          /* Section 3.1.4 */

	/* Protocol Version — MUST be 0x02 for MQTT-SN 2.0 (Section 3.1.5) */
	if ((uint8_t)readChar(&curdata) != MQTTSN_PROTOCOL_VERSION)
		goto exit;

	data->keepAlive     = readInt16(&curdata);     /* Section 3.1.6 */
	data->maxPacketSize = readInt16(&curdata);     /* Section 3.1.7 */

	/* Default Awake Messages: optional, present when flag is set (Section 3.1.8) */
	if (data->flags.bits.defaultAwakeMessages)
		data->defaultAwakeMessages = (uint8_t)readChar(&curdata);

	/* Session Expiry Interval: optional, present when flag is set (Section 3.1.9) */
	if (data->flags.bits.sessionExpiryInterval)
	{
		uint32_t sei = (uint32_t)((uint8_t)readChar(&curdata)) << 24;
		sei |= (uint32_t)((uint8_t)readChar(&curdata)) << 16;
		sei |= (uint32_t)((uint8_t)readChar(&curdata)) << 8;
		sei |= (uint32_t)((uint8_t)readChar(&curdata));
		data->sessionExpiryInterval = sei;
	}

	/* Will fields: embedded in CONNECT in v2.0 (Sections 3.1.10-3.1.13) */
	if (data->flags.bits.will)
	{
		data->will.topic.type =
			(MQTTSN_topicTypes)data->willFlags.bits.topicType;

		if (data->will.topic.type == MQTTSN_TOPIC_TYPE_SESSION ||
		    data->will.topic.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
		{
			/* 2-byte topic alias */
			data->will.topic.alt.alias = readInt16(&curdata);
		}
		else /* MQTTSN_TOPIC_TYPE_NAME: length-prefixed topic name */
		{
			data->will.topic.alt.string.len  = readInt16(&curdata);
			data->will.topic.alt.string.data = (char*)curdata;
			curdata += data->will.topic.alt.string.len;
		}

		/* Will Payload: 2-byte length then binary data (Section 3.1.12-3.1.13) */
		data->will.payload.len  = readInt16(&curdata);
		data->will.payload.data = curdata;
		curdata += data->will.payload.len;
	}

	/* Authentication (Sections 3.1.14-3.1.17): present when Auth flag is set.
	 * Method uses a 1-byte length prefix; Data uses a 2-byte length prefix.
	 */
	if (data->flags.bits.auth)
	{
		data->auth.method.islen8 = 1;
		data->auth.method.len    = (uint16_t)(uint8_t)readChar(&curdata);
		data->auth.method.data   = (char*)curdata;
		curdata += data->auth.method.len;

		data->auth.data.len  = readInt16(&curdata);
		data->auth.data.data = curdata;
		curdata += data->auth.data.len;
	}

	/* Client Identifier: fills to end of packet; zero-copy (Section 3.1.18) */
	data->clientID.len  = (uint16_t)(enddata - curdata);
	data->clientID.data = (char*)curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Calculates the length of the CONNACK packet body excluding the length field.
  * @param data the connack data to be serialized
  * @return the number of bytes required for the packet body
  */
static int32_t MQTTSNSerialize_connackLength(MQTTSNPacket_connackData* data)
{
	/* fixed: type(1) + flags(1) + packetId(2) + reasonCode(1) = 5 */
	int32_t len = 5;

	if (data->flags.bits.sessionExpiryInterval)
		len += 4;                              /* Section 3.2.5 */
	if (data->flags.bits.serverKeepAlive)
		len += 2;                              /* Section 3.2.6 */
	if (data->flags.bits.auth)
		len += 1 + data->auth.method.len       /* method: 1-byte len + data */
		     + 2 + data->auth.data.len;        /* auth data: 2-byte len + data */

	len += data->assignedClientID.len;         /* fills to end, Section 3.2.11 */

	return len;
}


/**
  * Serializes the CONNACK packet into the supplied buffer, ready for sending.
  *
  * @param buf    the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param data   the connack data to serialize
  * @return serialized length, or error if <= 0
  */
int32_t MQTTSNSerialize_connack(uint8_t* buf, int32_t buflen,
		MQTTSNPacket_connackData* data)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)MQTTSNSerialize_connackLength(data)))
	    > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_CONNACK);                /* write packet type */

	/* CONNACK Flags (Section 3.2.2): bits 0-3 used, bits 7-4 reserved (0) */
	writeChar(&ptr, (char)data->flags.all);

	writeInt16(&ptr, data->packetId);                /* Section 3.2.3 */
	writeChar(&ptr,  (char)data->reasonCode);        /* Section 3.2.4 */

	/* Optional fields written only when their flag is set */

	if (data->flags.bits.sessionExpiryInterval)      /* Section 3.2.5 */
		writeInt32(&ptr, data->sessionExpiryInterval);

	if (data->flags.bits.serverKeepAlive)            /* Section 3.2.6 */
		writeInt16(&ptr, data->serverKeepAlive);

	if (data->flags.bits.auth)                       /* Sections 3.2.7-3.2.10 */
	{
		/* auth method has 1-byte length prefix */
		writeChar(&ptr, (char)(uint8_t)data->auth.method.len);
		writeMQTTSNString(&ptr, data->auth.method, false);

		/* auth data has 2-byte length prefix */
		writeMQTTSNData(&ptr, data->auth.data, true);
	}

	/* Assigned Client Identifier: written when non-empty, fills to end
	 * (Section 3.2.11); no length prefix — length inferred from packet length
	 */
	if (data->assignedClientID.len > 0)
		writeMQTTSNString(&ptr, data->assignedClientID, false);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into DISCONNECT data.
  *
  * Changes from MQTT-SN 1.2:
  *   - The "duration" sleep field is absent; DISCONNECT now carries a flags
  *     byte with optional Packet Identifier, Reason Code, Session Expiry
  *     Interval and Reason String fields (Section 3.13)
  *   - Reason Code defaults to 0x00 (Normal disconnection) when absent
  *
  * @param data   the disconnect data structure to be filled out
  * @param buf    the raw buffer data
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int32_t MQTTSNDeserialize_disconnect(MQTTSNPacket_disconnectData* data,
		uint8_t* buf, int32_t buflen)
{
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
	if (enddata - curdata < 1)
		goto exit;

	if (readChar(&curdata) != MQTTSN_DISCONNECT)
		goto exit;

	/* DISCONNECT Flags (Section 3.13.2): bits 0-2 used, bits 7-3 reserved (0) */
	data->flags.all = (uint8_t)readChar(&curdata);

	/* Packet Identifier: present when flag bit 0 is set (Section 3.13.3) */
	if (data->flags.bits.packetId)
		data->packetId = readInt16(&curdata);
	else
		data->packetId = 0;

	/* Reason Code: present when flag bit 2 is set (Section 3.13.4);
	 * defaults to 0x00 (Normal disconnection) when absent
	 */
	if (data->flags.bits.reasonCode)
		data->reasonCode = (uint8_t)readChar(&curdata);
	else
		data->reasonCode = MQTT_SN_RC_SUCCESS;

	/* Session Expiry Interval: present when flag bit 1 is set (Section 3.13.5) */
	if (data->flags.bits.sessionExpiryInterval)
	{
		uint32_t sei = (uint32_t)((uint8_t)readChar(&curdata)) << 24;
		sei |= (uint32_t)((uint8_t)readChar(&curdata)) << 16;
		sei |= (uint32_t)((uint8_t)readChar(&curdata)) << 8;
		sei |= (uint32_t)((uint8_t)readChar(&curdata));
		data->sessionExpiryInterval = sei;
	}
	else
		data->sessionExpiryInterval = 0;

	/* Reason String: optional, fills to end of packet (Section 3.13.6) */
	data->reasonString.len  = (uint16_t)(enddata - curdata);
	data->reasonString.data = (char*)curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into PINGREQ data (server
  * receive side).
  *
  * Changes from MQTT-SN 1.2:
  *   - Client Identifier replaced by a mandatory Packet Identifier (Section
  *     3.11.2).  The server MUST echo this value in the corresponding PINGRESP.
  *
  * @param packetid returned Packet Identifier from the PINGREQ
  * @param buf      the raw buffer data
  * @param buflen   the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int32_t MQTTSNDeserialize_pingreq(uint16_t* packetid,
		uint8_t* buf, int32_t buflen)
{
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
	(void)enddata;
	if (mylen < 4)           /* length(1) + type(1) + packetid(2) */
		goto exit;

	if (readChar(&curdata) != MQTTSN_PINGREQ)
		goto exit;

	*packetid = readInt16(&curdata);               /* Section 3.11.2 */

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a PINGRESP packet into the supplied buffer (server send side).
  *
  * Changes from MQTT-SN 1.2:
  *   - Packet Identifier added (Section 3.12.2); MUST echo the Packet
  *     Identifier from the corresponding PINGREQ.
  *   - Application Messages Remaining (AMR) optional field added (Section
  *     3.12.3).  Pass messages_remaining >= 0 to include it; -1 to omit.
  *   - AMR tells a sleeping Client how many Application Messages remain
  *     queued at the Server.  0xFF means "an unspecified positive number".
  *
  * @param buf                the buffer into which the packet will be serialized
  * @param buflen             the length in bytes of the supplied buffer
  * @param packetid           Packet Identifier echoed from the PINGREQ
  * @param messages_remaining Application Messages Remaining (0-255 to include,
  *                           -1 to omit the field from the wire)
  * @return serialized length, or error if <= 0
  */
int32_t MQTTSNSerialize_pingresp(uint8_t* buf, int32_t buflen,
		uint16_t packetid, int messages_remaining)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	/* body: type(1) + packetid(2) + [amr(1, optional)] */
	int32_t  bodylen = 3 + (messages_remaining >= 0 ? 1 : 0);

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len((uint16_t)bodylen)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_PINGRESP);               /* write packet type */
	writeInt16(&ptr, packetid);                     /* Section 3.12.2 */

	/* Application Messages Remaining: written only when >= 0 (Section 3.12.3) */
	if (messages_remaining >= 0)
		writeChar(&ptr, (char)(uint8_t)messages_remaining);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
