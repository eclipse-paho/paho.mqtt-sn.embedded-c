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
 * Round-trip tests for MQTTSNSleepClient.c / MQTTSNSleepServer.c (SLEEPREQ,
 * SLEEPRESP and WAKEUP packets, Sections 3.14-3.16).
 *
 * Each test serializes a packet with a specific combination of field values,
 * then passes the resulting wire buffer to the deserializer and checks that
 * every field survives unchanged.
 *
 * Coverage:
 *   SLEEPREQ  - Retain Topic Aliases flag (both values)
 *             - packet identifier / Sleep Duration boundaries (min, typical, max)
 *   SLEEPRESP - Sleep Duration Flag / optional Sleep Duration field
 *             - optional Reason Code (present/absent)
 *   WAKEUP    - header-only packet (no fields)
 *   error handling - buffer too short (serialize)
 *                   - buflen shorter than the packet's own declared length is
 *                     rejected rather than read past (deserialize)
 *                   - malformed/short packets rejected by deserialize
 *                     (hand-crafted wire bytes whose own length field is
 *                     self-consistent with the supplied buflen, but which are
 *                     missing required trailing fields)
 */

#include "MQTTSNPacket.h"
#include "MQTTSNSleep.h"

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
 * SLEEPREQ round-trip helper
 * ========================================================================= */

static int sleepreq_roundtrip(const char* test_name, uint8_t in_retain,
        uint16_t in_packetid, uint32_t in_duration, int32_t expected_wire_len)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    uint8_t  out_retain = 0xFF;
    uint16_t out_packetid = 0;
    uint32_t out_duration = 0;
    int32_t  drc;
    int      ok = 1;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));

    slen = MQTTSNSerialize_sleepreq(buf, (int32_t)sizeof(buf),
                                     in_retain, in_packetid, in_duration);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_sleepreq(&out_retain, &out_packetid, &out_duration,
                                      buf, slen);
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

    RT_CHECK("retainTopicAliases", out_retain    == in_retain);
    RT_CHECK("packetid",           out_packetid  == in_packetid);
    RT_CHECK("duration",           out_duration  == in_duration);

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
 * SLEEPRESP round-trip helper
 * ========================================================================= */

static int sleepresp_roundtrip(const char* test_name, uint8_t in_durationPresent,
        uint16_t in_packetid, uint32_t in_duration, uint8_t in_returncode,
        int32_t expected_wire_len)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    uint8_t  out_durationPresent = 0xFF;
    uint16_t out_packetid = 0;
    uint32_t out_duration = 0xFFFFFFFFu;
    uint8_t  out_returncode = 0xFF;
    int32_t  drc;
    int      ok = 1;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));

    slen = MQTTSNSerialize_sleepresp(buf, (int32_t)sizeof(buf),
                                      in_durationPresent, in_packetid,
                                      in_duration, in_returncode);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_sleepresp(&out_durationPresent, &out_packetid,
                                       &out_duration, &out_returncode, buf, slen);
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

    RT_CHECK("durationPresent", out_durationPresent == in_durationPresent);
    RT_CHECK("packetid",        out_packetid         == in_packetid);
    RT_CHECK("returncode",      out_returncode        == in_returncode);

    if (in_durationPresent)
        RT_CHECK("duration", out_duration == in_duration);
    else
        RT_CHECK("duration absent -> 0", out_duration == 0);

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
 * WAKEUP round-trip helper
 * ========================================================================= */

static int wakeup_roundtrip(const char* test_name)
{
    uint8_t buf[BUF_SIZE];
    int32_t slen;
    int32_t drc;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));

    slen = MQTTSNSerialize_wakeup(buf, (int32_t)sizeof(buf));
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_wakeup(buf, slen);
    ++tests_run;
    if (drc != 1) {
        printf("FAIL  (deserialize returned %d)\n", (int)drc);
        ++tests_failed;
        return 0;
    }
    ++tests_passed;

    /* len(1)+type(1) = 2, no other fields (Section 3.14) */
    ++tests_run;
    if (slen == 2) {
        ++tests_passed;
        printf("ok\n");
        return 1;
    }

    ++tests_failed;
    printf("FAIL  (wire length %d, expected 2)\n", (int)slen);
    return 0;
}


/* =========================================================================
 * SLEEPREQ test cases
 * ========================================================================= */

static void test_sleepreq_retain_off(void)
{
    /* len(1)+type(1)+flags(1)+packetid(2)+duration(4) = 9 */
    sleepreq_roundtrip("SLEEPREQ Retain Topic Aliases off", 0, 1, 60, 9);
}

static void test_sleepreq_retain_on(void)
{
    sleepreq_roundtrip("SLEEPREQ Retain Topic Aliases on", 1, 0x1234, 3600, 9);
}

static void test_sleepreq_duration_min(void)
{
    /* Section 3.15.4: Sleep Duration MUST be greater than 0 */
    sleepreq_roundtrip("SLEEPREQ Sleep Duration minimum (1)", 0, 1, 1, 9);
}

static void test_sleepreq_duration_max(void)
{
    sleepreq_roundtrip("SLEEPREQ Sleep Duration maximum (0xFFFFFFFF)",
                        1, 0xFFFF, 0xFFFFFFFFu, 9);
}

static void test_sleepreq_packetid_max(void)
{
    sleepreq_roundtrip("SLEEPREQ packet identifier maximum (0xFFFF)",
                        0, 0xFFFF, 120, 9);
}


/* =========================================================================
 * SLEEPRESP test cases
 * ========================================================================= */

static void test_sleepresp_no_duration_success(void)
{
    /* len(1)+type(1)+flags(1)+packetid(2) = 5, no duration, no reason code */
    sleepresp_roundtrip("SLEEPRESP no Sleep Duration, Success",
                         0, 1, 0, MQTT_SN_RC_SUCCESS, 5);
}

static void test_sleepresp_with_duration_success(void)
{
    /* +duration(4) = 9, reason code still omitted (success) */
    sleepresp_roundtrip("SLEEPRESP with Sleep Duration, Success",
                         1, 0x5678, 120, MQTT_SN_RC_SUCCESS, 9);
}

static void test_sleepresp_with_duration_and_reasoncode(void)
{
    /* +reasoncode(1) = 10 */
    sleepresp_roundtrip("SLEEPRESP with Sleep Duration and Reason Code",
                         1, 99, 0xFFFFFFFFu, MQTT_SN_RC_SERVER_BUSY, 10);
}

static void test_sleepresp_no_duration_with_reasoncode(void)
{
    /* len(1)+type(1)+flags(1)+packetid(2)+reasoncode(1) = 6 */
    sleepresp_roundtrip("SLEEPRESP no Sleep Duration, with Reason Code",
                         0, 42, 0, MQTT_SN_RC_UNSPECIFIED_ERROR, 6);
}


/* =========================================================================
 * WAKEUP test case
 * ========================================================================= */

static void test_wakeup_basic(void)
{
    wakeup_roundtrip("WAKEUP header-only packet");
}


/* =========================================================================
 * error handling
 * ========================================================================= */

static void test_sleepreq_serialize_buffer_too_short(void)
{
    uint8_t buf[3]; /* too small for any valid SLEEPREQ */

    printf("%-55s ", "SLEEPREQ buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_sleepreq(buf, (int32_t)sizeof(buf), 0, 1, 60)
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

static void test_sleepresp_serialize_buffer_too_short(void)
{
    uint8_t buf[3]; /* too small for any valid SLEEPRESP */

    printf("%-55s ", "SLEEPRESP buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_sleepresp(buf, (int32_t)sizeof(buf), 0, 1, 0,
                                   MQTT_SN_RC_SUCCESS)
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

static void test_wakeup_serialize_buffer_too_short(void)
{
    uint8_t buf[4];

    printf("%-55s ", "WAKEUP buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_wakeup(buf, 0) == MQTTSNPACKET_BUFFER_TOO_SHORT)
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
 * MQTTSNDeserialize_sleepreq rejects a packet whose own declared Length
 * field is larger than the buflen actually supplied - i.e. a heap buffer
 * that physically holds fewer bytes than the packet claims to contain.
 * Allocated as an exactly-sized heap buffer so that, under
 * AddressSanitizer, any regression that reads past it is caught as a
 * heap-buffer-overflow.
 */
static void test_sleepreq_buflen_shorter_than_declared_length(void)
{
    uint8_t  full[64];
    int32_t  fulllen;
    uint8_t* truncated;
    uint8_t  retain;
    uint16_t packetid;
    uint32_t duration;
    int32_t  drc;

    printf("%-55s ", "SLEEPREQ buflen shorter than declared packet length");

    fulllen = MQTTSNSerialize_sleepreq(full, (int32_t)sizeof(full), 1, 1, 60);
    if (fulllen <= 4) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    /* physically only 4 bytes are available, but the Length byte inside
     * them still declares the original (longer) packet length */
    truncated = malloc(4);
    memcpy(truncated, full, 4);

    drc = MQTTSNDeserialize_sleepreq(&retain, &packetid, &duration, truncated, 4);
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

static void test_sleepresp_buflen_shorter_than_declared_length(void)
{
    uint8_t  full[64];
    int32_t  fulllen;
    uint8_t* truncated;
    uint8_t  durationPresent;
    uint16_t packetid;
    uint32_t duration;
    uint8_t  returncode;
    int32_t  drc;

    printf("%-55s ", "SLEEPRESP buflen shorter than declared packet length");

    fulllen = MQTTSNSerialize_sleepresp(full, (int32_t)sizeof(full), 1, 1, 60,
                                         MQTT_SN_RC_SUCCESS);
    if (fulllen <= 4) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    truncated = malloc(4);
    memcpy(truncated, full, 4);

    drc = MQTTSNDeserialize_sleepresp(&durationPresent, &packetid, &duration,
                                       &returncode, truncated, 4);
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
 * are missing required trailing fields. Each one must be rejected by the
 * respective deserializer rather than reading past the declared length.
 */
static void test_sleepreq_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[8];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short for header (type only)",
          { 2, MQTTSN_SLEEPREQ }, 2 },
        { "too short for header (missing packetid)",
          { 3, MQTTSN_SLEEPREQ, 0x00 }, 3 },
        { "too short for header (only 1 byte of packetid)",
          { 4, MQTTSN_SLEEPREQ, 0x00, 0x00 }, 4 },
        { "missing duration (only 1 of 4 bytes)",
          { 6, MQTTSN_SLEEPREQ, 0x00, 0x00, 0x01, 0x00 }, 6 },
    };
    size_t i;
    int    ok = 1;

    printf("%-55s ", "SLEEPREQ malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t  buf[sizeof(cases[0].bytes)];
        uint8_t  retain;
        uint16_t packetid;
        uint32_t duration;
        int32_t  drc;

        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_sleepreq(&retain, &packetid, &duration,
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

static void test_sleepresp_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[8];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short for header (type only)",
          { 2, MQTTSN_SLEEPRESP }, 2 },
        { "too short for header (missing packetid)",
          { 3, MQTTSN_SLEEPRESP, 0x00 }, 3 },
        { "too short for header (only 1 byte of packetid)",
          { 4, MQTTSN_SLEEPRESP, 0x00, 0x00 }, 4 },
        { "Sleep Duration Flag set, duration missing entirely",
          { 5, MQTTSN_SLEEPRESP, 0x01, 0x00, 0x00 }, 5 },
        { "Sleep Duration Flag set, only 1 of 4 duration bytes",
          { 6, MQTTSN_SLEEPRESP, 0x01, 0x00, 0x00, 0x00 }, 6 },
    };
    size_t i;
    int    ok = 1;

    printf("%-55s ", "SLEEPRESP malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t  buf[sizeof(cases[0].bytes)];
        uint8_t  durationPresent;
        uint16_t packetid;
        uint32_t duration;
        uint8_t  returncode;
        int32_t  drc;

        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_sleepresp(&durationPresent, &packetid, &duration,
                                           &returncode, buf, cases[i].len);
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

static void test_wakeup_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[2];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short for header (length byte only)",
          { 2 }, 1 },
        { "wrong packet type",
          { 2, MQTTSN_WAKEUP + 1 }, 2 },
    };
    size_t i;
    int    ok = 1;

    printf("%-55s ", "WAKEUP malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t buf[sizeof(cases[0].bytes)];
        int32_t drc;

        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_wakeup(buf, cases[i].len);
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
    printf("=== MQTT-SN 2.0 SLEEPREQ / SLEEPRESP / WAKEUP round-trip tests ===\n");

    section("SLEEPREQ");
    test_sleepreq_retain_off();
    test_sleepreq_retain_on();
    test_sleepreq_duration_min();
    test_sleepreq_duration_max();
    test_sleepreq_packetid_max();

    section("SLEEPRESP");
    test_sleepresp_no_duration_success();
    test_sleepresp_with_duration_success();
    test_sleepresp_with_duration_and_reasoncode();
    test_sleepresp_no_duration_with_reasoncode();

    section("WAKEUP");
    test_wakeup_basic();

    section("error handling");
    test_sleepreq_serialize_buffer_too_short();
    test_sleepresp_serialize_buffer_too_short();
    test_wakeup_serialize_buffer_too_short();
    test_sleepreq_buflen_shorter_than_declared_length();
    test_sleepresp_buflen_shorter_than_declared_length();
    test_sleepreq_deserialize_malformed();
    test_sleepresp_deserialize_malformed();
    test_wakeup_deserialize_malformed();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
