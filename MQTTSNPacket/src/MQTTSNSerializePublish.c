/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
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
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include "MQTTSNPacket.h"
#include "StackTrace.h"

#include <string.h>


/**
  * Determines the length of the MQTT publish packet that would be produced using the supplied parameters
  * @param qos the MQTT QoS of the publish (packetid is omitted for QoS 0)
  * @param topicName the topic name to be used in the publish  
  * @param payloadlen the length of the payload to be sent
  * @return the length of buffer needed to contain the serialized version of the packet
  */
int MQTTSNSerialize_publishLength(int payloadlen, MQTTSN_topicid topic, int qos)
{
	int len = 6;

	if (topic.type == MQTTSN_TOPIC_TYPE_NORMAL && qos == 3)
		len += topic.data.long_.len;

	return payloadlen + len;
}


/**
  * Serializes the supplied publish data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param qos integer - the MQTT QoS value
  * @param retained integer - the MQTT retained flag
  * @param packetid integer - the MQTT packet identifier
  * @param topic MQTTSN_topicid - the MQTT topic in the publish
  * @param payload byte buffer - the MQTT publish payload
  * @param payloadlen integer - the length of the MQTT payload
  * @return the length of the serialized data.  <= 0 indicates error
  */
int MQTTSNSerialize_publish(unsigned char* buf, int buflen, unsigned char dup, int qos, unsigned char retained, unsigned short packetid,
		MQTTSN_topicid topic, unsigned char* payload, int payloadlen)
{
	unsigned char *ptr = buf;
	MQTTSNFlags flags;
	int len = 0;
	int rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNSerialize_publishLength(payloadlen, topic, qos))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, MQTTSN_PUBLISH);      /* write message type */

	flags.all = 0;
	flags.bits.dup = dup;
	flags.bits.QoS = qos;
	flags.bits.retain = retained;
	flags.bits.topicIdType = topic.type;
	writeChar(&ptr, flags.all);

	if (topic.type == MQTTSN_TOPIC_TYPE_NORMAL && qos == 3)
	{
		/* special arrangement for long topic names in QoS -1 publishes.  The length of the topic is in the topicid field */
		writeInt(&ptr, topic.data.long_.len); /* topic length */
	}
	else if (topic.type == MQTTSN_TOPIC_TYPE_NORMAL || topic.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
		writeInt(&ptr, topic.data.id);
	else
	{
		writeChar(&ptr, topic.data.short_name[0]);
		writeChar(&ptr, topic.data.short_name[1]);
	}
	writeInt(&ptr, packetid);
	if (topic.type == MQTTSN_TOPIC_TYPE_NORMAL && qos == 3)
	{
		memcpy(ptr, topic.data.long_.name, topic.data.long_.len);
		ptr += topic.data.long_.len;
	}
	memcpy(ptr, payload, payloadlen);
	ptr += payloadlen;

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTSNSerialize_puback(unsigned char* buf, int buflen, unsigned short topicid, unsigned short packetid,
		unsigned char returncode)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(6)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, MQTTSN_PUBACK);      /* write message type */

	writeInt(&ptr, topicid);
	writeInt(&ptr, packetid);
	writeChar(&ptr, returncode);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;

}



/**
  * Serializes the ack packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param type the MQTT-SN packet type
  * @param packetid the MQTT-SN packet identifier
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_ack(unsigned char* buf, int buflen, unsigned short packet_type, unsigned short packetid)
{
	int rc = 0;
	unsigned char *ptr = buf;
	int len = 4; /* ack packet length */

	FUNC_ENTRY;
	if (len > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, packet_type);      /* write packet type */

	writeInt(&ptr, packetid);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a puback packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param packetid integer - the MQTT packet identifier
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_pubrec(unsigned char* buf, int buflen, unsigned short packetid)
{
	return MQTTSNSerialize_ack(buf, buflen, MQTTSN_PUBREC, packetid);
}


/**
  * Serializes a pubrel packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param packetid integer - the MQTT packet identifier
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_pubrel(unsigned char* buf, int buflen, unsigned short packetid)
{
	return MQTTSNSerialize_ack(buf, buflen, MQTTSN_PUBREL, packetid);
}


/**
  * Serializes a pubrel packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param packetid integer - the MQTT packet identifier
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_pubcomp(unsigned char* buf, int buflen, unsigned short packetid)
{
	return MQTTSNSerialize_ack(buf, buflen, MQTTSN_PUBCOMP, packetid);
}


/**
  * Determines the length of the MQTT register packet that would be produced using the supplied parameters
  * @param topicnamelen the length of the topic name to be used in the register
  * @return the length of buffer needed to contain the serialized version of the packet
  */
int MQTTSNSerialize_registerLength(int topicnamelen)
{
	return topicnamelen + 5;
}

/**
  * Serializes the supplied register data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param topicid if sent by a gateway, contains the id for the topicname, otherwise 0
  * @param packetid integer - the MQTT packet identifier
  * @param topicname null-terminated topic name string
  * @return the length of the serialized data.  <= 0 indicates error
  */
int MQTTSNSerialize_register(unsigned char* buf, int buflen, unsigned short topicid, unsigned short packetid,
		MQTTSNString* topicname)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = 0;
	int topicnamelen = 0;

	FUNC_ENTRY;
	topicnamelen = (topicname->cstring) ? (int)strlen(topicname->cstring) : topicname->lenstring.len;
	if ((len = MQTTSNPacket_len(MQTTSNSerialize_registerLength(topicnamelen))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len);  /* write length */
	writeChar(&ptr, MQTTSN_REGISTER);      /* write message type */

	writeInt(&ptr, topicid);
	writeInt(&ptr, packetid);

	memcpy(ptr, (topicname->cstring) ? topicname->cstring : topicname->lenstring.data, topicnamelen);
	ptr += topicnamelen;

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes the supplied register data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param topicid if sent by a gateway, contains the id for the topicname, otherwise 0
  * @param packetid integer - the MQTT packet identifier
  * @param return_code integer return code
  * @return the length of the serialized data.  <= 0 indicates error
  */
int MQTTSNSerialize_regack(unsigned char* buf, int buflen, unsigned short topicid, unsigned short packetid,
		unsigned char return_code)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(6)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len);  /* write length */
	writeChar(&ptr, MQTTSN_REGACK);      /* write message type */

	writeInt(&ptr, topicid);
	writeInt(&ptr, packetid);
	writeChar(&ptr, return_code);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
