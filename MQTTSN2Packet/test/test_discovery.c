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
 * Round-trip tests for MQTTSNDiscoveryClient.c / MQTTSNDiscoveryServer.c
 * (ADVERTISE, SEARCHGW and GWINFO packets, Section 3.20 Gateway Discovery
 * Packets).
 *
 * Each test serializes a packet with a specific combination of field values,
 * then passes the resulting wire buffer to the deserializer and checks that
 * every field survives unchanged.
 *
 * Coverage:
 *   ADVERTISE - Gateway Identifier / Duration boundaries
 *   SEARCHGW  - optional Additional Network Information (absent, present)
 *   GWINFO    - optional Gateway Address (absent = sent by Gateway,
 *               present = sent by a Client)
 *   error handling - buffer too short (serialize)
 *                   - buflen shorter than the packet's own declared length is
 *                     rejected rather than read past (deserialize)
 *                   - malformed/short packets rejected by deserialize
 *                     (hand-crafted wire bytes whose own length field is
 *                     self-consistent with the supplied buflen, but which are
 *                     missing required trailing fields)
 */

#include "MQTTSNPacket.h"
#include "MQTTSNDiscovery.h"

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
 * ADVERTISE round-trip helper
 * ========================================================================= */

static int advertise_roundtrip(const char* test_name, uint8_t in_gatewayId,
        uint16_t in_duration, int32_t expected_wire_len)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    uint8_t  out_gatewayId = 0xFF;
    uint16_t out_duration = 0;
    int32_t  drc;
    int      ok = 1;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));

    slen = MQTTSNSerialize_advertise(buf, (int32_t)sizeof(buf),
                                      in_gatewayId, in_duration);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_advertise(&out_gatewayId, &out_duration, buf, slen);
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

    RT_CHECK("gatewayId", out_gatewayId == in_gatewayId);
    RT_CHECK("duration",  out_duration  == in_duration);

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
 * SEARCHGW round-trip helper
 * ========================================================================= */

static int searchgw_roundtrip(const char* test_name,
        const MQTTSN_data* in_networkInfo, int32_t expected_wire_len)
{
    uint8_t     buf[BUF_SIZE];
    int32_t     slen;
    MQTTSN_data out_networkInfo;
    int32_t     drc;
    int         ok = 1;
    uint16_t    in_len = (in_networkInfo == NULL) ? 0 : in_networkInfo->len;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out_networkInfo, 0, sizeof(out_networkInfo));

    slen = MQTTSNSerialize_searchgw(buf, (int32_t)sizeof(buf), in_networkInfo);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_searchgw(&out_networkInfo, buf, slen);
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

    RT_CHECK("networkInfo.len", out_networkInfo.len == in_len);
    RT_CHECK("networkInfo.data",
             in_len == 0 ||
             memcmp(out_networkInfo.data, in_networkInfo->data, in_len) == 0);

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
 * GWINFO round-trip helper
 * ========================================================================= */

static int gwinfo_roundtrip(const char* test_name, uint8_t in_gatewayId,
        const MQTTSN_data* in_gatewayAddress, int32_t expected_wire_len)
{
    uint8_t     buf[BUF_SIZE];
    int32_t     slen;
    uint8_t     out_gatewayId = 0xFF;
    MQTTSN_data out_gatewayAddress;
    int32_t     drc;
    int         ok = 1;
    uint16_t    in_len = (in_gatewayAddress == NULL) ? 0 : in_gatewayAddress->len;

    printf("%-55s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out_gatewayAddress, 0, sizeof(out_gatewayAddress));

    slen = MQTTSNSerialize_gwinfo(buf, (int32_t)sizeof(buf),
                                   in_gatewayId, in_gatewayAddress);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_gwinfo(&out_gatewayId, &out_gatewayAddress, buf, slen);
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

    RT_CHECK("gatewayId",           out_gatewayId          == in_gatewayId);
    RT_CHECK("gatewayAddress.len",  out_gatewayAddress.len == in_len);
    RT_CHECK("gatewayAddress.data",
             in_len == 0 ||
             memcmp(out_gatewayAddress.data, in_gatewayAddress->data, in_len) == 0);

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
 * ADVERTISE test cases
 * ========================================================================= */

static void test_advertise_typical(void)
{
    /* len(1)+type(1)+gatewayId(1)+duration(2) = 5 */
    advertise_roundtrip("ADVERTISE typical", 1, 900, 5);
}

static void test_advertise_min(void)
{
    advertise_roundtrip("ADVERTISE gatewayId 0, duration 0", 0, 0, 5);
}

static void test_advertise_max(void)
{
    advertise_roundtrip("ADVERTISE gatewayId 0xFF, duration 0xFFFF (max)",
                         0xFF, 0xFFFF, 5);
}


/* =========================================================================
 * SEARCHGW test cases
 * ========================================================================= */

static void test_searchgw_no_networkinfo(void)
{
    /* len(1)+type(1) = 2, no Additional Network Information */
    searchgw_roundtrip("SEARCHGW no Additional Network Information", NULL, 2);
}

static void test_searchgw_with_networkinfo(void)
{
    unsigned char   bytes[] = { 0x05 }; /* e.g. ZigBee broadcast radius */
    MQTTSN_data     info = { (uint16_t)sizeof(bytes), bytes };

    /* len(1)+type(1)+info(1) = 3 */
    searchgw_roundtrip("SEARCHGW with Additional Network Information",
                        &info, 3);
}

static void test_searchgw_with_longer_networkinfo(void)
{
    unsigned char   bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    MQTTSN_data     info = { (uint16_t)sizeof(bytes), bytes };

    searchgw_roundtrip("SEARCHGW with longer Additional Network Information",
                        &info, -1);
}


/* =========================================================================
 * GWINFO test cases
 * ========================================================================= */

static void test_gwinfo_sent_by_gateway(void)
{
    /* len(1)+type(1)+gatewayId(1) = 3, no Gateway Address (sent by Gateway) */
    gwinfo_roundtrip("GWINFO sent by Gateway (no address)", 7, NULL, 3);
}

static void test_gwinfo_sent_by_client(void)
{
    unsigned char   addr[] = { 192, 168, 1, 42 }; /* e.g. IPv4 address */
    MQTTSN_data     address = { (uint16_t)sizeof(addr), addr };

    /* len(1)+type(1)+gatewayId(1)+address(4) = 7 */
    gwinfo_roundtrip("GWINFO sent by Client (with address)", 3, &address, 7);
}

static void test_gwinfo_max_gatewayid(void)
{
    gwinfo_roundtrip("GWINFO gatewayId maximum (0xFF), no address", 0xFF, NULL, 3);
}


/* =========================================================================
 * error handling
 * ========================================================================= */

static void test_advertise_serialize_buffer_too_short(void)
{
    uint8_t buf[3]; /* too small for any valid ADVERTISE */

    printf("%-55s ", "ADVERTISE buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_advertise(buf, (int32_t)sizeof(buf), 1, 900)
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

static void test_searchgw_serialize_buffer_too_short(void)
{
    uint8_t         buf[1]; /* too small for any valid SEARCHGW */
    unsigned char   bytes[] = { 0x01, 0x02 };
    MQTTSN_data     info = { (uint16_t)sizeof(bytes), bytes };

    printf("%-55s ", "SEARCHGW buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_searchgw(buf, (int32_t)sizeof(buf), &info)
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

static void test_gwinfo_serialize_buffer_too_short(void)
{
    uint8_t buf[2]; /* too small for any valid GWINFO */

    printf("%-55s ", "GWINFO buffer too short (serialize)");
    ++tests_run;
    if (MQTTSNSerialize_gwinfo(buf, (int32_t)sizeof(buf), 1, NULL)
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
 * MQTTSNDeserialize_advertise rejects a packet whose own declared Length
 * field is larger than the buflen actually supplied - i.e. a heap buffer
 * that physically holds fewer bytes than the packet claims to contain.
 * Allocated as an exactly-sized heap buffer so that, under
 * AddressSanitizer, any regression that reads past it is caught as a
 * heap-buffer-overflow.
 */
static void test_advertise_buflen_shorter_than_declared_length(void)
{
    uint8_t  full[64];
    int32_t  fulllen;
    uint8_t* truncated;
    uint8_t  gatewayId;
    uint16_t duration;
    int32_t  drc;

    printf("%-55s ", "ADVERTISE buflen shorter than declared packet length");

    fulllen = MQTTSNSerialize_advertise(full, (int32_t)sizeof(full), 1, 900);
    if (fulllen <= 3) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    /* physically only 3 bytes are available, but the Length byte inside
     * them still declares the original (longer) packet length */
    truncated = malloc(3);
    memcpy(truncated, full, 3);

    drc = MQTTSNDeserialize_advertise(&gatewayId, &duration, truncated, 3);
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

static void test_searchgw_buflen_shorter_than_declared_length(void)
{
    uint8_t         full[64];
    int32_t         fulllen;
    uint8_t*        truncated;
    MQTTSN_data     networkInfo;
    int32_t         drc;
    unsigned char   bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    MQTTSN_data     info = { (uint16_t)sizeof(bytes), bytes };

    printf("%-55s ", "SEARCHGW buflen shorter than declared packet length");

    fulllen = MQTTSNSerialize_searchgw(full, (int32_t)sizeof(full), &info);
    if (fulllen <= 3) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    truncated = malloc(3);
    memcpy(truncated, full, 3);

    memset(&networkInfo, 0, sizeof(networkInfo));
    drc = MQTTSNDeserialize_searchgw(&networkInfo, truncated, 3);
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

static void test_gwinfo_buflen_shorter_than_declared_length(void)
{
    uint8_t       full[64];
    int32_t       fulllen;
    uint8_t*      truncated;
    uint8_t       gatewayId;
    MQTTSN_data   gatewayAddress;
    int32_t       drc;
    unsigned char addr[] = { 192, 168, 1, 42, 0, 1, 2, 3 };
    MQTTSN_data   address = { (uint16_t)sizeof(addr), addr };

    printf("%-55s ", "GWINFO buflen shorter than declared packet length");

    fulllen = MQTTSNSerialize_gwinfo(full, (int32_t)sizeof(full), 5, &address);
    if (fulllen <= 3) {
        ++tests_run; ++tests_failed;
        printf("FAIL  (setup serialize returned %d)\n", (int)fulllen);
        return;
    }

    truncated = malloc(3);
    memcpy(truncated, full, 3);

    memset(&gatewayAddress, 0, sizeof(gatewayAddress));
    drc = MQTTSNDeserialize_gwinfo(&gatewayId, &gatewayAddress, truncated, 3);
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
static void test_advertise_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[4];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short for header (type only)",
          { 2, MQTTSN_ADVERTISE }, 2 },
        { "gatewayId present, duration missing entirely",
          { 3, MQTTSN_ADVERTISE, 0x05 }, 3 },
        { "missing duration (only 1 of 2 bytes)",
          { 4, MQTTSN_ADVERTISE, 0x01, 0x00 }, 4 },
    };
    size_t i;
    int    ok = 1;

    printf("%-55s ", "ADVERTISE malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t  buf[sizeof(cases[0].bytes)];
        uint8_t  gatewayId;
        uint16_t duration;
        int32_t  drc;

        memcpy(buf, cases[i].bytes, sizeof(buf));

        drc = MQTTSNDeserialize_advertise(&gatewayId, &duration, buf, cases[i].len);
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

static void test_searchgw_deserialize_malformed(void)
{
    /* SEARCHGW's only body field is optional, so the sole way to be
     * malformed/short is to lack the mandatory type byte itself. */
    uint8_t     buf[2] = { 1, MQTTSN_SEARCHGW }; /* Length says 1, i.e. no type byte */
    MQTTSN_data networkInfo;
    int32_t     drc;

    printf("%-55s ", "SEARCHGW malformed/short packets rejected");

    memset(&networkInfo, 0, sizeof(networkInfo));
    drc = MQTTSNDeserialize_searchgw(&networkInfo, buf, 1);

    ++tests_run;
    if (drc != 1)
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

static void test_gwinfo_deserialize_malformed(void)
{
    struct malformed_case
    {
        const char* label;
        uint8_t     bytes[3];
        int32_t     len;
    };

    static const struct malformed_case cases[] = {
        { "too short for header (type only)",
          { 2, MQTTSN_GWINFO }, 2 },
    };
    size_t i;
    int    ok = 1;

    printf("%-55s ", "GWINFO malformed/short packets rejected");

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        uint8_t     buf[sizeof(cases[0].bytes)];
        uint8_t     gatewayId;
        MQTTSN_data gatewayAddress;
        int32_t     drc;

        memcpy(buf, cases[i].bytes, sizeof(buf));
        memset(&gatewayAddress, 0, sizeof(gatewayAddress));

        drc = MQTTSNDeserialize_gwinfo(&gatewayId, &gatewayAddress, buf, cases[i].len);
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
    printf("=== MQTT-SN 2.0 ADVERTISE / SEARCHGW / GWINFO round-trip tests ===\n");

    section("ADVERTISE");
    test_advertise_typical();
    test_advertise_min();
    test_advertise_max();

    section("SEARCHGW");
    test_searchgw_no_networkinfo();
    test_searchgw_with_networkinfo();
    test_searchgw_with_longer_networkinfo();

    section("GWINFO");
    test_gwinfo_sent_by_gateway();
    test_gwinfo_sent_by_client();
    test_gwinfo_max_gatewayid();

    section("error handling");
    test_advertise_serialize_buffer_too_short();
    test_searchgw_serialize_buffer_too_short();
    test_gwinfo_serialize_buffer_too_short();
    test_advertise_buflen_shorter_than_declared_length();
    test_searchgw_buflen_shorter_than_declared_length();
    test_gwinfo_buflen_shorter_than_declared_length();
    test_advertise_deserialize_malformed();
    test_searchgw_deserialize_malformed();
    test_gwinfo_deserialize_malformed();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
