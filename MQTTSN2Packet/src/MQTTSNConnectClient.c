/*******************************************************************************
 * Copyright (c) 2014, 2026 Ian Craggs, IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v2.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
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
  * Determines the length of the MQTT connect packet that would be produced using the supplied connect options.
  * @param options the options to be used to build the connect packet
  * @return the length of buffer needed to contain the serialized version of the packet
  */
int MQTTSNSerialize_connectLength(MQTTSNPacket_connectData* options)
{
	int len = 9; /* length of compulsory fields */

	FUNC_ENTRY;
	if (options->flags.bits.will)
		len += 1;
	if (options->flags.bits.defaultAwakeMessages)
		len += 1;
	if (options->flags.bits.sessionExpiryInterval)
		len += 4;
	if (options->flags.bits.will)
	{
		len += 4; /* Will Topic Alias or Length (2) + payload length (2) */
		if (options->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_NAME)
			len += options->will.topic.alt.string.len;
		len += options->will.payload.len;
	}

	if (options->flags.bits.auth)
		len += 3 + options->auth.method.len + options->auth.data.len;

	len += options->clientID.len;

	len += (len > 255) ? 3 : 1; /* length field is 3 bytes for packet lengths > 255 */

	FUNC_EXIT_RC(len);
	return len;
}


/**
  * Serializes the connect options into the buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param options the options to be used to build the connect packet
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_connect(unsigned char* buf, int buflen, MQTTSNPacket_connectData* options)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = -1;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNSerialize_connectLength(options))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, MQTTSN_CONNECT);      /* write message type */

	writeChar(&ptr, options->flags.all);
	if (options->flags.bits.will)
		writeChar(&ptr, options->willFlags.all);

	writeInt16(&ptr, options->packetId);
	writeChar(&ptr, 0x02);        /* protocol version */
	writeInt16(&ptr, options->keepAlive);
	writeInt16(&ptr, options->maxPacketSize);
	if (options->flags.bits.defaultAwakeMessages)
		writeChar(&ptr, options->defaultAwakeMessages);
	if (options->flags.bits.sessionExpiryInterval)
		writeInt32(&ptr, options->sessionExpiryInterval);
	if (options->flags.bits.will)
	{
		if (options->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_SESSION ||
			options->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_PREDEFINED)
 			writeInt16(&ptr, options->will.topic.alt.alias);
		else
			writeMQTTSNString(&ptr, options->will.topic.alt.string, true);
		writeMQTTSNData(&ptr, options->will.payload, true);
	}
	if (options->flags.bits.auth)
	{
		options->auth.method.islen8 = 1; /* auth method has a 1 byte length */
		writeMQTTSNString(&ptr, options->auth.method, true);
		writeMQTTSNData(&ptr, options->auth.data, true);
	}

	writeMQTTSNString(&ptr, options->clientID, false);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into connack data - return code
  * @param connack_rc returned integer value of the connack return code
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTSNDeserialize_connack(MQTTSNPacket_connackData* data, unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen;

	FUNC_ENTRY;
	curdata += (rc = MQTTSNPacket_decode(curdata, buflen, &mylen)); /* read length */
	enddata = buf + mylen;
	if (enddata - buf < 3)
		goto exit;

	if (readChar(&curdata) != MQTTSN_CONNACK) /* Packet type */
		goto exit;

	data->flags.all = readChar(&curdata);
	data->packetId = readInt16(&curdata);
	data->reasonCode = readChar(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Determines the length of the MQTT disconnect packet (without length field)
  * @param duration the parameter used for the disconnect
  * @return the length of buffer needed to contain the serialized version of the packet
  */
int MQTTSNSerialize_disconnectLength(int duration)
{
	int len = 0;

	FUNC_ENTRY;
	len = (duration > 0) ? 3 : 1;
	FUNC_EXIT_RC(len);
	return len;
}


/**
  * Serializes a disconnect packet into the supplied buffer, ready for writing to a socket
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer, to avoid overruns
  * @param duration optional duration, not added to packet if < 0
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_disconnect(unsigned char* buf, int buflen, int duration)
{
	int rc = -1;
	unsigned char *ptr = buf;
	int len = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNSerialize_disconnectLength(duration))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, MQTTSN_DISCONNECT);      /* write message type */

	if (duration > 0)
		writeInt16(&ptr, duration);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a disconnect packet into the supplied buffer, ready for writing to a socket
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer, to avoid overruns
  * @param clientid optional string, not added to packet string == NULL
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_pingreq(unsigned char* buf, int buflen, MQTTSN_string clientid)
{
	int rc = -1;
	unsigned char *ptr = buf;
	int len = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(clientid.len + 1)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, MQTTSN_PINGREQ);      /* write message type */

	//writeMQTTSNString(&ptr, clientid);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTSNDeserialize_pingresp(unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen;

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