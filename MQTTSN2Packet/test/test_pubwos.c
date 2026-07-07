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
 * Round-trip tests for MQTTSNPubwos.c (PUBWOS packet, Section 3.6.1).
 *
 * Each test serializes a PUBWOS packet with a specific combination of
 * field values, then passes the resulting wire buffer to the deserializer
 * and checks that every field survives unchanged.
 *
 * Coverage:
 *   PUBWOS - topic type Predefined Topic Alias (minimum, typical, maximum)
 *          - topic type Topic Name (short, long)
 *          - retain flag (both values)
 *          - payload (empty, short, longer binary payload including
 *            embedded zero bytes)
 *          - wire length check
 *          - buffer too short rejection (serialize)
 *          - buflen shorter than the packet's own declared length is
 *            rejected rather than read past (deserialize)
 *          - malformed/short packets rejected by deserialize (hand-crafted
 *            wire bytes whose own length field is self-consistent with the
 *            supplied buflen, but which are missing required trailing fields)
 */

#include "MQTTSNPacket.h"
#include "MQTTSNPublish.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test infrastructure
 * ------------------------------------------------------------------------- */

#define BUF_SIZE 512

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void section(const char* title)
{
    printf("\n--- %s ---\n", title);
}


/* =========================================================================
 * PUBWOS round-trip helper
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_pubwos, deserialize with
 * MQTTSNDeserialize_pubwos, then check every field and the wire length.
 * expected_wire_len may be -1 to skip the wire length check.
 */
static int pubwos_roundtrip(const char* test_name, uint8_t in_retained,
        MQTTSN_topic* in_topic, uint8_t* in_payload, int32_t in_payloadlen,
        int32_t expected_wire_len)
{
    uint8_t      buf[BUF_SIZE];
    int32_t      slen;
    uint8_t      out_retained = 0xFF;
    MQTTSN_topic out_topic;
    uint8_t*     out_payload = NULL;
    int32_t      out_payloadlen = -1;
    int32_t      drc;
    int          ok = 1;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out_topic, 0, sizeof(out_topic));

    /* --- serialize --- */
    slen = MQTTSNSerialize_pubwos(buf, (int32_t)sizeof(buf),
                                   in_retained, in_topic, in_payload, in_payloadlen);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    /* --- deserialize --- */
    drc = MQTTSNDeserialize_pubwos(&out_retained, &out_topic,
                                    &out_payload, &out_payloadlen, buf, slen);
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

    RT_CHECK("retained",    out_retained   == in_retained);
    RT_CHECK("topic.type",  out_topic.type == in_topic->type);

    if (in_topic->type == MQTTSN_TOPIC_TYPE_NAME)
    {
        RT_CHECK("string.len",
                 out_topic.alt.string.len == in_topic->alt.string.len);
        RT_CHECK("string.data",
                 in_topic->alt.string.len == 0 ||
                 memcmp(out_topic.alt.string.data, in_topic->alt.string.data,
                        in_topic->alt.string.len) == 0);
    }
    else /* MQTTSN_TOPIC_TYPE_PREDEFINED */
    {
        RT_CHECK("alias", out_topic.alt.alias == in_topic->alt.alias);
    }

    RT_CHECK("payloadlen", out_payloadlen == in_payloadlen);
    RT_CHECK("payload",
             in_payloadlen == 0 ||
             memcmp(out_payload, in_payload, (size_t)in_payloadlen) == 0);

    if (expected_wire_len >= 0)
        RT_CHECK("wire length", slen == expected_wire_len);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}


/* =========================================================================
 * PUBWOS test cases
 * ========================================================================= */

/* topic type: PREDEFINED alias ---------------------------------------------- */

static void test_pubwos_predefined_min(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 1 } };
    unsigned char payload[] = "hello";

    /* len(1)+type(1)+flags(1)+alias(2)+payload(5) = 10 */
    pubwos_roundtrip("PUBWOS PREDEFINED alias 1 (minimum)",
                      0, &t, payload, (int32_t)(sizeof(payload) - 1), 10);
}

static void test_pubwos_predefined_typical(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 0x1234 } };
    unsigned char payload[] = "world";

    pubwos_roundtrip("PUBWOS PREDEFINED alias 0x1234",
                      1, &t, payload, (int32_t)(sizeof(payload) - 1), -1);
}

static void test_pubwos_predefined_max(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 0xFFFF } };
    unsigned char payload[] = { 0x01 };

    pubwos_roundtrip("PUBWOS PREDEFINED alias 0xFFFF (maximum)",
                      0, &t, payload, (int32_t)sizeof(payload), -1);
}

/* topic type: Topic Name ----------------------------------------------------- */

static void test_pubwos_name_short(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_NAME,
                       { .string = { false, 6, "sensor" } } };
    unsigned char payload[] = { 0x17 };

    /* len(1)+type(1)+flags(1)+namelen(2)+name(6)+payload(1) = 12 */
    pubwos_roundtrip("PUBWOS NAME short (\"sensor\")",
                      0, &t, payload, (int32_t)sizeof(payload), 12);
}

static void test_pubwos_name_long(void)
{
    static const char name[] = "building/floor3/room12/temperature";
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_NAME,
                       { .string = { false, (uint16_t)(sizeof(name) - 1), (char*)name } } };
    unsigned char payload[] = "21.5C";

    pubwos_roundtrip("PUBWOS NAME long path (35 chars)",
                      1, &t, payload, (int32_t)(sizeof(payload) - 1), -1);
}

/* payload variations ---------------------------------------------------------- */

static void test_pubwos_payload_empty(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 42 } };

    /* len(1)+type(1)+flags(1)+alias(2)+payload(0) = 5 */
    pubwos_roundtrip("PUBWOS zero-length payload",
                      0, &t, NULL, 0, 5);
}

static void test_pubwos_payload_embedded_zero(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 7 } };
    unsigned char payload[] = { 0x00, 0x01, 0x00, 0x02, 0x00 };

    pubwos_roundtrip("PUBWOS payload with embedded zero bytes",
                      1, &t, payload, (int32_t)sizeof(payload), -1);
}

static void test_pubwos_payload_long(void)
{
    static unsigned char payload[300];
    unsigned int i;
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 99 } };

    for (i = 0; i < sizeof(payload); ++i)
        payload[i] = (unsigned char)i;

    pubwos_roundtrip("PUBWOS long payload (300 bytes)",
                      0, &t, payload, (int32_t)sizeof(payload), -1);
}

/* error handling --------------------------------------------------------------- */

static void test_pubwos_serialize_buffer_too_short(void)
{
    uint8_t buf[3]; /* too small for any valid PUBWOS */
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 1 } };
    unsigned char payload[] = { 0x01 };

    printf("%-55s ", "PUBWOS buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_pubwos(buf, (int32_t)sizeof(buf),
                                0, &t, payload, (int32_t)sizeof(payload))
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
 * MQTTSNDeserialize_pubwos rejects a packet whose own declared Length field
 * is larger than the buflen actually supplied - i.e. a heap buffer that
 * physically holds fewer bytes than the packet claims to contain. Allocated
 * as an exactly-sized heap buffer so that, under AddressSanitizer, any
 * regression that reads past it is caught as a heap-buffer-overflow.
 */
static void test_pubwos_buflen_shorter_than_declared_length(void)
{
    uint8_t       full[64];
    int32_t       fulllen;
    uint8_t*      truncated;
    uint8_t       retained;
    MQTTSN_topic  topic;
    uint8_t*      payload;
    int32_t       payloadlen;
    int32_t       drc;
    MQTTSN_topic  t = { MQTTSN_TOPIC_TYPE_NAME,
                        { .string = { false, 6, "sensor" } } };
    unsigned char data[] = { 1, 2, 3, 4, 5 };

    printf("%-55s ", "PUBWOS buflen shorter than declared packet length");

    fulllen = MQTTSNSerialize_pubwos(full, (int32_t)sizeof(full),
                                      0, &t, data, (int32_t)sizeof(data));
    if (fulllen <= 4) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    /* physically only 4 bytes are available, but the Length byte inside
     * them still declares the original (longer) packet length */
    truncated = malloc(4);
    memcpy(truncated, full, 4);

    memset(&topic, 0, sizeof(topic));
    payload = NULL;
    payloadlen = -1;
    drc = MQTTSNDeserialize_pubwos(&retained, &topic, &payload, &payloadlen, truncated, 4);
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
 * Hand-crafted wire buffers that are self-consistent (the Length field
 * correctly matches the number of bytes actually supplied, exactly as a
 * genuinely short/truncated packet received off the wire would look) but
 * are missing required trailing fields. Each one must be rejected by
 * MQTTSNDeserialize_pubwos rather than reading past the declared length.
 */
static void test_pubwos_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[6];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short for header (type only)",
          { 2, MQTTSN_PUBWOS }, 2 },
        { "too short for header (missing topicAliasOrLen)",
          { 3, MQTTSN_PUBWOS, 0x00 }, 3 },
        { "too short for header (only 1 byte of topicAliasOrLen)",
          { 4, MQTTSN_PUBWOS, 0x00, 0x00 }, 4 },
        { "NAME type declares length 5, no name bytes present",
          { 5, MQTTSN_PUBWOS, 0x03 /* topic type NAME */, 0x00, 0x05 }, 5 },
    };
    size_t i;
    int    ok = 1;

    printf("%-55s ", "PUBWOS malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t       buf[sizeof(cases[0].bytes)];
        uint8_t       retained;
        MQTTSN_topic  topic;
        uint8_t*      payload;
        int32_t       payloadlen;
        int32_t       drc;

        memset(&topic, 0, sizeof(topic));
        payload = NULL;
        payloadlen = -1;
        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_pubwos(&retained, &topic, &payload, &payloadlen,
                                        buf, cases[i].len);
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
    printf("=== MQTT-SN 2.0 PUBWOS round-trip tests ===\n");

    section("PUBWOS — topic type PREDEFINED alias");
    test_pubwos_predefined_min();
    test_pubwos_predefined_typical();
    test_pubwos_predefined_max();

    section("PUBWOS — topic type NAME");
    test_pubwos_name_short();
    test_pubwos_name_long();

    section("PUBWOS — payload variations");
    test_pubwos_payload_empty();
    test_pubwos_payload_embedded_zero();
    test_pubwos_payload_long();

    section("PUBWOS — error handling");
    test_pubwos_serialize_buffer_too_short();
    test_pubwos_buflen_shorter_than_declared_length();
    test_pubwos_deserialize_malformed();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
