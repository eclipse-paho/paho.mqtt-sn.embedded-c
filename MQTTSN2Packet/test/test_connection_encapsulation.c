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
 * Round-trip tests for MQTTSNConnectionEncapsulation.c (Connection
 * Encapsulation packet, Section 3.18; packet type 0xFE, enum
 * MQTTSN_CONNECTION - Table 2 mislabels this "Session Encapsulation").
 *
 * Each test constructs a MQTTSNPacket_connectionData with a specific
 * combination of fields, serializes it with MQTTSNSerialize_connection, then
 * deserializes the resulting wire buffer with MQTTSNDeserialize_connection
 * and verifies that every field survives unchanged.
 *
 * As with the Forwarder Encapsulation, the outer Length field covers only
 * the header and Client Identifier - not the encapsulated MQTT-SN packet,
 * which is self-describing and simply appended afterwards. This means the
 * physical buffer is expected to be longer than the outer declared length;
 * tests specifically cover that this is handled correctly, alongside the
 * usual hardening checks.
 *
 * Coverage:
 *   Client Identifier   — absent, short, longer
 *   Inner packet types  — PUBLISH, SUBSCRIBE, DISCONNECT (distinct lengths)
 *   Inner packet 3-byte length encoding
 *   Buffer too short    — serialize must return MQTTSNPACKET_BUFFER_TOO_SHORT
 *   buflen shorter than the outer packet's own declared length is rejected
 *     rather than read past (deserialize)
 *   malformed/short packets rejected by deserialize (hand-crafted wire bytes
 *     whose own length fields are self-consistent with the supplied buflen,
 *     but which are missing required trailing fields, including a
 *     truncated/oversized inner packet length)
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnectionEncapsulation.h"

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
 * Serialize with MQTTSNSerialize_connection then deserialize with
 * MQTTSNDeserialize_connection and verify every field.
 */
static int connection_roundtrip(const char* test_name,
        const MQTTSNPacket_connectionData* in)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    MQTTSNPacket_connectionData out;
    int32_t  drc;
    int      ok = 1;

    printf("%-60s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out, 0, sizeof(out));

    /* --- serialize --- */
    slen = MQTTSNSerialize_connection(buf, (int32_t)sizeof(buf), in);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    /* --- deserialize --- */
    drc = MQTTSNDeserialize_connection(&out, buf, slen);
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

    RT_CHECK("clientID.len", out.clientID.len == in->clientID.len);
    if (in->clientID.len > 0)
        RT_CHECK("clientID.data",
                 memcmp(out.clientID.data, in->clientID.data,
                        in->clientID.len) == 0);

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

/* Minimal inner SUBSCRIBE (alias form): length(1) + type(1) + flags(1) +
 *                                       packetid(2) + alias(2) = 7 bytes  */
static const uint8_t inner_subscribe[] = {
    0x07, 0x08, 0x01, 0x00, 0x01, 0x00, 0x42
};

/* Minimal inner DISCONNECT: length(1) + type(1) = 2 bytes (no reason code) */
static const uint8_t inner_disconnect[] = {
    0x02, 0x0E
};


/* =========================================================================
 * Client Identifier tests
 * ========================================================================= */

static void test_no_client_id(void)
{
    MQTTSNPacket_connectionData in = {0};
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;
    connection_roundtrip("No Client Identifier", &in);
}

static void test_short_client_id(void)
{
    static const char clientid[] = "c1";
    MQTTSNPacket_connectionData in = {0};
    in.clientID.len  = (uint16_t)(sizeof(clientid) - 1);
    in.clientID.data = (char*)clientid;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;
    connection_roundtrip("Short Client Identifier (\"c1\")", &in);
}

static void test_longer_client_id(void)
{
    static const char clientid[] = "sensor-node-living-room-0042";
    MQTTSNPacket_connectionData in = {0};
    in.clientID.len  = (uint16_t)(sizeof(clientid) - 1);
    in.clientID.data = (char*)clientid;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_subscribe);
    in.mqttsnPacket.data = (unsigned char*)inner_subscribe;
    connection_roundtrip("Longer Client Identifier (29 chars)", &in);
}


/* =========================================================================
 * Inner packet type tests
 * ========================================================================= */

static void test_inner_publish(void)
{
    static const char clientid[] = "abc";
    MQTTSNPacket_connectionData in = {0};
    in.clientID.len  = (uint16_t)(sizeof(clientid) - 1);
    in.clientID.data = (char*)clientid;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;
    connection_roundtrip("Inner packet: PUBLISH", &in);
}

static void test_inner_subscribe(void)
{
    MQTTSNPacket_connectionData in = {0};
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_subscribe);
    in.mqttsnPacket.data = (unsigned char*)inner_subscribe;
    connection_roundtrip("Inner packet: SUBSCRIBE, no Client Identifier", &in);
}

static void test_inner_disconnect(void)
{
    static const char clientid[] = "d";
    MQTTSNPacket_connectionData in = {0};
    in.clientID.len  = (uint16_t)(sizeof(clientid) - 1);
    in.clientID.data = (char*)clientid;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_disconnect);
    in.mqttsnPacket.data = (unsigned char*)inner_disconnect;
    connection_roundtrip("Inner packet: DISCONNECT", &in);
}

static void test_inner_large_3byte_length(void)
{
    /* Craft a fake large inner packet that uses a 3-byte length encoding.
     * Length field: 0x01 0x01 0x00 means total length = 256 bytes.
     */
    static uint8_t large_inner[256];
    MQTTSNPacket_connectionData in = {0};

    memset(large_inner, 0, sizeof(large_inner));
    large_inner[0] = 0x01;     /* 3-byte length indicator */
    large_inner[1] = 0x01;     /* length high byte: 0x0100 = 256 */
    large_inner[2] = 0x00;     /* length low byte */
    large_inner[3] = 0x03;     /* MQTTSN_PUBLISH packet type */

    in.mqttsnPacket.len  = 256;
    in.mqttsnPacket.data = large_inner;
    connection_roundtrip("Inner packet: 256-byte (3-byte length field)", &in);
}


/* =========================================================================
 * error handling
 * ========================================================================= */

static void test_serialize_buffer_too_short(void)
{
    uint8_t buf[5]; /* far too small */
    MQTTSNPacket_connectionData in = {0};
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;

    printf("%-60s ", "Buffer too short (serialize must return error)");
    ++tests_run;
    if (MQTTSNSerialize_connection(buf, (int32_t)sizeof(buf), &in)
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
 * MQTTSNDeserialize_connection rejects a packet whose own declared outer
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
    MQTTSNPacket_connectionData data;
    int32_t  drc;
    static const char clientid[] = "abcd";
    MQTTSNPacket_connectionData in = {0};

    in.clientID.len  = (uint16_t)(sizeof(clientid) - 1);
    in.clientID.data = (char*)clientid;
    in.mqttsnPacket.len  = (uint16_t)sizeof(inner_publish);
    in.mqttsnPacket.data = (unsigned char*)inner_publish;

    printf("%-60s ", "buflen shorter than declared outer packet length");

    fulllen = MQTTSNSerialize_connection(full, (int32_t)sizeof(full), &in);
    /* header = len(1)+type(1)+clientid(4) = 6; must be > 3 for the truncation below */
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
    drc = MQTTSNDeserialize_connection(&data, truncated, 3);
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
 * header/Client Identifier portion) but are missing required trailing
 * fields, or whose embedded inner-packet length is inconsistent with the
 * physical buffer. Each one must be rejected by MQTTSNDeserialize_connection
 * rather than reading past the declared/physical length.
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
          { 3, MQTTSN_CONNECTION }, 2 },
        { "no inner packet after Client Identifier",
          { 2, MQTTSN_CONNECTION }, 2 },
        { "inner packet length byte present, body missing",
          { 2, MQTTSN_CONNECTION, 0x05 }, 3 },
        { "inner packet claims more bytes than physically supplied",
          { 2, MQTTSN_CONNECTION, 0x05, 0x03, 0x00 }, 5 },
    };
    size_t i;
    int    ok = 1;

    printf("%-60s ", "malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t                     buf[sizeof(cases[0].bytes)];
        MQTTSNPacket_connectionData data;
        int32_t                     drc;

        memset(&data, 0, sizeof(data));
        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_connection(&data, buf, cases[i].len);
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
    printf("=== MQTT-SN 2.0 Connection Encapsulation round-trip tests ===\n");

    section("Client Identifier");
    test_no_client_id();
    test_short_client_id();
    test_longer_client_id();

    section("Inner packet types");
    test_inner_publish();
    test_inner_subscribe();
    test_inner_disconnect();
    test_inner_large_3byte_length();

    section("Error handling");
    test_serialize_buffer_too_short();
    test_buflen_shorter_than_declared_length();
    test_deserialize_malformed();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
