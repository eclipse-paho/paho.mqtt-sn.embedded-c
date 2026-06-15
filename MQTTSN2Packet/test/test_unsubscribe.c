/*******************************************************************************
* Copyright (c) 2014, 2026 IBM Corp., Ian Craggs
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
 * Round-trip tests for MQTTSNUnsubscribeClient.c and MQTTSNUnsubscribeServer.c.
 *
 * Each test serializes a packet with a specific combination of option values,
 * then passes the resulting wire buffer to the corresponding deserializer and
 * checks that every field survives unchanged.
 *
 * Coverage:
 *   UNSUBSCRIBE - topic filter (short, wildcarded, long)
 *               - topic name (MQTTSN_TOPIC_TYPE_NAME, same wire type as FILTER)
 *               - session alias (minimum and maximum)
 *               - predefined alias (minimum and maximum)
 *               - packet identifier boundary values (1 and 0xFFFF)
 *               - buffer too short rejection
 *   UNSUBACK    - success (Reason Code absent from wire)
 *               - non-zero Reason Code present on wire
 *               - wire length checks for both cases
 *               - buffer too short rejection
 */

#include "MQTTSNPacket.h"
#include "MQTTSNUnsubscribe.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test infrastructure
 * ------------------------------------------------------------------------- */

#define BUF_SIZE 512

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


/* =========================================================================
 * UNSUBSCRIBE round-trip helper
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_unsubscribe (client), deserialize with
 * MQTTSNDeserialize_unsubscribe (server), then check every field.
 */
static int unsubscribe_roundtrip(const char* test_name,
        uint16_t in_packetid, MQTTSN_topic* in_topic)
{
    uint8_t      buf[BUF_SIZE];
    int32_t      slen;
    uint16_t     out_packetid = 0;
    MQTTSN_topic out_topic;
    int32_t      drc;
    int          ok = 1;

    printf("%-55s ", test_name);

    memset(buf,      0, sizeof(buf));
    memset(&out_topic, 0, sizeof(out_topic));

    /* --- serialize (client side) --- */
    slen = MQTTSNSerialize_unsubscribe(buf, (int32_t)sizeof(buf),
                                        in_packetid, in_topic);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    /* --- deserialize (server side) --- */
    drc = MQTTSNDeserialize_unsubscribe(&out_packetid, &out_topic, buf, slen);
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

    RT_CHECK("packetid",    out_packetid    == in_packetid);
    RT_CHECK("topic.type",  out_topic.type  == in_topic->type);

    if (in_topic->type == MQTTSN_TOPIC_TYPE_FILTER ||
        in_topic->type == MQTTSN_TOPIC_TYPE_NAME)
    {
        RT_CHECK("string.len",
                 out_topic.alt.string.len == in_topic->alt.string.len);
        RT_CHECK("string.data",
                 memcmp(out_topic.alt.string.data,
                        in_topic->alt.string.data,
                        in_topic->alt.string.len) == 0);
    }
    else /* SESSION or PREDEFINED */
    {
        RT_CHECK("alias", out_topic.alt.alias == in_topic->alt.alias);
    }

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}


/* =========================================================================
 * UNSUBACK round-trip helper
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_unsuback (server), deserialize with
 * MQTTSNDeserialize_unsuback (client), then check every field and wire length.
 */
static int unsuback_roundtrip(const char* test_name,
        uint16_t in_packetid, uint8_t in_returncode,
        int32_t expected_wire_len)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    uint16_t out_packetid   = 0;
    uint8_t  out_returncode = 0xFF;
    int32_t  drc;
    int      ok = 1;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));

    /* --- serialize (server side) --- */
    slen = MQTTSNSerialize_unsuback(buf, (int32_t)sizeof(buf),
                                     in_packetid, in_returncode);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    /* --- deserialize (client side) --- */
    drc = MQTTSNDeserialize_unsuback(&out_packetid, &out_returncode, buf, slen);
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

    RT_CHECK("packetid",     out_packetid    == in_packetid);
    RT_CHECK("returncode",   out_returncode  == in_returncode);
    RT_CHECK("wire length",  slen            == expected_wire_len);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}


/* =========================================================================
 * UNSUBSCRIBE test cases
 * ========================================================================= */

/* topic type: FILTER ------------------------------------------------------- */

static void test_unsubscribe_filter_single_level(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
                       { .string = { false, 1, "#" } } };
    unsubscribe_roundtrip("UNSUBSCRIBE FILTER single-level wildcard \"#\"",
                          1, &t);
}

static void test_unsubscribe_filter_multilevel(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
                       { .string = { false, 15, "building/+/temp" } } };
    unsubscribe_roundtrip("UNSUBSCRIBE FILTER multilevel \"building/+/temp\"",
                          2, &t);
}

static void test_unsubscribe_filter_long(void)
{
    static const char filter[] = "very/long/topic/filter/path/for/testing/purposes";
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
                       { .string = { false,
                                     (uint16_t)(sizeof(filter) - 1),
                                     (char*)filter } } };
    unsubscribe_roundtrip("UNSUBSCRIBE FILTER long path (48 chars)",
                          3, &t);
}

/* topic type: NAME --------------------------------------------------------- */

static void test_unsubscribe_name(void)
{
    /* MQTTSN_TOPIC_TYPE_NAME == MQTTSN_TOPIC_TYPE_FILTER on the wire (0b11);
     * verify that the enum alias round-trips correctly through the flags byte */
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_NAME,
                       { .string = { false, 13, "devices/dev42" } } };
    unsubscribe_roundtrip("UNSUBSCRIBE NAME (alias for FILTER, 0b11)",
                          4, &t);
}

/* topic type: SESSION alias ------------------------------------------------ */

static void test_unsubscribe_session_min(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 1 } };
    unsubscribe_roundtrip("UNSUBSCRIBE SESSION alias 1 (minimum)",
                          10, &t);
}

static void test_unsubscribe_session_typical(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 0x1234 } };
    unsubscribe_roundtrip("UNSUBSCRIBE SESSION alias 0x1234",
                          11, &t);
}

static void test_unsubscribe_session_max(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 0xFFFF } };
    unsubscribe_roundtrip("UNSUBSCRIBE SESSION alias 0xFFFF (maximum)",
                          12, &t);
}

/* topic type: PREDEFINED alias --------------------------------------------- */

static void test_unsubscribe_predefined_min(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 1 } };
    unsubscribe_roundtrip("UNSUBSCRIBE PREDEFINED alias 1 (minimum)",
                          20, &t);
}

static void test_unsubscribe_predefined_typical(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 0x00AB } };
    unsubscribe_roundtrip("UNSUBSCRIBE PREDEFINED alias 0x00AB",
                          21, &t);
}

static void test_unsubscribe_predefined_max(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 0xFFFF } };
    unsubscribe_roundtrip("UNSUBSCRIBE PREDEFINED alias 0xFFFF (maximum)",
                          22, &t);
}

/* packet identifier boundary values --------------------------------------- */

static void test_unsubscribe_packetid_min(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 1 } };
    unsubscribe_roundtrip("UNSUBSCRIBE packetid 1 (minimum non-zero)",
                          1, &t);
}

static void test_unsubscribe_packetid_max(void)
{
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 1 } };
    unsubscribe_roundtrip("UNSUBSCRIBE packetid 0xFFFF (maximum)",
                          0xFFFF, &t);
}

/* error handling ---------------------------------------------------------- */

static void test_unsubscribe_buffer_too_short(void)
{
    uint8_t buf[3]; /* too small for any valid UNSUBSCRIBE */
    MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
                       { .string = { false, 4, "test" } } };

    printf("%-55s ", "UNSUBSCRIBE buffer too short");
    ++tests_run;
    if (MQTTSNSerialize_unsubscribe(buf, (int32_t)sizeof(buf), 1, &t)
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


/* =========================================================================
 * UNSUBACK test cases
 * ========================================================================= */

static void test_unsuback_success(void)
{
    /* success: length(1)+type(1)+packetid(2) = 4 bytes; reason code omitted */
    unsuback_roundtrip("UNSUBACK success (reason code absent from wire)",
                       1, MQTT_SN_RC_SUCCESS, 4);
}

static void test_unsuback_no_subscription_existed(void)
{
    /* 0x11: length(1)+type(1)+packetid(2)+rc(1) = 5 bytes */
    unsuback_roundtrip("UNSUBACK 0x11 (no subscription existed)",
                       2, MQTT_SN_RC_NO_SUBSCRIPTION_EXISTED, 5);
}

static void test_unsuback_unspecified_error(void)
{
    unsuback_roundtrip("UNSUBACK 0x80 (unspecified error)",
                       3, MQTT_SN_RC_UNSPECIFIED_ERROR, 5);
}

static void test_unsuback_not_authorized(void)
{
    unsuback_roundtrip("UNSUBACK 0x87 (not authorized)",
                       4, MQTT_SN_RC_NOT_AUTHORIZED, 5);
}

static void test_unsuback_packetid_max(void)
{
    unsuback_roundtrip("UNSUBACK packetid 0xFFFF success",
                       0xFFFF, MQTT_SN_RC_SUCCESS, 4);
}

static void test_unsuback_buffer_too_short(void)
{
    uint8_t buf[2]; /* too small for any valid UNSUBACK */

    printf("%-55s ", "UNSUBACK buffer too short");
    ++tests_run;
    if (MQTTSNSerialize_unsuback(buf, (int32_t)sizeof(buf),
                                  1, MQTT_SN_RC_SUCCESS)
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


/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    printf("=== MQTT-SN 2.0 UNSUBSCRIBE / UNSUBACK round-trip tests ===\n");

    section("UNSUBSCRIBE — topic type FILTER");
    test_unsubscribe_filter_single_level();
    test_unsubscribe_filter_multilevel();
    test_unsubscribe_filter_long();

    section("UNSUBSCRIBE — topic type NAME (same wire encoding as FILTER)");
    test_unsubscribe_name();

    section("UNSUBSCRIBE — topic type SESSION alias");
    test_unsubscribe_session_min();
    test_unsubscribe_session_typical();
    test_unsubscribe_session_max();

    section("UNSUBSCRIBE — topic type PREDEFINED alias");
    test_unsubscribe_predefined_min();
    test_unsubscribe_predefined_typical();
    test_unsubscribe_predefined_max();

    section("UNSUBSCRIBE — packet identifier boundary values");
    test_unsubscribe_packetid_min();
    test_unsubscribe_packetid_max();

    section("UNSUBSCRIBE — error handling");
    test_unsubscribe_buffer_too_short();

    section("UNSUBACK — reason codes and wire length");
    test_unsuback_success();
    test_unsuback_no_subscription_existed();
    test_unsuback_unspecified_error();
    test_unsuback_not_authorized();
    test_unsuback_packetid_max();

    section("UNSUBACK — error handling");
    test_unsuback_buffer_too_short();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
