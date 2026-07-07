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
 * Round-trip tests for MQTTSNAuth.c (AUTH packet, Section 3.3).
 *
 * Each test serializes an AUTH packet with a specific combination of
 * field values, then passes the resulting wire buffer to the deserializer
 * and checks that every field survives unchanged.
 *
 * Coverage:
 *   AUTH - reason codes (success, continue authentication, re-authenticate)
 *        - authentication method (short, longer, empty)
 *        - authentication data (empty, short, longer binary payload
 *          including embedded zero bytes)
 *        - packet identifier boundary values (0, 1, 0xFFFF)
 *        - minimal packet (empty method and empty data together)
 *        - wire length check
 *        - buffer too short rejection (serialize)
 *        - buflen shorter than the packet's own declared length is
 *          rejected rather than read past (deserialize)
 *        - malformed/short packets rejected by deserialize (hand-crafted
 *          wire bytes whose own length field is self-consistent with the
 *          supplied buflen, but which are missing required trailing fields)
 */

#include "MQTTSNPacket.h"
#include "MQTTSNAuth.h"

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
 * AUTH round-trip helper
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_auth, deserialize with
 * MQTTSNDeserialize_auth, then check every field and the wire length.
 * expected_wire_len may be -1 to skip the wire length check.
 */
static int auth_roundtrip(const char* test_name,
        uint16_t in_packetid, uint8_t in_reasoncode,
        const MQTTSN_string* in_method, const MQTTSN_data* in_data,
        int32_t expected_wire_len)
{
    uint8_t       buf[BUF_SIZE];
    int32_t       slen;
    uint16_t      out_packetid   = 0;
    uint8_t       out_reasoncode = 0xFF;
    MQTTSN_string out_method;
    MQTTSN_data   out_data;
    int32_t       drc;
    int           ok = 1;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out_method, 0, sizeof(out_method));
    memset(&out_data,   0, sizeof(out_data));

    /* --- serialize --- */
    slen = MQTTSNSerialize_auth(buf, (int32_t)sizeof(buf),
                                 in_packetid, in_reasoncode, in_method, in_data);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    /* --- deserialize --- */
    drc = MQTTSNDeserialize_auth(&out_packetid, &out_reasoncode,
                                  &out_method, &out_data, buf, slen);
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
    RT_CHECK("reasonCode",  out_reasoncode  == in_reasoncode);
    RT_CHECK("method.len",  out_method.len  == in_method->len);
    RT_CHECK("method.data",
             in_method->len == 0 ||
             memcmp(out_method.data, in_method->data, in_method->len) == 0);
    RT_CHECK("data.len",    out_data.len    == in_data->len);
    RT_CHECK("data.data",
             in_data->len == 0 ||
             memcmp(out_data.data, in_data->data, in_data->len) == 0);

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
 * AUTH test cases
 * ========================================================================= */

/* reason codes -------------------------------------------------------------- */

static void test_auth_success(void)
{
    MQTTSN_string method = { 1, 5, "PLAIN" };
    unsigned char data[] = { 0x00, 'u', 's', 'e', 'r', 0x00, 'p', 'a', 's', 's' };
    MQTTSN_data   authdata = { (uint16_t)sizeof(data), data };

    /* len(1)+type(1)+packetid(2)+rc(1)+methodlen(1)+method(5)+data(10) = 21 */
    auth_roundtrip("AUTH success (rc=0x00)",
                   1, MQTT_SN_RC_SUCCESS, &method, &authdata, 21);
}

static void test_auth_continue(void)
{
    MQTTSN_string method = { 1, 11, "SCRAM-SHA-1" };
    unsigned char data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    MQTTSN_data   authdata = { (uint16_t)sizeof(data), data };

    auth_roundtrip("AUTH continue authentication (rc=0x18)",
                   2, MQTT_SN_RC_CONTINUE_AUTH, &method, &authdata, -1);
}

static void test_auth_reauthenticate(void)
{
    MQTTSN_string method = { 1, 5, "PLAIN" };
    MQTTSN_data   authdata = { 0, NULL };

    auth_roundtrip("AUTH re-authenticate (rc=0x19)",
                   3, MQTT_SN_RC_RE_AUTHENTICATE, &method, &authdata, -1);
}

/* authentication method ------------------------------------------------------ */

static void test_auth_method_empty(void)
{
    MQTTSN_string method = { 1, 0, NULL };
    unsigned char data[] = { 0x01 };
    MQTTSN_data   authdata = { (uint16_t)sizeof(data), data };

    auth_roundtrip("AUTH empty authentication method",
                   4, MQTT_SN_RC_SUCCESS, &method, &authdata, -1);
}

static void test_auth_method_long(void)
{
    static const char m[] = "GS2-KRB5-WITH-A-VERY-LONG-MECHANISM-NAME";
    MQTTSN_string method = { 1, (uint16_t)(sizeof(m) - 1), (char*)m };
    unsigned char data[] = { 0x01, 0x02, 0x03 };
    MQTTSN_data   authdata = { (uint16_t)sizeof(data), data };

    auth_roundtrip("AUTH long authentication method (41 chars)",
                   5, MQTT_SN_RC_CONTINUE_AUTH, &method, &authdata, -1);
}

/* authentication data --------------------------------------------------------- */

static void test_auth_data_empty(void)
{
    MQTTSN_string method = { 1, 5, "PLAIN" };
    MQTTSN_data   authdata = { 0, NULL };

    auth_roundtrip("AUTH empty authentication data",
                   6, MQTT_SN_RC_SUCCESS, &method, &authdata, -1);
}

static void test_auth_data_long(void)
{
    static unsigned char data[300];
    unsigned int i;
    MQTTSN_string method = { 1, 5, "PLAIN" };
    MQTTSN_data   authdata;

    for (i = 0; i < sizeof(data); ++i)
        data[i] = (unsigned char)i;
    authdata.len  = (uint16_t)sizeof(data);
    authdata.data = data;

    auth_roundtrip("AUTH long authentication data (300 bytes)",
                   7, MQTT_SN_RC_CONTINUE_AUTH, &method, &authdata, -1);
}

/* packet identifier boundary values ------------------------------------------- */

static void test_auth_packetid_zero(void)
{
    MQTTSN_string method = { 1, 5, "PLAIN" };
    MQTTSN_data   authdata = { 0, NULL };

    auth_roundtrip("AUTH packetid 0 (minimum)",
                   0, MQTT_SN_RC_SUCCESS, &method, &authdata, -1);
}

static void test_auth_packetid_max(void)
{
    MQTTSN_string method = { 1, 5, "PLAIN" };
    MQTTSN_data   authdata = { 0, NULL };

    auth_roundtrip("AUTH packetid 0xFFFF (maximum)",
                   0xFFFF, MQTT_SN_RC_SUCCESS, &method, &authdata, -1);
}

/* minimal packet -------------------------------------------------------------- */

static void test_auth_minimal(void)
{
    MQTTSN_string method = { 1, 0, NULL };
    MQTTSN_data   authdata = { 0, NULL };

    /* len(1)+type(1)+packetid(2)+rc(1)+methodlen(1) = 6; no method/data bytes */
    auth_roundtrip("AUTH minimal packet (empty method + empty data)",
                   9, MQTT_SN_RC_SUCCESS, &method, &authdata, 6);
}

/* error handling --------------------------------------------------------------- */

static void test_auth_serialize_buffer_too_short(void)
{
    uint8_t buf[4]; /* too small for any valid AUTH */
    MQTTSN_string method = { 1, 5, "PLAIN" };
    MQTTSN_data   authdata = { 0, NULL };

    printf("%-55s ", "AUTH buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_auth(buf, (int32_t)sizeof(buf),
                              1, MQTT_SN_RC_SUCCESS, &method, &authdata)
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
 * MQTTSNDeserialize_auth rejects a packet whose own declared Length field
 * is larger than the buflen actually supplied - i.e. a heap buffer that
 * physically holds fewer bytes than the packet claims to contain (as if a
 * truncated datagram had been delivered but the Length byte, being part of
 * the truncated prefix, still reflects the original full length). This is
 * allocated as an exactly-sized heap buffer so that, under AddressSanitizer,
 * any regression that reads past it is caught as a heap-buffer-overflow.
 */
static void test_auth_buflen_shorter_than_declared_length(void)
{
    uint8_t       full[64];
    int32_t       fulllen;
    uint8_t*      truncated;
    uint16_t      pid;
    uint8_t       rc;
    MQTTSN_string m;
    MQTTSN_data   d;
    int32_t       drc;
    MQTTSN_string method = { 1, 11, "SCRAM-SHA-1" };
    unsigned char data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    MQTTSN_data   authdata = { (uint16_t)sizeof(data), data };

    printf("%-55s ", "AUTH buflen shorter than declared packet length");

    fulllen = MQTTSNSerialize_auth(full, (int32_t)sizeof(full),
                                    1, MQTT_SN_RC_CONTINUE_AUTH, &method, &authdata);
    if (fulllen <= 4) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    /* physically only 4 bytes are available, but the Length byte inside
     * them still declares the original (longer) packet length */
    truncated = malloc(4);
    memcpy(truncated, full, 4);

    memset(&m, 0, sizeof(m));
    memset(&d, 0, sizeof(d));
    drc = MQTTSNDeserialize_auth(&pid, &rc, &m, &d, truncated, 4);
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
        printf("FAIL  (unexpectedly accepted)\n");
    }
}

/*
 * Hand-crafted wire buffers that are self-consistent (the Length field
 * correctly matches the number of bytes actually supplied, exactly as a
 * genuinely short/truncated packet received off the wire would look) but
 * are missing required trailing fields. Each one must be rejected by
 * MQTTSNDeserialize_auth rather than reading past the declared length.
 *
 * NOTE: this is a separate scenario from
 * test_auth_buflen_shorter_than_declared_length() above. Here buflen
 * always matches the packet's own declared Length field; these cases
 * probe what happens when that (now validated) length is simply too
 * small to hold the fields the packet claims to contain.
 */
static void test_auth_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[6];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short for header (type only)",
          { 2, MQTTSN_AUTH }, 2 },
        { "too short for header (type + packetid)",
          { 4, MQTTSN_AUTH, 0x00, 0x01 }, 4 },
        { "too short for header (missing methodLen)",
          { 5, MQTTSN_AUTH, 0x00, 0x01, MQTT_SN_RC_SUCCESS }, 5 },
        { "methodLen declares 5 bytes, none present",
          { 6, MQTTSN_AUTH, 0x00, 0x01, MQTT_SN_RC_SUCCESS, 5 }, 6 },
    };
    size_t i;
    int    ok = 1;

    printf("%-55s ", "AUTH malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t       buf[sizeof(cases[0].bytes)];
        uint16_t      pid;
        uint8_t       rc;
        MQTTSN_string m;
        MQTTSN_data   d;
        int32_t       drc;

        memset(&m, 0, sizeof(m));
        memset(&d, 0, sizeof(d));
        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_auth(&pid, &rc, &m, &d, buf, cases[i].len);
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
    printf("=== MQTT-SN 2.0 AUTH round-trip tests ===\n");

    section("AUTH — reason codes");
    test_auth_success();
    test_auth_continue();
    test_auth_reauthenticate();

    section("AUTH — authentication method");
    test_auth_method_empty();
    test_auth_method_long();

    section("AUTH — authentication data");
    test_auth_data_empty();
    test_auth_data_long();

    section("AUTH — packet identifier boundary values");
    test_auth_packetid_zero();
    test_auth_packetid_max();

    section("AUTH — minimal packet");
    test_auth_minimal();

    section("AUTH — error handling");
    test_auth_serialize_buffer_too_short();
    test_auth_buflen_shorter_than_declared_length();
    test_auth_deserialize_malformed();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
