/**************************************************************************************
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
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#include "MQTTSNGWProtection.h"
#include <string.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

extern "C" {
#include "MQTTSNPacket.h"
#include "MQTTSNProtection.h"
}

namespace MQTTSNGW
{

/* HMAC-SHA256 length */
#define HMAC_SHA256_LEN 32

/* Build the AAD for a PROTECTION packet.
 *
 * AAD = length_field + type(0xFF) + flags + scheme + senderid(8) + random(4)
 *       + crypto_material + counter + inner_packet
 *
 * The total_wire_len must be the wire length of the complete PROTECTION packet
 * (including the auth tag of the expected length), so the length field encodes
 * the same bytes as appear on the wire.
 */
static uint16_t build_aad(const MQTTSNPacket_protectionData* pd,
                           const uint8_t* inner_buf, uint16_t inner_len,
                           uint16_t total_wire_len,
                           uint8_t* aad)
{
    uint16_t aad_len = 0;

    /* Length field */
    int8_t lenlen = MQTTSNPacket_encode(aad, total_wire_len);
    if (lenlen <= 0) return 0;
    aad_len += (uint16_t)lenlen;

    /* Type byte */
    aad[aad_len++] = (uint8_t)MQTTSN_PROTECTION;

    /* Flags */
    aad[aad_len++] = pd->flags.all;

    /* Scheme */
    aad[aad_len++] = pd->scheme;

    /* Sender ID (8 bytes) */
    memcpy(&aad[aad_len], pd->senderid, MQTTSN_PROTECTION_SENDER_ID_LEN);
    aad_len += MQTTSN_PROTECTION_SENDER_ID_LEN;

    /* Random (4 bytes) */
    memcpy(&aad[aad_len], pd->random, MQTTSN_PROTECTION_RANDOM_LEN);
    aad_len += MQTTSN_PROTECTION_RANDOM_LEN;

    /* Crypto material */
    if (pd->cryptoMaterial.len > 0 && pd->cryptoMaterial.data != NULL)
    {
        memcpy(&aad[aad_len], pd->cryptoMaterial.data, pd->cryptoMaterial.len);
        aad_len += pd->cryptoMaterial.len;
    }

    /* Monotonic counter */
    if (pd->monotonicCounter.len > 0 && pd->monotonicCounter.data != NULL)
    {
        memcpy(&aad[aad_len], pd->monotonicCounter.data, pd->monotonicCounter.len);
        aad_len += pd->monotonicCounter.len;
    }

    /* Inner packet */
    memcpy(&aad[aad_len], inner_buf, inner_len);
    aad_len += inner_len;

    return aad_len;
}

/* Compute HMAC-SHA256 over data using key. Returns the length of the tag, or 0. */
static unsigned int compute_hmac_sha256(const uint8_t* key, int keylen,
                                         const uint8_t* data, uint16_t datalen,
                                         uint8_t* out)
{
    unsigned int out_len = 0;
    HMAC(EVP_sha256(), key, keylen, data, datalen, out, &out_len);
    return out_len;
}

/* Resolve auth tag byte count from auth_tag_len_field and scheme.
 * HMAC-SHA256 always produces 32 bytes; NOMINAL and PROVIDER both mean 32. */
static uint16_t resolve_tag_len(uint8_t auth_tag_len_field)
{
    if (auth_tag_len_field == MQTTSN_PROTECTION_AUTHTAG_NOMINAL ||
        auth_tag_len_field == MQTTSN_PROTECTION_AUTHTAG_PROVIDER)
        return HMAC_SHA256_LEN;
    if (auth_tag_len_field >= 4u)
        return (uint16_t)(auth_tag_len_field * 2u); /* field * 16 bits */
    return HMAC_SHA256_LEN;
}

int GWProtection_validate(uint8_t* buf, int buflen,
                          const uint8_t* key, int keylen,
                          MQTTSNPacket_protectionData* pd_out)
{
    MQTTSNPacket_protectionData pd;
    memset(&pd, 0, sizeof(pd));

    if (MQTTSNDeserialize_protection(&pd, buf, buflen) != 1)
        return -1;

    /* Only HMAC-SHA256 is supported for now */
    if (pd.scheme != MQTTSN_PROTECTION_HMAC_SHA256)
        return -1;

    uint8_t auth_tag_len_field = (pd.flags.all >> 4) & 0x0Fu;
    uint16_t tag_len = resolve_tag_len(auth_tag_len_field);

    if (pd.authTag.len < tag_len)
        return -1;

    /* total wire len = the length value in the packet's own length field */
    int mylen = 0;
    int lenlen = MQTTSNPacket_decode(buf, buflen, &mylen);
    if (lenlen <= 0) return -1;
    uint16_t total_wire_len = (uint16_t)mylen;

    /* Build AAD */
    uint8_t aad[1536];
    uint16_t aad_len = build_aad(&pd,
                                  pd.protectedPacket.data,
                                  pd.protectedPacket.len,
                                  total_wire_len, aad);
    if (aad_len == 0) return -1;

    /* Compute expected HMAC */
    uint8_t expected_tag[HMAC_SHA256_LEN];
    unsigned int computed_len = compute_hmac_sha256(key, keylen,
                                                     aad, aad_len,
                                                     expected_tag);
    if (computed_len == 0) return -1;

    /* Compare tags (constant-time) */
    if (pd.authTag.len < tag_len) return 0;
    int match = (CRYPTO_memcmp(pd.authTag.data, expected_tag, tag_len) == 0);

    if (match && pd_out != NULL)
        *pd_out = pd;

    return match ? 1 : 0;
}

int32_t GWProtection_wrap(const uint8_t* sender_id,
                           const uint8_t* key, int keylen,
                           const uint8_t* inner_buf, uint16_t inner_len,
                           uint8_t* out_buf, int32_t out_buflen)
{
    MQTTSNPacket_protectionData pd;
    memset(&pd, 0, sizeof(pd));

    pd.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL; /* 32-byte tag */
    pd.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    pd.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    pd.scheme = MQTTSN_PROTECTION_HMAC_SHA256;

    memcpy(pd.senderid, sender_id, MQTTSN_PROTECTION_SENDER_ID_LEN);

    /* Generate 4 random bytes */
    if (RAND_bytes(pd.random, (int)MQTTSN_PROTECTION_RANDOM_LEN) != 1)
        return -1;

    pd.protectedPacket.data = (unsigned char*)inner_buf;
    pd.protectedPacket.len  = inner_len;

    /* Compute wire length: body = type(1)+flags(1)+scheme(1)+senderid(8)+random(4)+inner+tag(32) */
    uint16_t body_len = (uint16_t)(15u + inner_len + HMAC_SHA256_LEN);
    uint16_t total_wire_len = (uint16_t)(MQTTSNPacket_len(body_len));

    /* Build AAD */
    uint8_t aad[1536];
    uint16_t aad_len = build_aad(&pd, inner_buf, inner_len, total_wire_len, aad);
    if (aad_len == 0) return -1;

    /* Compute HMAC */
    uint8_t tag[HMAC_SHA256_LEN];
    unsigned int tag_len = compute_hmac_sha256(key, keylen, aad, aad_len, tag);
    if (tag_len == 0) return -1;

    pd.authTag.data = tag;
    pd.authTag.len  = (uint16_t)tag_len;

    return MQTTSNSerialize_protection(out_buf, out_buflen, &pd);
}

} /* namespace MQTTSNGW */
