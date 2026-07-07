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

#include "MQTTSNPacket.h"
#include "StackTrace.h"

#include <string.h>

/**
  * Determines the length of the MQTTSN subscribe packet that would be produced using the supplied parameters, 
  * excluding length
  * @param topicName the topic name to be used in the publish  
  * @return the length of buffer needed to contain the serialized version of the packet
  */
int MQTTSNSerialize_subscribeLength(MQTTSN_topicid* topicFilter)
{
	int len = 4;

	if (topicFilter->type == MQTTSN_TOPIC_TYPE_NORMAL)
		len += topicFilter->data.long_.len;
	else if (topicFilter->type == MQTTSN_TOPIC_TYPE_SHORT || topicFilter->type == MQTTSN_TOPIC_TYPE_PREDEFINED)
		len += 2;

	return len;
}


/**
  * Serializes the supplied subscribe data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param dup integer - the MQTT-SN dup flag
  * @param qos integer - the MQTT-SN QoS value
  * @param packetid integer - the MQTT-SN packet identifier
  * @param topic MQTTSN_topicid - the MQTT-SN topic in the subscribe
  * @return the length of the serialized data.  <= 0 indicates error
  */
int MQTTSNSerialize_subscribe(unsigned char* buf, int buflen, unsigned char dup, int qos, unsigned short packetid,
		MQTTSN_topicid* topicFilter)
{
	unsigned char *ptr = buf;
	MQTTSNFlags flags;
	int len = 0;
	int rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNSerialize_subscribeLength(topicFilter))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len);   /* write length */
	writeChar(&ptr, MQTTSN_SUBSCRIBE);      /* write message type */

	flags.all = 0;
	flags.bits.dup = dup;
	flags.bits.QoS = qos;
	flags.bits.topicIdType = topicFilter->type;
	writeChar(&ptr, flags.all);

	writeInt(&ptr, packetid);

	/* now the topic id or name */
	if (topicFilter->type == MQTTSN_TOPIC_TYPE_NORMAL) /* means long topic name */
	{
		memcpy(ptr, topicFilter->data.long_.name, topicFilter->data.long_.len);
		ptr += topicFilter->data.long_.len;
	}
	else if (topicFilter->type == MQTTSN_TOPIC_TYPE_PREDEFINED)
		writeInt(&ptr, topicFilter->data.id);
	else if (topicFilter->type == MQTTSN_TOPIC_TYPE_SHORT)
	{
		writeChar(&ptr, topicFilter->data.short_name[0]);
		writeChar(&ptr, topicFilter->data.short_name[1]);
	}

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;

}


/**
  * Deserializes the supplied (wire) buffer into suback data
  * @param qos the returned qos
  * @param topicid returned if "accepted" the value which will be used by the gateway in subsequent PUBLISH packets
  * @param packetid returned - the same value as the one contained in the corresponding SUBSCRIBE
  * @param returncode returned - "accepted" or rejection reason
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int MQTTSNDeserialize_suback(int* qos, unsigned short* topicid, unsigned short* packetid,
		unsigned char* returncode, unsigned char* buf, int buflen)
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
	/* type(1) + flags(1) + topicid(2) + packetid(2) + returncode(1) */
	if (!buf_avail(curdata, enddata, 7))
		goto exit;

	if (readChar(&curdata) != MQTTSN_SUBACK)
		goto exit;

	flags.all = readChar(&curdata);
	*qos = flags.bits.QoS;

	*topicid = readInt(&curdata);
	*packetid = readInt(&curdata);
	*returncode = readChar(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}



