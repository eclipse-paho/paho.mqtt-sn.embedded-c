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

#if !defined(MQTTSNPROTECTION_H_)
#define MQTTSNPROTECTION_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_data */

/* -------------------------------------------------------------------------
 * Protection Flags (Section 3.17.2)
 *
 * Bit 7-4 : Authentication Tag Length
 * Bit 3-2 : Cryptographic Material Length  (Crypto Length)
 * Bit 1-0 : Monotonic Counter Length       (Counter Length)
 * ------------------------------------------------------------------------- */

typedef union MQTTSNProtection_flags MQTTSNProtection_flags;
union MQTTSNProtection_flags
{
	uint8_t all;
#if defined(REVERSED)
	struct protectionFlagsBits
	{
		uint8_t authTagLen : 4;    /**< bits 7-4 */
		uint8_t cryptoLen  : 2;    /**< bits 3-2 */
		uint8_t counterLen : 2;    /**< bits 1-0 */
	} bits;
#else
	struct protectionFlagsBits
	{
		uint8_t counterLen : 2;    /**< bits 1-0: Monotonic Counter Length  */
		uint8_t cryptoLen  : 2;    /**< bits 3-2: Crypto Material Length    */
		uint8_t authTagLen : 4;    /**< bits 7-4: Authentication Tag Length */
	} bits;
#endif
};

/* -------------------------------------------------------------------------
 * Counter Length values (bits 1-0)
 * ------------------------------------------------------------------------- */
#define MQTTSN_PROTECTION_COUNTER_NONE  0x0u  /**< No counter                       */
#define MQTTSN_PROTECTION_COUNTER_16    0x1u  /**< 2-byte (16-bit) counter          */
#define MQTTSN_PROTECTION_COUNTER_32    0x2u  /**< 4-byte (32-bit) counter          */
/* 0x3 is reserved; MUST NOT be used [MQTT-SN-3.17.2.1-1]                           */

/* -------------------------------------------------------------------------
 * Crypto Material Length values (bits 3-2)
 * ------------------------------------------------------------------------- */
#define MQTTSN_PROTECTION_CRYPTO_NONE   0x0u  /**< No cryptographic material        */
#define MQTTSN_PROTECTION_CRYPTO_16     0x1u  /**< 2-byte crypto material           */
#define MQTTSN_PROTECTION_CRYPTO_32     0x2u  /**< 4-byte crypto material           */
#define MQTTSN_PROTECTION_CRYPTO_96     0x3u  /**< 12-byte crypto material          */

/* -------------------------------------------------------------------------
 * Authentication Tag Length values (bits 7-4)
 * ------------------------------------------------------------------------- */
#define MQTTSN_PROTECTION_AUTHTAG_PROVIDER  0x0u /**< provider-defined length       */
#define MQTTSN_PROTECTION_AUTHTAG_NOMINAL   0x1u /**< scheme nominal tag size       */
/* 0x2 and 0x3 are reserved; MUST NOT be used [MQTT-SN-3.17.2.3-3]                 */
/* 0x4..0xF: auth-only only; tag = (value * 16) bits [MQTT-SN-3.17.2.3-6]          */

/* -------------------------------------------------------------------------
 * Protection Schemes (Section 3.17.3 / Figure 3-29)
 * ------------------------------------------------------------------------- */
enum MQTTSN_protectionSchemes
{
	/* Authentication-only schemes */
	MQTTSN_PROTECTION_HMAC_SHA256       = 0x00,
	MQTTSN_PROTECTION_HMAC_SHA3_256     = 0x01,
	MQTTSN_PROTECTION_CMAC_128          = 0x02,
	MQTTSN_PROTECTION_CMAC_192          = 0x03,
	MQTTSN_PROTECTION_CMAC_256          = 0x04,
	/* 0x05-0x3B: reserved */
	/* 0x3C-0x3F: provider-defined authentication-only */

	/* AEAD schemes */
	MQTTSN_PROTECTION_AES_CCM_64_128    = 0x40,
	MQTTSN_PROTECTION_AES_CCM_64_192    = 0x41,
	MQTTSN_PROTECTION_AES_CCM_64_256    = 0x42,
	MQTTSN_PROTECTION_AES_CCM_128_128   = 0x43,
	MQTTSN_PROTECTION_AES_CCM_128_192   = 0x44,
	MQTTSN_PROTECTION_AES_CCM_128_256   = 0x45,
	MQTTSN_PROTECTION_AES_GCM_128_128   = 0x46,
	MQTTSN_PROTECTION_AES_GCM_128_192   = 0x47,
	MQTTSN_PROTECTION_AES_GCM_128_256   = 0x48,
	MQTTSN_PROTECTION_CHACHA20_POLY1305 = 0x49,
	/* 0x4A-0xEF: reserved */
	/* 0xF0-0xFF: provider-defined non-authentication-only */
};

/* -------------------------------------------------------------------------
 * Fixed field sizes
 * ------------------------------------------------------------------------- */
#define MQTTSN_PROTECTION_SENDER_ID_LEN  8u   /**< Sender Identifier always 8 bytes */
#define MQTTSN_PROTECTION_RANDOM_LEN     4u   /**< Random field always 4 bytes      */

/* -------------------------------------------------------------------------
 * Protection Encapsulation data structure
 *
 * This structure represents the parsed contents of one Protection
 * Encapsulation packet (Section 3.17).  The library serializes and
 * deserializes the wire format only; cryptographic computation (key
 * derivation, encryption, tag generation and verification) is the
 * caller's responsibility.
 *
 * Variable-length fields (cryptoMaterial, monotonicCounter,
 * protectedPacket, authTag) use MQTTSN_data with zero-copy pointers
 * into the wire buffer on deserialization; the caller must keep the
 * buffer alive for as long as the data structure is in use.
 * ------------------------------------------------------------------------- */
typedef struct MQTTSNPacket_protectionData MQTTSNPacket_protectionData;
struct MQTTSNPacket_protectionData
{
	MQTTSNProtection_flags flags;     /**< Protection Flags (Section 3.17.2)    */
	uint8_t  scheme;                  /**< Protection Scheme (Section 3.17.3)   */
	uint8_t  senderid[MQTTSN_PROTECTION_SENDER_ID_LEN]; /**< Sender Identifier  */
	uint8_t  random[MQTTSN_PROTECTION_RANDOM_LEN];      /**< Random             */
	MQTTSN_data cryptoMaterial;       /**< Cryptographic Material (0/2/4/12 B)  */
	MQTTSN_data monotonicCounter;     /**< Monotonic Counter (0/2/4 bytes)      */
	MQTTSN_data protectedPacket;      /**< The inner MQTT-SN packet             */
	MQTTSN_data authTag;              /**< Authentication Tag                   */
};

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/*
 * Serialize a Protection Encapsulation packet.
 *
 * The caller is responsible for:
 *   - setting data->flags correctly for the variable fields present
 *   - providing data->cryptoMaterial and data->monotonicCounter with the
 *     correct lengths for the flags (see MQTTSN_PROTECTION_CRYPTO_* and
 *     MQTTSN_PROTECTION_COUNTER_* constants above)
 *   - providing the already-serialized inner packet in data->protectedPacket
 *   - providing the pre-computed authentication tag in data->authTag
 *
 * @param buf    the buffer into which the packet will be serialized
 * @param buflen the length in bytes of the supplied buffer
 * @param data   the protection data to serialize
 * @return the length of the serialized data.  <= 0 indicates error
 */
int32_t MQTTSNSerialize_protection(uint8_t* buf, int32_t buflen,
        const MQTTSNPacket_protectionData* data);

/*
 * Deserialize a Protection Encapsulation packet.
 *
 * All variable-length fields in the output structure are set as zero-copy
 * pointers into buf; buf must remain valid for the lifetime of the structure.
 *
 * The caller is responsible for:
 *   - verifying the authentication tag (data->authTag) against the preceding
 *     fields using the scheme indicated by data->scheme
 *   - decrypting data->protectedPacket if the scheme is not authentication-only
 *
 * @param data   the structure to fill with parsed packet fields
 * @param buf    the raw buffer data
 * @param buflen the length in bytes of the data in the supplied buffer
 * @return error code.  1 is success, 0 is failure
 */
int32_t MQTTSNDeserialize_protection(MQTTSNPacket_protectionData* data,
        uint8_t* buf, int32_t buflen);

/*
 * Returns the number of bytes of Cryptographic Material implied by a
 * Crypto Length field value (bits 3-2 of the Protection Flags).
 *
 * @param crypto_len_field the 2-bit Crypto Length value (0-3)
 * @return number of bytes: 0, 2, 4, or 12
 */
uint8_t MQTTSNProtection_cryptoMaterialLen(uint8_t crypto_len_field);

/*
 * Returns the number of bytes of Monotonic Counter implied by a
 * Counter Length field value (bits 1-0 of the Protection Flags).
 * Returns 0 for the reserved value 0x3.
 *
 * @param counter_len_field the 2-bit Counter Length value (0-3)
 * @return number of bytes: 0, 2, or 4 (0 for reserved value 0x3)
 */
uint8_t MQTTSNProtection_counterLen(uint8_t counter_len_field);

#endif /* MQTTSNPROTECTION_H_ */
