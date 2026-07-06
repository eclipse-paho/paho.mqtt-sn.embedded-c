/*******************************************************************************
 * Copyright (c) 2014, 2026 IBM Corp., Ian Craggs
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
 *******************************************************************************/

#include "MQTTSNPacket.h"
#include "StackTrace.h"

#include <string.h>


/**
  * Serializes the supplied advertise data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param radius the broadcast radius of this message
  * @param duration - the time interval until the next advertise will be sent
  * @return the length of the serialized data.  <= 0 indicates error
  */
int MQTTSNSerialize_advertise(unsigned char* buf, int buflen, unsigned char gatewayid, unsigned short duration)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(4)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len);   /* write length */
	writeChar(&ptr, MQTTSN_ADVERTISE);      /* write message type */

	writeChar(&ptr, gatewayid);
	writeInt(&ptr, duration);

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into searchgw data
  * @param radius the returned broadcast radius of this message
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int MQTTSNDeserialize_searchgw(unsigned char* radius, unsigned char* buf, int buflen)
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
	if (enddata - curdata > buflen)
		goto exit;

	if (readChar(&curdata) != MQTTSN_SEARCHGW)
		goto exit;

	*radius = readChar(&curdata);

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes the supplied gwinfo data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param gatewayid the gateway id
  * @param gatewayaddress_len the optional length of the gateway address (0 if none)
  * @param gatewayaddress the optional gateway address (NULL if none)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int MQTTSNSerialize_gwinfo(unsigned char* buf, int buflen, unsigned char gatewayid, unsigned short gatewayaddress_len,
		unsigned char* gatewayaddress)
{
	unsigned char *ptr = buf;
	int len = 0;
	int rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(2 + gatewayaddress_len)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, len);   /* write length */
	writeChar(&ptr, MQTTSN_GWINFO);      /* write message type */

	writeChar(&ptr, gatewayid);
	if (gatewayaddress_len > 0 && gatewayaddress != NULL)
	{
		memcpy(ptr, gatewayaddress, gatewayaddress_len);
		ptr += gatewayaddress_len;
	}

	rc = ptr - buf;
exit:
	FUNC_EXIT_RC(rc);
	return rc;

}



