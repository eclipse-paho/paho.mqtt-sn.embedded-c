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
 * Round-trip tests for MQTTSNProtection.c.
 *
 * Each test constructs a MQTTSNPacket_protectionData with a specific
 * combination of fields, serializes it with MQTTSNSerialize_protection,
 * then deserializes the resulting wire buffer with MQTTSNDeserialize_protection
 * and verifies that every field survives unchanged.
 *
 * Coverage:
 *   Crypto Material Length  — all four values (none / 2 / 4 / 12 bytes)
 *   Counter Length          — all three valid values (none / 2 / 4 bytes)
 *   Authentication Tag      — provider-defined (0x0), nominal (0x1),
 *                             truncated auth-only (0x4-0xF)
 *   Protection Scheme       — representative auth-only and AEAD schemes
 *   Inner packet types      — PUBLISH, CONNECT, SUBSCRIBE (distinct lengths)
 *   Large inner packet      — inner packet using 3-byte length encoding
 *   All optional fields     — maximum combination
 *   No optional fields      — minimum combination
 *   Buffer too short        — serialize must return MQTTSNPACKET_BUFFER_TOO_SHORT
 *   Reserved flag values    — Counter Length 0x3 and Auth Tag Length 0x2/0x3
 *                             must be rejected by the deserializer
 */

#include "MQTTSNPacket.h"
#include "MQTTSNProtection.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test infrastructure
 * ------------------------------------------------------------------------- */

#define BUF_SIZE 2048

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(label, expr)                                          \
    do {                                                            \
        ++tests_run;                                                \
        if (expr) {                                                 \
            ++tests_passed;                                         \
        } else {                                                    \
            ++tests_failed;                                         \
            printf("  FAIL  %s:%d  %s\n", __FILE__, __LINE__,      \
                   (label));                                        \
        }                                                           \
    } while (0)

static void section(const char* title)
{
    printf("\n--- %s ---\n", title);
}


/* -------------------------------------------------------------------------
 * Round-trip helper
 * ------------------------------------------------------------------------- */

/*
 * Serialize with MQTTSNSerialize_protection then deserialize with
 * MQTTSNDeserialize_protection and verify every field.
 */
static int protection_roundtrip(const char* test_name,
        const MQTTSNPacket_protectionData* in)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    MQTTSNPacket_protectionData out;
    int32_t  drc;
    int      ok = 1;

    printf("%-60s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out, 0, sizeof(out));

    /* --- serialize --- */
    slen = MQTTSNSerialize_protection(buf, (int32_t)sizeof(buf), in);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    /* --- deserialize --- */
    drc = MQTTSNDeserialize_protection(&out, buf, slen);
    ++tests_run;
    if (drc != 1) {
        printf("FAIL  (deserialize returned %d)\n", (int)drc);
        ++tests_failed;
        return 0;
    }
    ++tests_passed;

#define RT_CHECK(label, expr) \
    do { ++tests_run; if (expr) { ++tests_passed; } \
         else { ++tests_failed; ok = 0; \
                printf("\n  FAIL  field: %s", (label)); } } while (0)

    RT_CHECK("flags.all",    out.flags.all  == in->flags.all);
    RT_CHECK("scheme",       out.scheme     == in->scheme);

    RT_CHECK("senderid",
             memcmp(out.senderid, in->senderid,
                    MQTTSN_PROTECTION_SENDER_ID_LEN) == 0);
    RT_CHECK("random",
             memcmp(out.random,   in->random,
                    MQTTSN_PROTECTION_RANDOM_LEN) == 0);

    RT_CHECK("cryptoMaterial.len",  out.cryptoMaterial.len  == in->cryptoMaterial.len);
    if (in->cryptoMaterial.len > 0)
        RT_CHECK("cryptoMaterial.data",
                 memcmp(out.cryptoMaterial.data,
                        in->cryptoMaterial.data,
                        in->cryptoMaterial.len) == 0);

    RT_CHECK("monotonicCounter.len", out.monotonicCounter.len == in->monotonicCounter.len);
    if (in->monotonicCounter.len > 0)
        RT_CHECK("monotonicCounter.data",
                 memcmp(out.monotonicCounter.data,
                        in->monotonicCounter.data,
                        in->monotonicCounter.len) == 0);

    RT_CHECK("protectedPacket.len",  out.protectedPacket.len  == in->protectedPacket.len);
    RT_CHECK("protectedPacket.data",
             memcmp(out.protectedPacket.data,
                    in->protectedPacket.data,
                    in->protectedPacket.len) == 0);

    RT_CHECK("authTag.len",  out.authTag.len  == in->authTag.len);
    if (in->authTag.len > 0)
        RT_CHECK("authTag.data",
                 memcmp(out.authTag.data,
                        in->authTag.data,
                        in->authTag.len) == 0);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}


/* -------------------------------------------------------------------------
 * Shared test data
 * ------------------------------------------------------------------------- */

/* Minimal inner PUBLISH: length(1) + type(1) + flags(1) +
 *                        topic_alias(2) + payload(4) = 9 bytes            */
static const uint8_t inner_publish[] = {
    0x09, 0x03, 0x00, 0x00, 0x01, 'T', 'e', 's', 't'
};

/* Minimal inner CONNECT: length(1) + type(1) + flags(1) + protocol(1) +
 *                        keepalive(2) + maxpktsize(2) + clientid(2) = 10  */
static const uint8_t inner_connect[] = {
    0x0A, 0x01, 0x01, 0x02, 0x00, 0x3C, 0x00, 0x64, 'c', 'l'
};

/* Minimal inner SUBSCRIBE (alias form): length(1) + type(1) + flags(1) +
 *                                       packetid(2) + alias(2) = 7 bytes  */
static const uint8_t inner_subscribe[] = {
    0x07, 0x08, 0x01, 0x00, 0x01, 0x00, 0x42
};

static const uint8_t sender_id[8] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};
static const uint8_t random_val[4] = { 0xDE, 0xAD, 0xBE, 0xEF };


/* =========================================================================
 * Crypto Material Length tests
 * ========================================================================= */

static void test_crypto_none(void)
{
    static const uint8_t tag[16] = { 0xAA };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_HMAC_SHA256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Crypto Material NONE (0x0)", &in);
}

static void test_crypto_16(void)
{
    static const uint8_t tag[16]    = { 0xBB };
    static const uint8_t crypto[2]  = { 0x01, 0x02 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_16;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_CMAC_128;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.cryptoMaterial.len  = 2;
    in.cryptoMaterial.data = (unsigned char*)crypto;
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Crypto Material 16-bit / 2 bytes (0x1)", &in);
}

static void test_crypto_32(void)
{
    static const uint8_t tag[16]    = { 0xCC };
    static const uint8_t crypto[4]  = { 0xA1, 0xB2, 0xC3, 0xD4 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_32;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_AES_GCM_128_128;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.cryptoMaterial.len  = 4;
    in.cryptoMaterial.data = (unsigned char*)crypto;
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Crypto Material 32-bit / 4 bytes (0x2)", &in);
}

static void test_crypto_96(void)
{
    static const uint8_t tag[16]   = { 0xDD };
    static const uint8_t crypto[12] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C
    };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_96;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_CHACHA20_POLY1305;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.cryptoMaterial.len  = 12;
    in.cryptoMaterial.data = (unsigned char*)crypto;
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Crypto Material 96-bit / 12 bytes (0x3)", &in);
}


/* =========================================================================
 * Monotonic Counter Length tests
 * ========================================================================= */

static void test_counter_none(void)
{
    static const uint8_t tag[8] = { 0x11 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_HMAC_SHA3_256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_connect);
    in.protectedPacket.data = (unsigned char*)inner_connect;
    in.authTag.len  = 8;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Counter Length NONE (0x0)", &in);
}

static void test_counter_16(void)
{
    static const uint8_t tag[16]     = { 0x22 };
    static const uint8_t counter[2]  = { 0x01, 0x00 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_16;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_CMAC_256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.monotonicCounter.len  = 2;
    in.monotonicCounter.data = (unsigned char*)counter;
    in.protectedPacket.len  = sizeof(inner_connect);
    in.protectedPacket.data = (unsigned char*)inner_connect;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Counter Length 16-bit / 2 bytes (0x1)", &in);
}

static void test_counter_32(void)
{
    static const uint8_t tag[16]     = { 0x33 };
    static const uint8_t counter[4]  = { 0x00, 0x01, 0x23, 0x45 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_32;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_AES_CCM_128_256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.monotonicCounter.len  = 4;
    in.monotonicCounter.data = (unsigned char*)counter;
    in.protectedPacket.len  = sizeof(inner_connect);
    in.protectedPacket.data = (unsigned char*)inner_connect;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Counter Length 32-bit / 4 bytes (0x2)", &in);
}


/* =========================================================================
 * Authentication Tag Length tests
 * ========================================================================= */

static void test_authtag_provider_defined(void)
{
    /* 0x0 = provider-defined; deserializer infers length from remaining bytes */
    static const uint8_t tag[5] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_PROVIDER;
    in.scheme = MQTTSN_PROTECTION_HMAC_SHA256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_subscribe);
    in.protectedPacket.data = (unsigned char*)inner_subscribe;
    in.authTag.len  = 5;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Auth Tag Length provider-defined (0x0, 5 bytes)", &in);
}

static void test_authtag_nominal(void)
{
    /* 0x1 = nominal tag size for the scheme */
    static const uint8_t tag[16] = { 0xFF };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_AES_GCM_128_256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_subscribe);
    in.protectedPacket.data = (unsigned char*)inner_subscribe;
    in.authTag.len  = 16;  /* 128 bits = nominal for AES-GCM */
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Auth Tag Length nominal (0x1, AES-GCM 128-bit tag)", &in);
}

static void test_authtag_truncated_64(void)
{
    /* 0x4 = 4 * 16 = 64 bits = 8 bytes; auth-only schemes only */
    static const uint8_t tag[8] = { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = 0x4u; /* 0x4 * 16 = 64 bits = 8 bytes */
    in.scheme = MQTTSN_PROTECTION_CMAC_128;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_subscribe);
    in.protectedPacket.data = (unsigned char*)inner_subscribe;
    in.authTag.len  = 8;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Auth Tag Length 0x4 (64-bit truncation, 8 bytes)", &in);
}

static void test_authtag_truncated_128(void)
{
    /* 0x8 = 8 * 16 = 128 bits = 16 bytes */
    static const uint8_t tag[16] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                                     0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = 0x8u; /* 0x8 * 16 = 128 bits = 16 bytes */
    in.scheme = MQTTSN_PROTECTION_HMAC_SHA256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_subscribe);
    in.protectedPacket.data = (unsigned char*)inner_subscribe;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Auth Tag Length 0x8 (128-bit truncation, 16 bytes)", &in);
}


/* =========================================================================
 * Inner packet type and size tests
 * ========================================================================= */

static void test_inner_publish(void)
{
    static const uint8_t tag[16] = { 0x10 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_AES_CCM_64_128;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 8;   /* AES-CCM-64 nominal tag is 64 bits = 8 bytes */
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Inner packet: PUBLISH", &in);
}

static void test_inner_connect(void)
{
    static const uint8_t tag[16] = { 0x20 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_AES_GCM_128_192;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_connect);
    in.protectedPacket.data = (unsigned char*)inner_connect;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Inner packet: CONNECT", &in);
}

static void test_inner_subscribe(void)
{
    static const uint8_t tag[16] = { 0x30 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_CHACHA20_POLY1305;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_subscribe);
    in.protectedPacket.data = (unsigned char*)inner_subscribe;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Inner packet: SUBSCRIBE", &in);
}

static void test_inner_large_3byte_length(void)
{
    /* Craft a fake large inner packet that uses a 3-byte length encoding.
     * Length field: 0x01 0x01 0x00 means total length = 256 bytes.
     * We fill the remainder with 0x00 to simulate payload.
     */
    static uint8_t large_inner[256];
    static const uint8_t tag[16] = { 0x40 };
    MQTTSNPacket_protectionData in = {0};

    memset(large_inner, 0, sizeof(large_inner));
    large_inner[0] = 0x01;     /* 3-byte length indicator */
    large_inner[1] = 0x01;     /* length high byte: 0x0100 = 256 */
    large_inner[2] = 0x00;     /* length low byte */
    large_inner[3] = 0x03;     /* MQTTSN_PUBLISH packet type */

    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_AES_GCM_128_128;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = 256;
    in.protectedPacket.data = large_inner;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Inner packet: 256-byte (3-byte length field)", &in);
}


/* =========================================================================
 * Combined field tests
 * ========================================================================= */

static void test_no_optional_fields(void)
{
    /* Minimum protection packet: no crypto material, no counter */
    static const uint8_t tag[16] = { 0x01 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_NONE;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_NONE;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_HMAC_SHA256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Minimum: no crypto material, no counter", &in);
}

static void test_all_optional_fields(void)
{
    /* Maximum: 12-byte crypto material + 4-byte counter */
    static const uint8_t tag[16]    = { 0xFF };
    static const uint8_t crypto[12] = { 0x01,0x02,0x03,0x04,0x05,0x06,
                                         0x07,0x08,0x09,0x0A,0x0B,0x0C };
    static const uint8_t counter[4] = { 0x00, 0x00, 0x10, 0x00 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.cryptoLen  = MQTTSN_PROTECTION_CRYPTO_96;
    in.flags.bits.counterLen = MQTTSN_PROTECTION_COUNTER_32;
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_AES_GCM_128_256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.cryptoMaterial.len  = 12;
    in.cryptoMaterial.data = (unsigned char*)crypto;
    in.monotonicCounter.len  = 4;
    in.monotonicCounter.data = (unsigned char*)counter;
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("Maximum: 12-byte crypto + 4-byte counter", &in);
}

static void test_distinct_sender_ids(void)
{
    /* Verify all 8 sender ID bytes are individually preserved */
    static const uint8_t tag[4] = { 0xAB, 0xCD, 0xEF, 0x01 };
    static const uint8_t sid[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_PROVIDER;
    in.scheme = MQTTSN_PROTECTION_CMAC_192;
    memcpy(in.senderid, sid, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 4;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("All 8 sender ID bytes individually set", &in);
}

static void test_distinct_random_bytes(void)
{
    /* Verify all 4 random bytes are individually preserved */
    static const uint8_t tag[4]  = { 0x01, 0x02, 0x03, 0x04 };
    static const uint8_t rnd[4]  = { 0xAA, 0xBB, 0xCC, 0xDD };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_PROVIDER;
    in.scheme = MQTTSN_PROTECTION_CMAC_256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   rnd, 4);
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 4;
    in.authTag.data = (unsigned char*)tag;
    protection_roundtrip("All 4 random bytes individually set", &in);
}


/* =========================================================================
 * Error handling
 * ========================================================================= */

static void test_buffer_too_short(void)
{
    uint8_t buf[10]; /* far too small */
    static const uint8_t tag[16] = { 0xEE };
    MQTTSNPacket_protectionData in = {0};
    in.flags.bits.authTagLen = MQTTSN_PROTECTION_AUTHTAG_NOMINAL;
    in.scheme = MQTTSN_PROTECTION_HMAC_SHA256;
    memcpy(in.senderid, sender_id, 8);
    memcpy(in.random,   random_val, 4);
    in.protectedPacket.len  = sizeof(inner_publish);
    in.protectedPacket.data = (unsigned char*)inner_publish;
    in.authTag.len  = 16;
    in.authTag.data = (unsigned char*)tag;

    printf("%-60s ", "Buffer too short (serialize must return error)");
    ++tests_run;
    if (MQTTSNSerialize_protection(buf, (int32_t)sizeof(buf), &in)
        == MQTTSNPACKET_BUFFER_TOO_SHORT)
    {
        ++tests_passed;
        printf("ok\n");
    }
    else
    {
        ++tests_failed;
        printf("FAIL\n");
    }
}

static void test_reserved_counter_len(void)
{
    /* Counter Length 0x3 is reserved [MQTT-SN-3.17.2.1-1] and must be
     * rejected by the deserializer.  Craft a raw wire buffer with that value.
     *
     * Wire: length(1)=18 + type(1)=0xFF + flags(1)=0x13 (counter=3,crypto=0,
     *       authtag=1) + scheme(1) + senderid(8) + random(4) + authtag(2)
     */
    static uint8_t bad_buf[18];
    int pos = 0;
    bad_buf[pos++] = 18;                        /* length */
    bad_buf[pos++] = MQTTSN_PROTECTION;         /* type */
    bad_buf[pos++] = 0x13u;                     /* flags: counterLen=0x3 (reserved) */
    bad_buf[pos++] = 0x00;                      /* scheme */
    memcpy(&bad_buf[pos], sender_id, 8); pos += 8; /* senderid */
    memcpy(&bad_buf[pos], random_val,  4); pos += 4; /* random */
    bad_buf[pos++] = 0x02; bad_buf[pos++] = 0x03; /* 2-byte auth tag */

    MQTTSNPacket_protectionData out;
    memset(&out, 0, sizeof(out));

    printf("%-60s ", "Reserved Counter Length 0x3 rejected by deserializer");
    CHECK("deserialize rejects reserved counter length",
          MQTTSNDeserialize_protection(&out, bad_buf, sizeof(bad_buf)) != 1);
    printf("\n");
}

static void test_reserved_auth_tag_len_2(void)
{
    /* Authentication Tag Length 0x2 is reserved [MQTT-SN-3.17.2.3-3]. */
    static uint8_t bad_buf[20];
    int pos = 0;
    bad_buf[pos++] = 20;
    bad_buf[pos++] = MQTTSN_PROTECTION;
    bad_buf[pos++] = 0x20u; /* flags: authTagLen=0x2 (reserved) */
    bad_buf[pos++] = 0x00;
    memcpy(&bad_buf[pos], sender_id, 8); pos += 8;
    memcpy(&bad_buf[pos], random_val,  4); pos += 4;
    /* minimal inner packet placeholder */
    bad_buf[pos++] = 0x03; bad_buf[pos++] = 0x03;  /* 2-byte "packet" */
    bad_buf[pos++] = 0xFF; bad_buf[pos++] = 0xFF;  /* auth tag */

    MQTTSNPacket_protectionData out;
    memset(&out, 0, sizeof(out));

    printf("%-60s ", "Reserved Auth Tag Length 0x2 rejected by deserializer");
    CHECK("deserialize rejects reserved auth tag length 0x2",
          MQTTSNDeserialize_protection(&out, bad_buf, sizeof(bad_buf)) != 1);
    printf("\n");
}

static void test_reserved_auth_tag_len_3(void)
{
    /* Authentication Tag Length 0x3 is reserved [MQTT-SN-3.17.2.3-3]. */
    static uint8_t bad_buf[20];
    int pos = 0;
    bad_buf[pos++] = 20;
    bad_buf[pos++] = MQTTSN_PROTECTION;
    bad_buf[pos++] = 0x30u; /* flags: authTagLen=0x3 (reserved) */
    bad_buf[pos++] = 0x00;
    memcpy(&bad_buf[pos], sender_id, 8); pos += 8;
    memcpy(&bad_buf[pos], random_val,  4); pos += 4;
    bad_buf[pos++] = 0x03; bad_buf[pos++] = 0x03;
    bad_buf[pos++] = 0xFF; bad_buf[pos++] = 0xFF;

    MQTTSNPacket_protectionData out;
    memset(&out, 0, sizeof(out));

    printf("%-60s ", "Reserved Auth Tag Length 0x3 rejected by deserializer");
    CHECK("deserialize rejects reserved auth tag length 0x3",
          MQTTSNDeserialize_protection(&out, bad_buf, sizeof(bad_buf)) != 1);
    printf("\n");
}


/* =========================================================================
 * Helper function tests
 * ========================================================================= */

static void test_helper_crypto_len(void)
{
    section("Helper: MQTTSNProtection_cryptoMaterialLen");
    CHECK("0x0 → 0 bytes",  MQTTSNProtection_cryptoMaterialLen(0x0) ==  0);
    CHECK("0x1 → 2 bytes",  MQTTSNProtection_cryptoMaterialLen(0x1) ==  2);
    CHECK("0x2 → 4 bytes",  MQTTSNProtection_cryptoMaterialLen(0x2) ==  4);
    CHECK("0x3 → 12 bytes", MQTTSNProtection_cryptoMaterialLen(0x3) == 12);
}

static void test_helper_counter_len(void)
{
    section("Helper: MQTTSNProtection_counterLen");
    CHECK("0x0 → 0 bytes",        MQTTSNProtection_counterLen(0x0) == 0);
    CHECK("0x1 → 2 bytes",        MQTTSNProtection_counterLen(0x1) == 2);
    CHECK("0x2 → 4 bytes",        MQTTSNProtection_counterLen(0x2) == 4);
    CHECK("0x3 → 0 bytes (rsvd)", MQTTSNProtection_counterLen(0x3) == 0);
}


/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    printf("=== MQTT-SN 2.0 Protection Encapsulation round-trip tests ===\n");

    section("Cryptographic Material Length");
    test_crypto_none();
    test_crypto_16();
    test_crypto_32();
    test_crypto_96();

    section("Monotonic Counter Length");
    test_counter_none();
    test_counter_16();
    test_counter_32();

    section("Authentication Tag Length");
    test_authtag_provider_defined();
    test_authtag_nominal();
    test_authtag_truncated_64();
    test_authtag_truncated_128();

    section("Inner packet types");
    test_inner_publish();
    test_inner_connect();
    test_inner_subscribe();
    test_inner_large_3byte_length();

    section("Combined optional fields");
    test_no_optional_fields();
    test_all_optional_fields();
    test_distinct_sender_ids();
    test_distinct_random_bytes();

    section("Error handling");
    test_buffer_too_short();
    test_reserved_counter_len();
    test_reserved_auth_tag_len_2();
    test_reserved_auth_tag_len_3();

    test_helper_crypto_len();
    test_helper_counter_len();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
