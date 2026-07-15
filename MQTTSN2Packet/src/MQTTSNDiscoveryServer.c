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

#include "MQTTSNPacket.h"
#include "MQTTSNDiscovery.h"
#include "StackTrace.h"


/**
  * Serializes the supplied ADVERTISE data into the supplied buffer, ready
  * for sending (Gateway/server transmit side).
  *
  * @param buf       the buffer into which the packet will be serialized
  * @param buflen    the length in bytes of the supplied buffer
  * @param gatewayId the Gateway Identifier (Section 3.20.1.2)
  * @param duration  time interval in seconds until the next ADVERTISE packet
  *                  from this Gateway (Section 3.20.1.3)
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_advertise(uint8_t* buf, int32_t buflen,
		uint8_t gatewayId, uint16_t duration)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	/* body: type(1) + gatewayId(1) + duration(2) = 4 bytes */
	if ((len = MQTTSNPacket_len(4u)) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_ADVERTISE);              /* write packet type */

	writeChar(&ptr, (char)gatewayId);
	writeInt16(&ptr, duration);

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into SEARCHGW data (server/
  * Gateway receive side).
  *
  * @param networkInfo returned - Additional Network Information
  *                    (Section 3.20.2.2); zero-copy pointer into buf; len is
  *                    0 when the field is absent from the wire
  * @param buf         the raw buffer data of the correct length
  * @param buflen      the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_searchgw(MQTTSN_data* networkInfo,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	uint8_t  *endbuffer = buf + buflen;
	int32_t  rc = 0;
	int32_t  mylen = 0;
	int32_t  lenlen;

	FUNC_ENTRY;
	lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
	{
		rc = MQTTSNPACKET_READ_ERROR;
		goto exit;
	}
	if (buflen < mylen)              /* packet longer than the supplied buffer */
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	curdata += lenlen;
	enddata = buf + mylen;
	if (!buf_avail(curdata, endbuffer, 1))              /* type(1) */
		goto exit;

	if (readChar(&curdata) != MQTTSN_SEARCHGW)
		goto exit;

	/* Additional Network Information: fills to end of packet, optional
	 * (Section 3.20.2.2)
	 */
	networkInfo->len  = (uint16_t)(enddata - curdata);
	networkInfo->data = curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes the supplied GWINFO data into the supplied buffer, ready for
  * sending (Gateway/server transmit side).
  *
  * @param buf            the buffer into which the packet will be serialized
  * @param buflen         the length in bytes of the supplied buffer
  * @param gatewayId      the Gateway Identifier (Section 3.20.3.2)
  * @param gatewayAddress Network Address of the Gateway (Section 3.20.3.3);
  *                       fills to the end of the packet, no length prefix;
  *                       pass a zero-length value (or NULL) when sent by a
  *                       Gateway - it is only included when sent by a Client
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_gwinfo(uint8_t* buf, int32_t buflen,
		uint8_t gatewayId, const MQTTSN_data* gatewayAddress)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;
	uint16_t addrlen = (gatewayAddress == NULL) ? 0 : gatewayAddress->len;

	FUNC_ENTRY;
	/* body: type(1) + gatewayId(1) + gatewayAddress(variable, optional) */
	if ((len = MQTTSNPacket_len((uint16_t)(2 + addrlen))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_GWINFO);                 /* write packet type */

	writeChar(&ptr, (char)gatewayId);

	if (addrlen > 0)
		writeMQTTSNData(&ptr, *gatewayAddress, false); /* fills to end of packet */

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
