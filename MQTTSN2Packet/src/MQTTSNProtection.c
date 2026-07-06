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
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

/*
 * Serialization and deserialization of the MQTT-SN 2.0 Protection
 * Encapsulation packet (Section 3.17).
 *
 * Wire format (Section 3.17 / Figure 3-28):
 *
 *   Length          (1 or 3 bytes — variable, covers the entire packet)
 *   Packet Type     (1 byte  — 0xFF = MQTTSN_PROTECTION)
 *   Protection Flags(1 byte  — bits 1-0: Counter Length,
 *                              bits 3-2: Crypto Material Length,
 *                              bits 7-4: Authentication Tag Length)
 *   Protection Scheme (1 byte)
 *   Sender Identifier (8 bytes — always present)
 *   Random            (4 bytes — always present)
 *   Cryptographic Material (0, 2, 4, or 12 bytes — Crypto Length field)
 *   Monotonic Counter     (0, 2, or 4 bytes — Counter Length field)
 *   Protected MQTT-SN Packet (variable — inner packet with its own length)
 *   Authentication Tag    (variable — remaining bytes after inner packet)
 *
 * This library handles only the wire envelope.  Cryptographic computation
 * (key derivation, encryption/decryption, tag generation/verification) is
 * the caller's responsibility.
 */

#include "MQTTSNPacket.h"
#include "MQTTSNProtection.h"
#include "StackTrace.h"

#include <string.h>
#include <assert.h>

/*
 * Protection Flags bit positions
 */
#define PROTECTION_FLAGS_COUNTER_LEN_SHIFT   0u
#define PROTECTION_FLAGS_CRYPTO_LEN_SHIFT    2u
#define PROTECTION_FLAGS_AUTH_TAG_LEN_SHIFT  4u

/*
 * Cryptographic Material byte counts indexed by Crypto Length field value
 * (bits 3-2 of Protection Flags).
 *   0x0 → 0 bytes (absent)
 *   0x1 → 2 bytes (16 bits)
 *   0x2 → 4 bytes (32 bits)
 *   0x3 → 12 bytes (96 bits)
 */
static const uint8_t cryptoMaterialLens[4] = { 0u, 2u, 4u, 12u };

/*
 * Monotonic Counter byte counts indexed by Counter Length field value
 * (bits 1-0 of Protection Flags).
 *   0x0 → 0 bytes (absent)
 *   0x1 → 2 bytes (16 bits)
 *   0x2 → 4 bytes (32 bits)
 *   0x3 → 0 bytes (reserved; [MQTT-SN-3.17.2.1-1] MUST NOT be used)
 */
static const uint8_t counterLens[4] = { 0u, 2u, 4u, 0u };


/**
  * Returns the number of bytes of Cryptographic Material for a given
  * Crypto Length field value (bits 3-2 of the Protection Flags).
  */
uint8_t MQTTSNProtection_cryptoMaterialLen(uint8_t crypto_len_field)
{
	return cryptoMaterialLens[crypto_len_field & 0x03u];
}


/**
  * Returns the number of bytes of Monotonic Counter for a given Counter
  * Length field value (bits 1-0 of the Protection Flags).
  * Returns 0 for the reserved value 0x3.
  */
uint8_t MQTTSNProtection_counterLen(uint8_t counter_len_field)
{
	return counterLens[counter_len_field & 0x03u];
}


/**
  * Determines the body length of a Protection Encapsulation packet,
  * excluding the variable-length length field itself.
  */
static int32_t MQTTSNSerialize_protectionLength(
		const MQTTSNPacket_protectionData* data)
{
	/* type(1) + flags(1) + scheme(1) + senderid(8) + random(4) = 15 */
	int32_t len = 15;

	uint8_t counter_len_field =
		(data->flags.all >> PROTECTION_FLAGS_COUNTER_LEN_SHIFT) & 0x03u;
	uint8_t crypto_len_field  =
		(data->flags.all >> PROTECTION_FLAGS_CRYPTO_LEN_SHIFT)  & 0x03u;

	len += (int32_t)cryptoMaterialLens[crypto_len_field];
	len += (int32_t)counterLens[counter_len_field];
	len += (int32_t)data->protectedPacket.len;
	len += (int32_t)data->authTag.len;

	return len;
}


/**
  * Serializes a Protection Encapsulation packet into the supplied buffer,
  * ready for sending.
  *
  * The caller is responsible for:
  *   - setting data->flags correctly for the variable fields present
  *   - providing data->cryptoMaterial.len matching the Crypto Length flag
  *   - providing data->monotonicCounter.len matching the Counter Length flag
  *   - providing the already-serialized inner packet in data->protectedPacket
  *   - providing the pre-computed authentication tag in data->authTag
  *
  * Assertions validate incoming flag/length consistency but are omitted from
  * the outgoing path so that tests can send invalid data for error testing.
  *
  * @param buf    the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param data   the protection data to serialize
  * @return the length of the serialized data.  <= 0 indicates error
  */
int32_t MQTTSNSerialize_protection(uint8_t* buf, int32_t buflen,
		const MQTTSNPacket_protectionData* data)
{
	uint8_t  *ptr = buf;
	int32_t  len = 0;
	int32_t  rc = 0;

	FUNC_ENTRY;
	if ((len = MQTTSNPacket_len(
	             (uint16_t)MQTTSNSerialize_protectionLength(data))) > buflen)
	{
		rc = MQTTSNPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	ptr += MQTTSNPacket_encode(ptr, (uint16_t)len); /* write length */
	writeChar(&ptr, (unsigned int)MQTTSN_PROTECTION);             /* write packet type */

	writeChar(&ptr, (char)data->flags.all);         /* Protection Flags  */
	writeChar(&ptr, (char)data->scheme);            /* Protection Scheme */

	/* Sender Identifier — always 8 bytes (Section 3.17.4) */
	memcpy(ptr, data->senderid, MQTTSN_PROTECTION_SENDER_ID_LEN);
	ptr += MQTTSN_PROTECTION_SENDER_ID_LEN;

	/* Random — always 4 bytes (Section 3.17.5) */
	memcpy(ptr, data->random, MQTTSN_PROTECTION_RANDOM_LEN);
	ptr += MQTTSN_PROTECTION_RANDOM_LEN;

	/* Cryptographic Material — 0, 2, 4 or 12 bytes (Section 3.17.6) */
	if (data->cryptoMaterial.len > 0)
	{
		memcpy(ptr, data->cryptoMaterial.data, data->cryptoMaterial.len);
		ptr += data->cryptoMaterial.len;
	}

	/* Monotonic Counter — 0, 2 or 4 bytes (Section 3.17.7) */
	if (data->monotonicCounter.len > 0)
	{
		memcpy(ptr, data->monotonicCounter.data, data->monotonicCounter.len);
		ptr += data->monotonicCounter.len;
	}

	/* Protected MQTT-SN Packet — self-describing inner packet (Section 3.17.8) */
	memcpy(ptr, data->protectedPacket.data, data->protectedPacket.len);
	ptr += data->protectedPacket.len;

	/* Authentication Tag — length caller-supplied (Section 3.17.9) */
	memcpy(ptr, data->authTag.data, data->authTag.len);
	ptr += data->authTag.len;

	rc = (int32_t)(ptr - buf);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
  * Deserializes a Protection Encapsulation packet from the supplied wire buffer.
  *
  * All variable-length fields in the output structure are set as zero-copy
  * pointers into buf.  buf must remain valid for the lifetime of the structure.
  *
  * The inner packet length is determined from its own embedded length field,
  * not from the Protection Flags.  The authentication tag is whatever bytes
  * remain after the inner packet.  This means the tag length does not need to
  * be known in advance — the deserializer can handle provider-defined lengths
  * (Authentication Tag Length = 0x0) as well as all other values.
  *
  * The caller is responsible for verifying the authentication tag and, for
  * non-authentication-only schemes, decrypting the inner packet.
  *
  * @param data   the structure to fill with parsed packet fields
  * @param buf    the raw buffer data
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int32_t MQTTSNDeserialize_protection(MQTTSNPacket_protectionData* data,
		uint8_t* buf, int32_t buflen)
{
	uint8_t  *curdata = buf;
	uint8_t  *enddata = NULL;
	int32_t  rc = 0;
	int32_t  mylen = 0;
	uint8_t  counter_len_field;
	uint8_t  crypto_len_field;
	uint8_t  auth_tag_len_field;
	uint8_t  counter_bytes;
	uint8_t  crypto_bytes;
	uint16_t inner_pkt_len;

	FUNC_ENTRY;
	int32_t lenlen = MQTTSNPacket_decode(curdata, buflen, &mylen); /* read length */
	if (lenlen < 0)
		goto exit;
	curdata += lenlen;
	enddata = buf + mylen;
	if (enddata - curdata > buflen)
		goto exit;

	if ((uint8_t)readChar(&curdata) != MQTTSN_PROTECTION)
		goto exit;

	/* Protection Flags (Section 3.17.2) */
	data->flags.all    = (uint8_t)readChar(&curdata);
	counter_len_field  = (data->flags.all >> PROTECTION_FLAGS_COUNTER_LEN_SHIFT) & 0x03u;
	crypto_len_field   = (data->flags.all >> PROTECTION_FLAGS_CRYPTO_LEN_SHIFT)  & 0x03u;
	auth_tag_len_field = (data->flags.all >> PROTECTION_FLAGS_AUTH_TAG_LEN_SHIFT) & 0x0Fu;

	/* validate reserved flag values */
	if (counter_len_field == 0x3u)
		goto exit; /* [MQTT-SN-3.17.2.1-1] Counter Length value 0x3 is reserved */
	if (auth_tag_len_field == 0x2u || auth_tag_len_field == 0x3u)
		goto exit; /* [MQTT-SN-3.17.2.3-3] Authentication Tag Length values 0x2 and 0x3 are reserved */

	counter_bytes = counterLens[counter_len_field];
	crypto_bytes  = cryptoMaterialLens[crypto_len_field];
	(void)auth_tag_len_field; /* length is inferred from packet length; see below */

	/* Protection Scheme (Section 3.17.3) */
	data->scheme = (uint8_t)readChar(&curdata);

	/* Sender Identifier — always 8 bytes (Section 3.17.4) */
	memcpy(data->senderid, curdata, MQTTSN_PROTECTION_SENDER_ID_LEN);
	curdata += MQTTSN_PROTECTION_SENDER_ID_LEN;

	/* Random — always 4 bytes (Section 3.17.5) */
	memcpy(data->random, curdata, MQTTSN_PROTECTION_RANDOM_LEN);
	curdata += MQTTSN_PROTECTION_RANDOM_LEN;

	/* Cryptographic Material — zero-copy (Section 3.17.6) */
	data->cryptoMaterial.len  = crypto_bytes;
	data->cryptoMaterial.data = (crypto_bytes > 0) ? curdata : NULL;
	curdata += crypto_bytes;

	/* Monotonic Counter — zero-copy (Section 3.17.7) */
	data->monotonicCounter.len  = counter_bytes;
	data->monotonicCounter.data = (counter_bytes > 0) ? curdata : NULL;
	curdata += counter_bytes;

	/* Protected MQTT-SN Packet (Section 3.17.8).
	 *
	 * The inner packet is self-describing: its length is encoded in its own
	 * leading length field (1 byte when < 256, 3 bytes when >= 256 with the
	 * first byte set to 0x01).  Read that length to find where the inner
	 * packet ends and the Authentication Tag begins.
	 */
	if (curdata >= enddata)
		goto exit; /* no inner packet */

	if (curdata[0] == 0x01u) /* 3-byte length field */
		inner_pkt_len = (uint16_t)(((uint16_t)curdata[1] << 8) | curdata[2]);
	else                     /* 1-byte length field */
		inner_pkt_len = (uint16_t)curdata[0];

	if (curdata + inner_pkt_len > enddata)
		goto exit; /* inner packet overruns buffer */

	data->protectedPacket.len  = inner_pkt_len;
	data->protectedPacket.data = curdata;
	curdata += inner_pkt_len;

	/* Authentication Tag — zero-copy, fills to end of packet (Section 3.17.9).
	 *
	 * The tag length is whatever bytes remain.  This handles all Authentication
	 * Tag Length field values including 0x0 (provider-defined) without needing
	 * scheme-specific knowledge in the deserializer.
	 */
	data->authTag.len  = (uint16_t)(enddata - curdata);
	data->authTag.data = (data->authTag.len > 0) ? curdata : NULL;

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
