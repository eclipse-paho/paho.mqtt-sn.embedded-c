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

/*
 * Round-trip tests for MQTTSNForwarder.c (Forwarder Encapsulation packet,
 * Section 3.19).
 *
 * Each test constructs a MQTTSNPacket_forwarderData with a specific
 * combination of fields, serializes it with MQTTSNSerialize_forwarder,
 * then deserializes the resulting wire buffer with
 * MQTTSNDeserialize_forwarder and verifies that every field survives
 * unchanged.
 *
 * Unlike every other envelope in this library, the Forwarder Encapsulation's
 * outer Length field covers only the header and Client Addressing
 * Information - not the encapsulated MQTT-SN packet, which is self-describing
 * and simply appended afterwards. This means the physical buffer is expected
 * to be longer than the outer declared length; tests specifically cover that
 * this is handled correctly, alongside the usual hardening checks.
 *
 * Coverage:
 *   Client Addressing Information — absent, short, longer
 *   Inner packet types            — PUBLISH, CONNECT, SUBSCRIBE (distinct lengths)
 *   Inner packet 3-byte length encoding
 *   Buffer too short              — serialize must return MQTTSNPACKET_BUFFER_TOO_SHORT
 *   buflen shorter than the outer packet's own declared length is rejected
 *     rather than read past (deserialize)
 *   malformed/short packets rejected by deserialize (hand-crafted wire bytes
 *     whose own length fields are self-consistent with the supplied buflen,
 *     but which are missing required trailing fields, including a
 *     truncated/oversized inner packet length)
 */

#include "MQTTSNPacket.h"
#include "MQTTSNForwarder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test infrastructure
 * ------------------------------------------------------------------------- */

#define BUF_SIZE 2048

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void section(const char* title)
{
    printf("\n--- %s ---\n", title);
}


/* -------------------------------------------------------------------------
 * Round-trip helper
 * ------------------------------------------------------------------------- */

/*
 * Serialize with MQTTSNSerialize_forwarder then deserialize with
 * MQTTSNDeserialize_forwarder and verify every field.
 */
static int forwarder_roundtrip(const char* test_name,
        const MQTTSNPacket_forwarderData* in)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    MQTTSNPacket_forwarderData out;
    int32_t  drc;
    int      ok = 1;

    printf("%-60s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out, 0, sizeof(out));

    /* --- serialize --- */
    slen = MQTTSNSerialize_forwarder(buf, (int32_t)sizeof(buf), in);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    /* --- deserialize --- */
    drc = MQTTSNDeserialize_forwarder(&out, buf, slen);
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

    RT_CHECK("clientAddress.len", out.clientAddress.len == in->clientAddress.len);
    if (in->clientAddress.len > 0)
        RT_CHECK("clientAddress.data",
                 memcmp(out.clientAddress.data, in->clientAddress.data,
                        in->clientAddress.len) == 0);

    RT_CHECK("mqttsnPacket.len", out.mqttsnPacket.len == in->mqttsnPacket.len);
    RT_CHECK("mqttsnPacket.data",
             memcmp(out.mqttsnPacket.data, in->mqttsnPacket.data,
                    in->mqttsnPacket.len) == 0);

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


/* =========================================================================
 * Client Addressing Information tests
 * ========================================================================= */

static void test_no_client_address(void)
{
    MQTTSNPacket_forwarderData in = {0};
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;
    forwarder_roundtrip("No Client Addressing Information", &in);
}

static void test_short_client_address(void)
{
    /* e.g. a wireless node identifier, as in MQTT-SN 1.2 */
    static const uint8_t addr[] = { 0x01, 0x02 };
    MQTTSNPacket_forwarderData in = {0};
    in.clientAddress.len  = (uint16_t)sizeof(addr);
    in.clientAddress.data = (unsigned char*)addr;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;
    forwarder_roundtrip("Short Client Addressing Information (2 bytes)", &in);
}

static void test_longer_client_address(void)
{
    /* e.g. an IPv6 address + port */
    static const uint8_t addr[18] = {
        0x20, 0x01, 0x0D, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01,
        0x1F, 0x90
    };
    MQTTSNPacket_forwarderData in = {0};
    in.clientAddress.len  = (uint16_t)sizeof(addr);
    in.clientAddress.data = (unsigned char*)addr;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_connect);
    in.mqttsnPacket.data = (unsigned char*)inner_connect;
    forwarder_roundtrip("Longer Client Addressing Information (18 bytes)", &in);
}


/* =========================================================================
 * Inner packet type tests
 * ========================================================================= */

static void test_inner_publish(void)
{
    static const uint8_t addr[] = { 0xAA, 0xBB };
    MQTTSNPacket_forwarderData in = {0};
    in.clientAddress.len  = (uint16_t)sizeof(addr);
    in.clientAddress.data = (unsigned char*)addr;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;
    forwarder_roundtrip("Inner packet: PUBLISH", &in);
}

static void test_inner_connect(void)
{
    static const uint8_t addr[] = { 0xCC };
    MQTTSNPacket_forwarderData in = {0};
    in.clientAddress.len  = (uint16_t)sizeof(addr);
    in.clientAddress.data = (unsigned char*)addr;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_connect);
    in.mqttsnPacket.data = (unsigned char*)inner_connect;
    forwarder_roundtrip("Inner packet: CONNECT", &in);
}

static void test_inner_subscribe(void)
{
    MQTTSNPacket_forwarderData in = {0};
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_subscribe);
    in.mqttsnPacket.data = (unsigned char*)inner_subscribe;
    forwarder_roundtrip("Inner packet: SUBSCRIBE, no Client Address", &in);
}

static void test_inner_large_3byte_length(void)
{
    /* Craft a fake large inner packet that uses a 3-byte length encoding.
     * Length field: 0x01 0x01 0x00 means total length = 256 bytes.
     */
    static uint8_t large_inner[256];
    MQTTSNPacket_forwarderData in = {0};

    memset(large_inner, 0, sizeof(large_inner));
    large_inner[0] = 0x01;     /* 3-byte length indicator */
    large_inner[1] = 0x01;     /* length high byte: 0x0100 = 256 */
    large_inner[2] = 0x00;     /* length low byte */
    large_inner[3] = 0x03;     /* MQTTSN_PUBLISH packet type */

    in.mqttsnPacket.len  = 256;
    in.mqttsnPacket.data = large_inner;
    forwarder_roundtrip("Inner packet: 256-byte (3-byte length field)", &in);
}


/* =========================================================================
 * error handling
 * ========================================================================= */

static void test_serialize_buffer_too_short(void)
{
    uint8_t buf[5]; /* far too small */
    MQTTSNPacket_forwarderData in = {0};
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;

    printf("%-60s ", "Buffer too short (serialize must return error)");
    ++tests_run;
    if (MQTTSNSerialize_forwarder(buf, (int32_t)sizeof(buf), &in)
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

/*
 * MQTTSNDeserialize_forwarder rejects a packet whose own declared outer
 * Length field is larger than the buflen actually supplied - i.e. a heap
 * buffer that physically holds fewer bytes than the outer header claims to
 * contain. Allocated as an exactly-sized heap buffer so that, under
 * AddressSanitizer, any regression that reads past it is caught as a
 * heap-buffer-overflow.
 */
static void test_buflen_shorter_than_declared_length(void)
{
    uint8_t  full[64];
    int32_t  fulllen;
    uint8_t* truncated;
    MQTTSNPacket_forwarderData data;
    int32_t  drc;
    static const uint8_t addr[] = { 0x01, 0x02, 0x03, 0x04 };
    MQTTSNPacket_forwarderData in = {0};

    in.clientAddress.len  = (uint16_t)sizeof(addr);
    in.clientAddress.data = (unsigned char*)addr;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;

    printf("%-60s ", "buflen shorter than declared outer packet length");

    fulllen = MQTTSNSerialize_forwarder(full, (int32_t)sizeof(full), &in);
    /* header = len(1)+type(1)+addr(4) = 6; must be > 3 for the truncation below */
    if (fulllen <= 3) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    /* physically only 3 bytes are available, but the Length byte inside
     * them still declares the original (longer) outer header length */
    truncated = malloc(3);
    memcpy(truncated, full, 3);

    memset(&data, 0, sizeof(data));
    drc = MQTTSNDeserialize_forwarder(&data, truncated, 3);
    free(truncated);

    ++tests_run;
    if (drc == MQTTSNPACKET_BUFFER_TOO_SHORT)
    {
        ++tests_passed;
        printf("ok\n");
    }
    else
    {
        ++tests_failed;
        printf("FAIL  (unexpectedly accepted or wrong error code: %d)\n", (int)drc);
    }
}

/*
 * Hand-crafted wire buffers that are self-consistent (the outer Length
 * field correctly matches the number of bytes actually supplied for the
 * header/addressing portion) but are missing required trailing fields, or
 * whose embedded inner-packet length is inconsistent with the physical
 * buffer. Each one must be rejected by MQTTSNDeserialize_forwarder rather
 * than reading past the declared/physical length.
 */
static void test_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[8];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short to decode the outer length field",
          { 1 }, 1 },
        { "outer length (3) exceeds supplied buflen (2)",
          { 3, MQTTSN_FORWARDER }, 2 },
        { "no inner packet after Client Addressing Information",
          { 2, MQTTSN_FORWARDER }, 2 },
        { "inner packet length byte present, body missing",
          { 2, MQTTSN_FORWARDER, 0x05 }, 3 },
        { "inner packet claims more bytes than physically supplied",
          { 2, MQTTSN_FORWARDER, 0x05, 0x03, 0x00 }, 5 },
    };
    size_t i;
    int    ok = 1;

    printf("%-60s ", "malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t                    buf[sizeof(cases[0].bytes)];
        MQTTSNPacket_forwarderData data;
        int32_t                    drc;

        memset(&data, 0, sizeof(data));
        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_forwarder(&data, buf, cases[i].len);
        if (drc == 1)
        {
            ok = 0;
            printf("\n  FAIL  case '%s' unexpectedly accepted", cases[i].label);
        }
    }

    ++tests_run;
    if (ok)
    {
        ++tests_passed;
        printf("ok\n");
    }
    else
    {
        ++tests_failed;
        printf("\n");
    }
}


/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    printf("=== MQTT-SN 2.0 Forwarder Encapsulation round-trip tests ===\n");

    section("Client Addressing Information");
    test_no_client_address();
    test_short_client_address();
    test_longer_client_address();

    section("Inner packet types");
    test_inner_publish();
    test_inner_connect();
    test_inner_subscribe();
    test_inner_large_3byte_length();

    section("Error handling");
    test_serialize_buffer_too_short();
    test_buflen_shorter_than_declared_length();
    test_deserialize_malformed();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
