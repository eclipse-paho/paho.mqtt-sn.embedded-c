/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
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
 *    Nicholas Humfrey - Reformatting to make more consistent; bug 453862
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
	int len = 0;

	FUNC_ENTRY;
	len = 5 + MQTTSNstrlen(options->clientID);
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
	MQTTSNFlags flags;
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

	flags.all = 0;
	flags.bits.cleanSession = options->cleansession;
	flags.bits.will = options->willFlag;
	writeChar(&ptr, flags.all);
	writeChar(&ptr, 0x01);                 /* protocol ID */
	writeInt(&ptr, options->duration);
	writeMQTTSNString(&ptr, options->clientID);

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
int MQTTSNDeserialize_connack(int* connack_rc, unsigned char* buf, int buflen)
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

	if (readChar(&curdata) != MQTTSN_CONNACK)
		goto exit;

	*connack_rc = readChar(&curdata);

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
		writeInt(&ptr, duration);

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
int MQTTSNSerialize_pingreq(unsigned char* buf, int buflen, MQTTSNString clientid)
{
	int rc = -1;
	unsigned char *ptr = buf;
	int len = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNstrlen(clientid) + 1)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, MQTTSN_PINGREQ);      /* write message type */

	writeMQTTSNString(&ptr, clientid);

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


/**
  * Serializes a willtopic or willtopicupd packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param willQoS the qos of the will message
  * @param willRetain the retained flag of the will message
  * @param willTopic the topic of the will message
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_willtopic1(unsigned char* buf, int buflen, int willQoS, unsigned char willRetain, MQTTSNString willTopic,
		enum MQTTSN_msgTypes packet_type)
{
	unsigned char *ptr = buf;
	MQTTSNFlags flags;
	int len = 0;
	int rc = -1;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNstrlen(willTopic) + 2)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, packet_type);      /* write message type */

	flags.all = 0;
	flags.bits.QoS = willQoS;
	flags.bits.retain = willRetain;
	writeChar(&ptr, flags.all);

	writeMQTTSNString(&ptr, willTopic);

	rc = ptr - buf;

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a willtopicupd packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param willQoS the qos of the will message
  * @param willRetain the retained flag of the will message
  * @param willTopic the topic of the will message
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_willtopicupd(unsigned char* buf, int buflen, int willQoS, unsigned char willRetain, MQTTSNString willTopic)
{
	return MQTTSNSerialize_willtopic1(buf, buflen, willQoS, willRetain, willTopic, MQTTSN_WILLTOPICUPD);
}


/**
  * Serializes a willtopic packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param willQoS the qos of the will message
  * @param willRetain the retained flag of the will message
  * @param willTopic the topic of the will message
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_willtopic(unsigned char* buf, int buflen, int willQoS, unsigned char willRetain, MQTTSNString willTopic)
{
	return MQTTSNSerialize_willtopic1(buf, buflen, willQoS, willRetain, willTopic, MQTTSN_WILLTOPIC);
}


/**
  * Serializes a willmsg or willmsgupd packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param willMsg the will message
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_willmsg1(unsigned char* buf, int buflen, MQTTSNString willMsg, enum MQTTSN_msgTypes packet_type)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = -1;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(MQTTSNstrlen(willMsg) + 1)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len); /* write length */
	writeChar(&ptr, packet_type);      /* write message type */

	writeMQTTSNString(&ptr, willMsg);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes a willmsg packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffersage
  * @param willMsg the will message
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_willmsg(unsigned char* buf, int buflen, MQTTSNString willMsg)
{
	return MQTTSNSerialize_willmsg1(buf, buflen, willMsg, MQTTSN_WILLMSG);
}


/**
  * Serializes a willmsgupd packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffersage
  * @param willMsg the will message
  * @return serialized length, or error if 0
  */
int MQTTSNSerialize_willmsgupd(unsigned char* buf, int buflen, MQTTSNString willMsg)
{
	return MQTTSNSerialize_willmsg1(buf, buflen, willMsg, MQTTSN_WILLMSGUPD);
}


/**
  * Deserializes the supplied (wire) buffer
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTSNDeserialize_willtopicreq(unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = -1;
	int mylen;

	FUNC_ENTRY;
	if (MQTTSNPacket_decode(curdata++, buflen, &mylen) != 1) /* read length */
		goto exit;
	if (mylen > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	enddata = buf + mylen;
	if (enddata - curdata < 1)
		goto exit;

	if (readChar(&curdata) != MQTTSN_WILLTOPICREQ)
		goto exit;

	rc = 1;
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
int MQTTSNDeserialize_willmsgreq(unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = -1;
	int mylen;

	FUNC_ENTRY;
	if (MQTTSNPacket_decode(curdata++, buflen, &mylen) != 1) /* read length */
		goto exit;
	if (mylen > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	enddata = buf + mylen;
	if (enddata - curdata < 1)
		goto exit;

	if (readChar(&curdata) != MQTTSN_WILLMSGREQ)
		goto exit;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into willtopicresp data - return code
  * @param connack_rc returned integer value of the return code
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTSNDeserialize_willtopicresp(int* resp_rc, unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - buf < 3)
		goto exit;

	if (readChar(&curdata) != MQTTSN_WILLTOPICRESP)
		goto exit;

	*resp_rc = readChar(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into willmsgresp data - return code
  * @param connack_rc returned integer value of the return code
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTSNDeserialize_willmsgresp(int* resp_rc, unsigned char* buf, int buflen)
{
	unsigned char* curdata = buf;
	unsigned char* enddata = NULL;
	int rc = 0;
	int mylen;

	FUNC_ENTRY;
	curdata += MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	enddata = buf + mylen;
	if (enddata - buf < 3)
		goto exit;

	if (readChar(&curdata) != MQTTSN_WILLMSGRESP)
		goto exit;

	*resp_rc = readChar(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
