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
 * Round-trip tests for MQTTSNConnectClient.c and MQTTSNConnectServer.c.
 *
 * Each test serializes a packet with a specific combination of option values,
 * then passes the resulting wire buffer to the corresponding deserializer and
 * checks that every field survives unchanged.
 *
 * Coverage:
 *   CONNECT   - minimal; cleanStart; keepAlive / maxPacketSize / packetId
 *               boundaries; will with session alias, predefined alias, and
 *               topic name type; defaultAwakeMessages; sessionExpiryInterval;
 *               auth block; all flags simultaneously; empty clientId;
 *               buffer too short
 *   CONNACK   - minimal success; non-zero reason code; sessionPresent;
 *               sessionExpiryInterval; serverKeepAlive; auth block;
 *               assignedClientID; all optional fields; buffer too short
 *   DISCONNECT - success (reason code omitted from wire); non-zero reason
 *                code; buffer too short
 *   PINGREQ   - typical packet identifier; minimum (1) and maximum (0xFFFF)
 *               packet identifier; wire length is always 4 bytes; buffer too short
 *   PINGRESP  - no AMR field (wire length 4); AMR=0, typical, and 0xFF
 *               (wire length 5); packet identifier echoed correctly; buffer too short
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test infrastructure
 * ------------------------------------------------------------------------- */

#define BUF_SIZE 1024

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
 * CONNECT / CONNACK round-trip helpers
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_connect, deserialize with
 * MQTTSNDeserialize_connect, then check every field.
 */
static int connect_roundtrip(const char* test_name,
        const MQTTSNPacket_connectData* in)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    MQTTSNPacket_connectData out;
    int32_t  drc;
    int      ok = 1;

    printf("%-58s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out, 0, sizeof(out));

    slen = MQTTSNSerialize_connect(buf, (int32_t)sizeof(buf), in);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_connect(&out, buf, slen);
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

    RT_CHECK("flags.all",      out.flags.all      == in->flags.all);
    RT_CHECK("packetId",       out.packetId        == in->packetId);
    RT_CHECK("keepAlive",      out.keepAlive        == in->keepAlive);
    RT_CHECK("maxPacketSize",  out.maxPacketSize    == in->maxPacketSize);

    if (in->flags.bits.will)
    {
        RT_CHECK("willFlags.all", out.willFlags.all == in->willFlags.all);

        if (in->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_SESSION ||
            in->willFlags.bits.topicType == MQTTSN_TOPIC_TYPE_PREDEFINED)
        {
            RT_CHECK("will.topic.alt.alias",
                     out.will.topic.alt.alias == in->will.topic.alt.alias);
        }
        else /* MQTTSN_TOPIC_TYPE_NAME */
        {
            RT_CHECK("will.topic.alt.string.len",
                     out.will.topic.alt.string.len == in->will.topic.alt.string.len);
            RT_CHECK("will.topic.alt.string.data",
                     memcmp(out.will.topic.alt.string.data,
                            in->will.topic.alt.string.data,
                            in->will.topic.alt.string.len) == 0);
        }
        RT_CHECK("will.payload.len",  out.will.payload.len == in->will.payload.len);
        RT_CHECK("will.payload.data",
                 memcmp(out.will.payload.data,
                        in->will.payload.data,
                        in->will.payload.len) == 0);
    }

    if (in->flags.bits.defaultAwakeMessages)
        RT_CHECK("defaultAwakeMessages",
                 out.defaultAwakeMessages == in->defaultAwakeMessages);

    if (in->flags.bits.sessionExpiryInterval)
        RT_CHECK("sessionExpiryInterval",
                 out.sessionExpiryInterval == in->sessionExpiryInterval);

    if (in->flags.bits.auth)
    {
        RT_CHECK("auth.method.len",
                 out.auth.method.len == in->auth.method.len);
        RT_CHECK("auth.method.data",
                 memcmp(out.auth.method.data,
                        in->auth.method.data,
                        in->auth.method.len) == 0);
        RT_CHECK("auth.data.len",
                 out.auth.data.len == in->auth.data.len);
        RT_CHECK("auth.data.data",
                 memcmp(out.auth.data.data,
                        in->auth.data.data,
                        in->auth.data.len) == 0);
    }

    RT_CHECK("clientID.len",  out.clientID.len == in->clientID.len);
    if (in->clientID.len > 0)
        RT_CHECK("clientID.data",
                 memcmp(out.clientID.data,
                        in->clientID.data,
                        in->clientID.len) == 0);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}


/*
 * Serialize with MQTTSNSerialize_connack, deserialize with
 * MQTTSNDeserialize_connack, then check every field.
 */
static int connack_roundtrip(const char* test_name,
        const MQTTSNPacket_connackData* in)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    MQTTSNPacket_connackData out;
    int32_t  drc;
    int      ok = 1;

    printf("%-58s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out, 0, sizeof(out));

    slen = MQTTSNSerialize_connack(buf, (int32_t)sizeof(buf),
                                   (MQTTSNPacket_connackData*)in);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_connack(&out, buf, slen);
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

    RT_CHECK("flags.all",   out.flags.all   == in->flags.all);
    RT_CHECK("packetId",    out.packetId     == in->packetId);
    RT_CHECK("reasonCode",  out.reasonCode   == in->reasonCode);

    if (in->flags.bits.sessionExpiryInterval)
        RT_CHECK("sessionExpiryInterval",
                 out.sessionExpiryInterval == in->sessionExpiryInterval);

    if (in->flags.bits.serverKeepAlive)
        RT_CHECK("serverKeepAlive",
                 out.serverKeepAlive == in->serverKeepAlive);

    if (in->flags.bits.auth)
    {
        RT_CHECK("auth.method.len",
                 out.auth.method.len == in->auth.method.len);
        RT_CHECK("auth.method.data",
                 memcmp(out.auth.method.data,
                        in->auth.method.data,
                        in->auth.method.len) == 0);
        RT_CHECK("auth.data.len",
                 out.auth.data.len == in->auth.data.len);
        RT_CHECK("auth.data.data",
                 memcmp(out.auth.data.data,
                        in->auth.data.data,
                        in->auth.data.len) == 0);
    }

    RT_CHECK("assignedClientID.len",
             out.assignedClientID.len == in->assignedClientID.len);
    if (in->assignedClientID.len > 0)
        RT_CHECK("assignedClientID.data",
                 memcmp(out.assignedClientID.data,
                        in->assignedClientID.data,
                        in->assignedClientID.len) == 0);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}


/* =========================================================================
 * CONNECT test cases
 * ========================================================================= */

static void test_connect_minimal(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.packetId     = 1;
    in.keepAlive    = 60;
    in.maxPacketSize= 256;
    in.clientID.len = 8;
    in.clientID.data= "test-cli";
    connect_roundtrip("CONNECT minimal", &in);
}

static void test_connect_clean_start(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.cleanStart = 1;
    in.packetId      = 2;
    in.keepAlive     = 30;
    in.maxPacketSize = 512;
    in.clientID.len  = 2;
    in.clientID.data = "ab";
    connect_roundtrip("CONNECT cleanStart=1", &in);
}

static void test_connect_empty_clientid(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.packetId      = 3;
    in.keepAlive     = 0;   /* keepAlive=0: no keepalive timeout */
    in.maxPacketSize = 100;
    in.clientID.len  = 0;
    in.clientID.data = NULL;
    connect_roundtrip("CONNECT empty clientId", &in);
}

static void test_connect_packetid_max(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.packetId      = 0xFFFF;
    in.keepAlive     = 0xFFFF;
    in.maxPacketSize = 0xFFFF;
    in.clientID.len  = 1;
    in.clientID.data = "X";
    connect_roundtrip("CONNECT packetId/keepAlive/maxPacketSize 0xFFFF", &in);
}

static void test_connect_default_awake(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.defaultAwakeMessages = 1;
    in.packetId             = 4;
    in.keepAlive            = 60;
    in.maxPacketSize        = 256;
    in.defaultAwakeMessages = 10;
    in.clientID.len         = 6;
    in.clientID.data        = "sleepy";
    connect_roundtrip("CONNECT defaultAwakeMessages=10", &in);
}

static void test_connect_session_expiry(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.sessionExpiryInterval = 1;
    in.packetId              = 5;
    in.keepAlive             = 60;
    in.maxPacketSize         = 256;
    in.sessionExpiryInterval = 3600;
    in.clientID.len          = 6;
    in.clientID.data         = "sesnex";
    connect_roundtrip("CONNECT sessionExpiryInterval=3600", &in);
}

static void test_connect_session_expiry_max(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.sessionExpiryInterval = 1;
    in.packetId              = 6;
    in.keepAlive             = 60;
    in.maxPacketSize         = 256;
    in.sessionExpiryInterval = 0xFFFFFFFF;  /* session never expires */
    in.clientID.len          = 3;
    in.clientID.data         = "max";
    connect_roundtrip("CONNECT sessionExpiryInterval=0xFFFFFFFF (never)", &in);
}

static void test_connect_will_session_alias(void)
{
    static const uint8_t will_payload[] = "device/offline";
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.will              = 1;
    in.willFlags.bits.topicType     = MQTTSN_TOPIC_TYPE_SESSION;
    in.willFlags.bits.QoS           = 1;
    in.willFlags.bits.retain        = 0;
    in.packetId                     = 7;
    in.keepAlive                    = 60;
    in.maxPacketSize                = 256;
    in.will.topic.type              = MQTTSN_TOPIC_TYPE_SESSION;
    in.will.topic.alt.alias         = 0x0042;
    in.will.payload.len             = (uint16_t)sizeof(will_payload) - 1;
    in.will.payload.data            = (unsigned char*)will_payload;
    in.clientID.len                 = 7;
    in.clientID.data                = "will-s1";
    connect_roundtrip("CONNECT will / session alias 0x0042", &in);
}

static void test_connect_will_predefined_alias(void)
{
    static const uint8_t will_payload[] = "\x00\x01";
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.will              = 1;
    in.willFlags.bits.topicType     = MQTTSN_TOPIC_TYPE_PREDEFINED;
    in.willFlags.bits.QoS           = 2;
    in.willFlags.bits.retain        = 1;
    in.packetId                     = 8;
    in.keepAlive                    = 60;
    in.maxPacketSize                = 256;
    in.will.topic.type              = MQTTSN_TOPIC_TYPE_PREDEFINED;
    in.will.topic.alt.alias         = 0xABCD;
    in.will.payload.len             = 2;
    in.will.payload.data            = (unsigned char*)will_payload;
    in.clientID.len                 = 7;
    in.clientID.data                = "will-p1";
    connect_roundtrip("CONNECT will / predefined alias 0xABCD QoS2 retain", &in);
}

static void test_connect_will_topic_name(void)
{
    static const uint8_t will_payload[] = "offline";
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.will                  = 1;
    in.willFlags.bits.topicType         = MQTTSN_TOPIC_TYPE_NAME;
    in.willFlags.bits.QoS               = 1;
    in.willFlags.bits.retain            = 1;
    in.packetId                         = 9;
    in.keepAlive                        = 60;
    in.maxPacketSize                    = 512;
    in.will.topic.type                  = MQTTSN_TOPIC_TYPE_NAME;
    in.will.topic.alt.string.len        = 16;
    in.will.topic.alt.string.data       = "devices/dev1/lwt";
    in.will.payload.len                 = 7;
    in.will.payload.data                = (unsigned char*)will_payload;
    in.clientID.len                     = 4;
    in.clientID.data                    = "dev1";
    connect_roundtrip("CONNECT will / topic name QoS1 retain", &in);
}

static void test_connect_auth(void)
{
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.auth          = 1;
    in.packetId                 = 10;
    in.keepAlive                = 60;
    in.maxPacketSize            = 256;
    in.auth.method.len          = 8;
    in.auth.method.data         = "password";
    in.auth.data.len            = 6;
    in.auth.data.data           = (unsigned char*)"secret";
    in.clientID.len             = 8;
    in.clientID.data            = "auth-cli";
    connect_roundtrip("CONNECT auth (method=\"password\" data=\"secret\")", &in);
}

static void test_connect_all_flags(void)
{
    static const uint8_t will_payload[] = "gone";
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.flags.bits.cleanStart            = 1;
    in.flags.bits.will                  = 1;
    in.flags.bits.auth                  = 1;
    in.flags.bits.sessionExpiryInterval = 1;
    in.flags.bits.defaultAwakeMessages  = 1;
    in.willFlags.bits.topicType         = MQTTSN_TOPIC_TYPE_NAME;
    in.willFlags.bits.QoS               = 2;
    in.willFlags.bits.retain            = 1;
    in.packetId                         = 0x1234;
    in.keepAlive                        = 120;
    in.maxPacketSize                    = 1024;
    in.defaultAwakeMessages             = 5;
    in.sessionExpiryInterval            = 86400;
    in.will.topic.type                  = MQTTSN_TOPIC_TYPE_NAME;
    in.will.topic.alt.string.len        = 8;
    in.will.topic.alt.string.data       = "lwt/full";
    in.will.payload.len                 = 4;
    in.will.payload.data                = (unsigned char*)will_payload;
    in.auth.method.len                  = 5;
    in.auth.method.data                 = "plain";
    in.auth.data.len                    = 8;
    in.auth.data.data                   = (unsigned char*)"userpass";
    in.clientID.len                     = 8;
    in.clientID.data                    = "full-cli";
    connect_roundtrip("CONNECT all flags set", &in);
}

static void test_connect_buffer_too_short(void)
{
    uint8_t buf[4]; /* too small for any valid CONNECT */
    MQTTSNPacket_connectData in = MQTTSNPacket_connectData_initializer;
    in.clientID.len  = 6;
    in.clientID.data = "client";

    printf("%-58s ", "CONNECT buffer too short");
    ++tests_run;
    if (MQTTSNSerialize_connect(buf, (int32_t)sizeof(buf), &in)
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
 * CONNACK test cases
 * ========================================================================= */

static void test_connack_minimal_success(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.packetId   = 1;
    in.reasonCode = MQTT_SN_RC_SUCCESS;
    connack_roundtrip("CONNACK minimal success", &in);
}

static void test_connack_reason_code(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.packetId   = 2;
    in.reasonCode = MQTT_SN_RC_NOT_AUTHORIZED;
    connack_roundtrip("CONNACK reason code 0x87 (not authorized)", &in);
}

static void test_connack_session_present(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.flags.bits.sessionPresent = 1;
    in.packetId                  = 3;
    in.reasonCode                = MQTT_SN_RC_SUCCESS;
    connack_roundtrip("CONNACK sessionPresent=1", &in);
}

static void test_connack_session_expiry(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.flags.bits.sessionExpiryInterval = 1;
    in.packetId                         = 4;
    in.reasonCode                       = MQTT_SN_RC_SUCCESS;
    in.sessionExpiryInterval            = 7200;
    connack_roundtrip("CONNACK sessionExpiryInterval=7200", &in);
}

static void test_connack_server_keep_alive(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.flags.bits.serverKeepAlive = 1;
    in.packetId                   = 5;
    in.reasonCode                 = MQTT_SN_RC_SUCCESS;
    in.serverKeepAlive            = 120;
    connack_roundtrip("CONNACK serverKeepAlive=120", &in);
}

static void test_connack_auth(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.flags.bits.auth     = 1;
    in.packetId            = 6;
    in.reasonCode          = MQTT_SN_RC_CONTINUE_AUTH;
    in.auth.method.islen8  = 1;
    in.auth.method.len     = 8;
    in.auth.method.data    = "password";
    in.auth.data.len       = 9;
    in.auth.data.data      = (unsigned char*)"challenge";
    connack_roundtrip("CONNACK auth (continue authentication)", &in);
}

static void test_connack_assigned_clientid(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.packetId                = 7;
    in.reasonCode              = MQTT_SN_RC_SUCCESS;
    in.assignedClientID.len    = 36;
    in.assignedClientID.data   = "550e8400-e29b-41d4-a716-446655440000";
    connack_roundtrip("CONNACK assignedClientID (UUID)", &in);
}

static void test_connack_all_optional_fields(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.flags.bits.sessionPresent        = 1;
    in.flags.bits.sessionExpiryInterval = 1;
    in.flags.bits.serverKeepAlive       = 1;
    in.packetId                         = 0x5678;
    in.reasonCode                       = MQTT_SN_RC_SUCCESS;
    in.sessionExpiryInterval            = 3600;
    in.serverKeepAlive                  = 90;
    in.assignedClientID.len             = 8;
    in.assignedClientID.data            = "srv-asgn";
    connack_roundtrip("CONNACK all optional fields (no auth)", &in);
}

static void test_connack_all_flags_with_auth(void)
{
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.flags.bits.sessionPresent        = 1;
    in.flags.bits.sessionExpiryInterval = 1;
    in.flags.bits.serverKeepAlive       = 1;
    in.flags.bits.auth                  = 1;
    in.packetId                         = 0x9ABC;
    in.reasonCode                       = MQTT_SN_RC_SUCCESS;
    in.sessionExpiryInterval            = 0xFFFFFFFF;
    in.serverKeepAlive                  = 300;
    in.auth.method.islen8               = 1;
    in.auth.method.len                  = 4;
    in.auth.method.data                 = "SCRAM";
    in.auth.data.len                    = 5;
    in.auth.data.data                   = (unsigned char*)"token";
    in.assignedClientID.len             = 6;
    in.assignedClientID.data            = "newcli";
    connack_roundtrip("CONNACK all flags + auth + assignedClientID", &in);
}

static void test_connack_buffer_too_short(void)
{
    uint8_t buf[2];
    MQTTSNPacket_connackData in;
    memset(&in, 0, sizeof(in));
    in.packetId   = 1;
    in.reasonCode = MQTT_SN_RC_SUCCESS;

    printf("%-58s ", "CONNACK buffer too short");
    ++tests_run;
    if (MQTTSNSerialize_connack(buf, (int32_t)sizeof(buf), &in)
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
 * DISCONNECT test cases
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_disconnect, deserialize with
 * MQTTSNDeserialize_disconnect, then check the reason code and wire length.
 */
static int disconnect_roundtrip(const char* test_name,
        uint8_t reason_code, int32_t expected_wire_len)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    MQTTSNPacket_disconnectData out;
    int32_t  drc;
    int      ok = 1;

    printf("%-58s ", test_name);

    memset(buf, 0, sizeof(buf));
    memset(&out, 0, sizeof(out));

    slen = MQTTSNSerialize_disconnect(buf, (int32_t)sizeof(buf), reason_code);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_disconnect(&out, buf, slen);
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

    RT_CHECK("reasonCode",      out.reasonCode  == reason_code);
    RT_CHECK("wire length",     slen            == expected_wire_len);

    /* success: reason code flag should be clear and reason code absent from wire */
    if (reason_code == MQTT_SN_RC_SUCCESS)
        RT_CHECK("reasonCode flag clear",
                 out.flags.bits.reasonCode == 0);
    else
        RT_CHECK("reasonCode flag set",
                 out.flags.bits.reasonCode == 1);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}

static void test_disconnect_success(void)
{
    /* success: length(1)+type(1)+flags(1) = 3 bytes; reason code omitted */
    disconnect_roundtrip("DISCONNECT success (reason code absent from wire)",
                         MQTT_SN_RC_SUCCESS, 3);
}

static void test_disconnect_server_busy(void)
{
    /* with reason code: length(1)+type(1)+flags(1)+rc(1) = 4 bytes */
    disconnect_roundtrip("DISCONNECT reason 0x89 (server busy)",
                         MQTT_SN_RC_SERVER_BUSY, 4);
}

static void test_disconnect_session_taken_over(void)
{
    disconnect_roundtrip("DISCONNECT reason 0x8E (session taken over)",
                         MQTT_SN_RC_SESSION_TAKEN_OVER, 4);
}

static void test_disconnect_no_virtual_connection(void)
{
    disconnect_roundtrip("DISCONNECT reason 0xF4 (no virtual connection)",
                         MQTT_SN_RC_NO_VIRTUAL_CONNECTION, 4);
}

static void test_disconnect_buffer_too_short(void)
{
    uint8_t buf[1];  /* too small for any valid DISCONNECT */
    printf("%-58s ", "DISCONNECT buffer too short");
    ++tests_run;
    if (MQTTSNSerialize_disconnect(buf, (int32_t)sizeof(buf), MQTT_SN_RC_SUCCESS)
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
 * PINGREQ test cases
 *
 * In MQTT-SN 2.0 PINGREQ carries a mandatory Packet Identifier (§3.11.2).
 * The Client Identifier field present in v1.2 is gone.
 * Wire format: length(1) + type(1) + packetid(2) — always 4 bytes.
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_pingreq (client), deserialize with
 * MQTTSNDeserialize_pingreq (server), then check the Packet Identifier
 * and the wire length.
 */
static int pingreq_roundtrip(const char* test_name, uint16_t packetid)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    uint16_t out_packetid = 0;
    int32_t  drc;
    int      ok = 1;

    printf("%-58s ", test_name);

    memset(buf, 0, sizeof(buf));

    slen = MQTTSNSerialize_pingreq(buf, (int32_t)sizeof(buf), packetid);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_pingreq(&out_packetid, buf, slen);
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

    RT_CHECK("packetid round-trips",  out_packetid == packetid);
    RT_CHECK("wire length is 4",      slen == 4);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}

static void test_pingreq_typical(void)
{
    pingreq_roundtrip("PINGREQ typical packetid 0x0042", 0x0042);
}

static void test_pingreq_packetid_min(void)
{
    pingreq_roundtrip("PINGREQ packetid 1 (minimum)", 1);
}

static void test_pingreq_packetid_max(void)
{
    pingreq_roundtrip("PINGREQ packetid 0xFFFF (maximum)", 0xFFFF);
}

static void test_pingreq_buffer_too_short(void)
{
    uint8_t buf[3];  /* 3 bytes: too small for length(1)+type(1)+packetid(2) */
    printf("%-58s ", "PINGREQ buffer too short");
    ++tests_run;
    if (MQTTSNSerialize_pingreq(buf, (int32_t)sizeof(buf), 1)
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
 * PINGRESP test cases
 *
 * In MQTT-SN 2.0 PINGRESP carries a mandatory Packet Identifier (§3.12.2)
 * that echoes the one from PINGREQ, and an optional Application Messages
 * Remaining byte (§3.12.3) whose presence is inferred from packet length.
 *
 * Wire lengths:
 *   Without AMR: length(1) + type(1) + packetid(2) = 4 bytes
 *   With AMR:    length(1) + type(1) + packetid(2) + amr(1) = 5 bytes
 * ========================================================================= */

/*
 * Serialize with MQTTSNSerialize_pingresp (server), deserialize with
 * MQTTSNDeserialize_pingresp (client), then check every field and the wire
 * length.  messages_remaining == -1 means the AMR field is omitted.
 */
static int pingresp_roundtrip(const char* test_name,
        uint16_t packetid, int messages_remaining, int32_t expected_wire_len)
{
    uint8_t  buf[BUF_SIZE];
    int32_t  slen;
    uint16_t out_packetid          = 0;
    int      out_messages_remaining = 0;
    int32_t  drc;
    int      ok = 1;

    printf("%-58s ", test_name);

    memset(buf, 0, sizeof(buf));

    slen = MQTTSNSerialize_pingresp(buf, (int32_t)sizeof(buf),
                                     packetid, messages_remaining);
    if (slen <= 0) {
        printf("FAIL  (serialize returned %d)\n", (int)slen);
        ++tests_run; ++tests_failed;
        return 0;
    }

    drc = MQTTSNDeserialize_pingresp(&out_packetid, &out_messages_remaining,
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

    RT_CHECK("packetid round-trips",         out_packetid == packetid);
    RT_CHECK("messages_remaining round-trips",
             out_messages_remaining == messages_remaining);
    RT_CHECK("wire length",                  slen == expected_wire_len);

#undef RT_CHECK

    if (ok)
        printf("ok\n");
    else
        printf("\n");

    return ok;
}

static void test_pingresp_no_amr(void)
{
    /* AMR absent: wire length = 4 bytes; deserializer returns -1 for AMR */
    pingresp_roundtrip("PINGRESP no AMR field (wire length 4)",
                       0x0042, -1, 4);
}

static void test_pingresp_amr_zero(void)
{
    /* AMR=0 means "no messages queued"; field IS present (wire length 5) */
    pingresp_roundtrip("PINGRESP AMR=0 (no messages queued, wire length 5)",
                       0x0042, 0, 5);
}

static void test_pingresp_amr_typical(void)
{
    pingresp_roundtrip("PINGRESP AMR=3 (three messages queued)",
                       0x0042, 3, 5);
}

static void test_pingresp_amr_max(void)
{
    /* 0xFF means "an unspecified positive number of messages" (§3.12.3) */
    pingresp_roundtrip("PINGRESP AMR=0xFF (unspecified positive count)",
                       0x0042, 0xFF, 5);
}

static void test_pingresp_packetid_echoed(void)
{
    /* Verify that an arbitrary packetid survives the round-trip */
    pingresp_roundtrip("PINGRESP packetid 0x1234 echoed correctly",
                       0x1234, -1, 4);
}

static void test_pingresp_buffer_too_short(void)
{
    uint8_t buf[3];  /* 3 bytes: too small for length(1)+type(1)+packetid(2) */
    printf("%-58s ", "PINGRESP buffer too short");
    ++tests_run;
    if (MQTTSNSerialize_pingresp(buf, (int32_t)sizeof(buf), 1, -1)
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
    printf("=== MQTT-SN 2.0 connect/server round-trip tests ===\n");

    section("CONNECT — minimal and fixed fields");
    test_connect_minimal();
    test_connect_clean_start();
    test_connect_empty_clientid();
    test_connect_packetid_max();

    section("CONNECT — optional single-field flags");
    test_connect_default_awake();
    test_connect_session_expiry();
    test_connect_session_expiry_max();

    section("CONNECT — will (all three topic types)");
    test_connect_will_session_alias();
    test_connect_will_predefined_alias();
    test_connect_will_topic_name();

    section("CONNECT — auth block");
    test_connect_auth();

    section("CONNECT — all flags simultaneously");
    test_connect_all_flags();

    section("CONNECT — error handling");
    test_connect_buffer_too_short();

    section("CONNACK — reason codes and fixed fields");
    test_connack_minimal_success();
    test_connack_reason_code();
    test_connack_session_present();

    section("CONNACK — individual optional fields");
    test_connack_session_expiry();
    test_connack_server_keep_alive();
    test_connack_auth();
    test_connack_assigned_clientid();

    section("CONNACK — all optional fields simultaneously");
    test_connack_all_optional_fields();
    test_connack_all_flags_with_auth();

    section("CONNACK — error handling");
    test_connack_buffer_too_short();

    section("DISCONNECT");
    test_disconnect_success();
    test_disconnect_server_busy();
    test_disconnect_session_taken_over();
    test_disconnect_no_virtual_connection();
    test_disconnect_buffer_too_short();

    section("PINGREQ");
    test_pingreq_typical();
    test_pingreq_packetid_min();
    test_pingreq_packetid_max();
    test_pingreq_buffer_too_short();

    section("PINGRESP");
    test_pingresp_no_amr();
    test_pingresp_amr_zero();
    test_pingresp_amr_typical();
    test_pingresp_amr_max();
    test_pingresp_packetid_echoed();
    test_pingresp_buffer_too_short();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}
