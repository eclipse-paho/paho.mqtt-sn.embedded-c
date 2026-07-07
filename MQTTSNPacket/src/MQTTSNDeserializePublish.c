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
 *    Ian Craggs - use Claude.ai to add guards in deserialize functions
 *******************************************************************************/

#include "StackTrace.h"
#include "MQTTSNPacket.h"
#include <string.h>

#define min(a, b) ((a < b) ? 1 : 0)

/**
  * Deserializes the supplied (wire) buffer into publish data
  * @param dup returned integer - the MQTT dup flag
  * @param qos returned integer - the MQTT QoS value
  * @param retained returned integer - the MQTT retained flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param topicName returned MQTTSNString - the MQTT topic in the publish
  * @param payload returned byte buffer - the MQTT publish payload
  * @param payloadlen returned integer - the length of the MQTT payload
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int MQTTSNDeserialize_publish(unsigned char* dup, int* qos, unsigned char* retained, unsigned short* packetid, MQTTSN_topicid* topic,
		unsigned char** payload, int* payloadlen, unsigned char* buf, int buflen)
{
	MQTTSNFlags flags;
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;
	int lenlen;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
		goto exit;
	curdata += lenlen;
	enddata = buf + mylen;
	if (!buf_avail(curdata, enddata, 2))          /* type(1) + flags(1) */
		goto exit;

	if (readChar(&curdata) != MQTTSN_PUBLISH)
		goto exit;

	flags.all = readChar(&curdata);
	*dup = flags.bits.dup;
	*qos = flags.bits.QoS;
	*retained = flags.bits.retain;

	topic->type = (MQTTSN_topicTypes)flags.bits.topicIdType;

	/* topicid/short name(2) + packetid(2) */
	if (!buf_avail(curdata, enddata, 4))
		goto exit;

	if (topic->type == MQTTSN_TOPIC_TYPE_NORMAL && *qos == 3)
	{
		/* special arrangement for long topic names in QoS -1 publishes.  The length of the topic is in the topicid field */
		topic->data.long_.len = readInt(&curdata);
	}
	else if (topic->type == MQTTSN_TOPIC_TYPE_NORMAL || topic->type == MQTTSN_TOPIC_TYPE_PREDEFINED)
		topic->data.id = readInt(&curdata);
	else
	{
		topic->data.short_name[0] = readChar(&curdata);
		topic->data.short_name[1] = readChar(&curdata);
	}
	*packetid = readInt(&curdata);

	if (topic->type == MQTTSN_TOPIC_TYPE_NORMAL && *qos == 3)
	{
		if (!buf_avail(curdata, enddata, topic->data.long_.len))
			goto exit;
		topic->data.long_.name = (char*)curdata;
		curdata += topic->data.long_.len;
	}

	*payloadlen = enddata - curdata;
	*payload = curdata;
	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTSNDeserialize_puback(unsigned short* topicid, unsigned short* packetid,
		unsigned char* returncode, unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;
	int lenlen;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
		goto exit;
	curdata += lenlen;
	enddata = buf + mylen;
	/* type(1) + topicid(2) + packetid(2) + returncode(1) */
	if (!buf_avail(curdata, enddata, 6))
		goto exit;

	if (readChar(&curdata) != MQTTSN_PUBACK)
		goto exit;

	*topicid = readInt(&curdata);
	*packetid = readInt(&curdata);
	*returncode = readChar(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into an ack
  * @param packettype returned integer - the MQTT packet type
  * @param packetid returned integer - the MQTT packet identifier
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTSNDeserialize_ack(unsigned char* type, unsigned short* packetid, unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;
	int lenlen;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
		goto exit;
	curdata += lenlen;
	enddata = buf + mylen;
	/* type(1) + packetid(2) */
	if (!buf_avail(curdata, enddata, 3))
		goto exit;

	*type = readChar(&curdata);
	if (*type != MQTTSN_PUBREL && *type != MQTTSN_PUBREC && *type != MQTTSN_PUBCOMP)
		goto exit;

	*packetid = readInt(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into register data
  * @param topicid returned topic id
  * @param packetid returned integer - the MQTT packet identifier
  * @param topicName returned MQTTSNString - the MQTT topic in the register
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int MQTTSNDeserialize_register(unsigned short* topicid, unsigned short* packetid, MQTTSNString* topicname,
		unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;
	int lenlen;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
		goto exit;
	curdata += lenlen;
	enddata = buf + mylen;
	/* type(1) + topicid(2) + packetid(2) */
	if (!buf_avail(curdata, enddata, 5))
		goto exit;

	if (readChar(&curdata) != MQTTSN_REGISTER)
		goto exit;

	*topicid = readInt(&curdata);
	*packetid = readInt(&curdata);

	topicname->lenstring.data = (char*)curdata;
	topicname->lenstring.len = enddata - curdata;
	topicname->cstring = NULL;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into register data
  * @param topicid returned topic id
  * @param packetid returned integer - the MQTT packet identifier
  * @param return_code returned integer return code
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int MQTTSNDeserialize_regack(unsigned short* topicid, unsigned short* packetid, unsigned char* return_code,
		unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen = 0;
	int lenlen;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
		goto exit;
	curdata += lenlen;
	enddata = buf + mylen;
	/* type(1) + topicid(2) + packetid(2) + return_code(1) */
	if (!buf_avail(curdata, enddata, 6))
		goto exit;

	if (readChar(&curdata) != MQTTSN_REGACK)
		goto exit;

	*topicid = readInt(&curdata);
	*packetid = readInt(&curdata);
	*return_code = readChar(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
