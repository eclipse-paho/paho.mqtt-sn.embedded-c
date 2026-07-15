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
  * Deserializes the supplied (wire) buffer into ADVERTISE data (client
  * receive side).
  *
  * @param gatewayId returned - the Gateway Identifier (Section 3.20.1.2)
  * @param duration  returned - time interval in seconds until the next
  *                  ADVERTISE packet from this Gateway (Section 3.20.1.3)
  * @param buf       the raw buffer data of the correct length
  * @param buflen    the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_advertise(uint8_t* gatewayId, uint16_t* duration,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
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
	/* type(1) + gatewayId(1) + duration(2) = 4 bytes */
	if (!buf_avail(curdata, endbuffer, 4))
		goto exit;

	if (readChar(&curdata) != MQTTSN_ADVERTISE)
		goto exit;

	*gatewayId = (uint8_t)readChar(&curdata);   /* Section 3.20.1.2 */
	*duration  = readInt16(&curdata);           /* Section 3.20.1.3 */

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Serializes the supplied SEARCHGW data into the supplied buffer, ready for
  * sending (client transmit side).
  *
  * @param buf         the buffer into which the packet will be serialized
  * @param buflen      the length in bytes of the supplied buffer
  * @param networkInfo Additional Network Information (Section 3.20.2.2);
  *                    fills to the end of the packet, no length prefix;
  *                    pass a zero-length value (or NULL) to omit it
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_searchgw(uint8_t* buf, int32_t buflen,
		const MQTTSN_data* networkInfo)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;
	uint16_t infolen = (networkInfo == NULL) ? 0 : networkInfo->len;

	FUNC_ENTRY;
	/* body: type(1) + networkInfo(variable, optional) */
	if ((len = MQTTSNPacket_len((uint16_t)(1 + infolen))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, MQTTSN_SEARCHGW);               /* write packet type */

	if (infolen > 0)
		writeMQTTSNData(&ptr, *networkInfo, false); /* fills to end of packet */

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes the supplied (wire) buffer into GWINFO data (client receive
  * side).
  *
  * @param gatewayId      returned - the Gateway Identifier (Section 3.20.3.2)
  * @param gatewayAddress returned - Network Address of the Gateway
  *                       (Section 3.20.3.3); zero-copy pointer into buf;
  *                       len is 0 when the field is absent from the wire
  *                       (i.e. the packet was sent by a Gateway)
  * @param buf            the raw buffer data of the correct length
  * @param buflen         the length in bytes of the data in the supplied
  *                       buffer
  * @return error code.  1 is success
  */
int32_t MQTTSNDeserialize_gwinfo(uint8_t* gatewayId, MQTTSN_data* gatewayAddress,
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
	/* type(1) + gatewayId(1) = 2 bytes */
	if (!buf_avail(curdata, endbuffer, 2))
		goto exit;

	if (readChar(&curdata) != MQTTSN_GWINFO)
		goto exit;

	*gatewayId = (uint8_t)readChar(&curdata);   /* Section 3.20.3.2 */

	/* Gateway Address: fills to end of packet, optional (Section 3.20.3.3) */
	gatewayAddress->len  = (uint16_t)(enddata - curdata);
	gatewayAddress->data = curdata;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
