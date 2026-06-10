/* Enable POSIX extensions (timersub, localtime_r) */
#if !defined(_GNU_SOURCE)
  #define _GNU_SOURCE
#endif

/*******************************************************************************
 * Copyright (c) 2026 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation for MQTT-SN 2.0
 *******************************************************************************/

/**
 * @file
 * Tests for MQTTSNSerializePublish.c and MQTTSNDeserializePublish.c.
 *
 * Exercises every combination of PUBLISH options defined by the
 * MQTT-SN 2.0 Committee Specification Draft 01 (October 2025):
 *   - QoS 0, 1, 2
 *   - DUP flag (QoS 1 and 2 only)
 *   - Retain flag
 *   - Topic types: Session alias, Predefined alias, Topic Name
 *   - Zero-length payload
 *   - PUBACK / PUBREC / PUBREL / PUBCOMP with and without reason codes
 *
 * Build (example, adjust include/library paths as needed):
 *   gcc -Wall -Wextra -o test_mqttsn_publish test_mqttsn_publish.c \
 *       MQTTSNSerializePublish.c MQTTSNDeserializePublish.c     \
 *       MQTTSNPacket.c StackTrace.c                              \
 *       -I. -o test_mqttsn_publish
 */

#include "MQTTSNPublish.h"
#include "MQTTSNPacket.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(_WIN32)
  #include <sys/time.h>
#else
  #include <windows.h>
#endif

/* ---------------------------------------------------------------------------
 * Test framework (modelled on paho.mqtt.c/test/test1.c)
 * ---------------------------------------------------------------------------*/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define LOGA_DEBUG 0
#define LOGA_INFO  1

struct Options
{
    int verbose;
    int test_no;
} options =
{
    0,  /* verbose */
    0,  /* test_no: 0 = run all */
};

void usage(void)
{
    printf("Options:\n");
    printf("  --test_no <n>   run only test n (default: run all)\n");
    printf("  --verbose       enable debug logging\n");
    exit(EXIT_FAILURE);
}

void getopts(int argc, char** argv)
{
    int i = 1;
    while (i < argc)
    {
        if (strcmp(argv[i], "--test_no") == 0)
        {
            if (++i < argc)
                options.test_no = atoi(argv[i]);
            else
                usage();
        }
        else if (strcmp(argv[i], "--verbose") == 0)
        {
            options.verbose = 1;
            printf("Verbose mode on\n");
        }
        else
            usage();
        ++i;
    }
}

/* Timestamped log ---------------------------------------------------------*/
#if defined(_WIN32)
#define START_TIME_TYPE DWORD
static DWORD start_time = 0;
START_TIME_TYPE start_clock(void) { return GetTickCount(); }
long elapsed(START_TIME_TYPE t)   { return (long)(GetTickCount() - t); }
#else
#define START_TIME_TYPE struct timeval
START_TIME_TYPE start_clock(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t;
}
long elapsed(START_TIME_TYPE start)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&now, &start, &res);
    return res.tv_sec * 1000L + res.tv_usec / 1000L;
}
#endif

void MyLog(int level, const char* fmt, ...)
{
    static char buf[512];
    va_list args;
#if defined(_WIN32)
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buf, sizeof(buf), "%04d%02d%02d %02d%02d%02d.%03d ",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    struct timeval tv;
    struct tm tm_info;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm_info);
    strftime(buf, 80, "%Y%m%d %H%M%S", &tm_info);
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
             ".%03lu ", (unsigned long)(tv.tv_usec / 1000UL));
#endif
    if (level == LOGA_DEBUG && !options.verbose)
        return;
    va_start(args, fmt);
    vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, args);
    va_end(args);
    printf("%s\n", buf);
    fflush(stdout);
}

/* Assertion machinery -----------------------------------------------------*/
int tests    = 0;
int failures = 0;
FILE*          xml;
START_TIME_TYPE global_start_time;
char            output[3000];
char*           cur_output = output;

void write_test_result(void)
{
    long dur = elapsed(global_start_time);
    fprintf(xml, " time=\"%ld.%03ld\" >\n", dur / 1000L, dur % 1000L);
    if (cur_output != output)
    {
        fprintf(xml, "%s", output);
        cur_output = output;
    }
    fprintf(xml, "</testcase>\n");
}

void myassert(const char* file, int line,
              const char* description, int value,
              const char* fmt, ...)
{
    ++tests;
    if (!value)
    {
        va_list args;
        ++failures;
        MyLog(LOGA_INFO, "Assertion FAILED  %s:%d  %s", file, line, description);
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        cur_output += sprintf(cur_output,
            "<failure type=\"%s\">file %s, line %d</failure>\n",
            description, file, line);
    }
    else
        MyLog(LOGA_DEBUG, "Assertion ok      %s:%d  %s", file, line, description);
}

#define assert(desc, val, fmt, ...)  \
    myassert(__FILE__, __LINE__, desc, (int)(val), fmt, ##__VA_ARGS__)


/* ---------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------------*/

/* Buffer large enough for any test packet */
#define BUF_SIZE 512

/**
 * Fill a topic struct for SESSION or PREDEFINED topic type.
 */
static void make_alias_topic(MQTTSN_topic* t, uint8_t type, uint16_t alias)
{
    t->type      = type;
    t->alt.alias = alias;
}

/**
 * Fill a topic struct for TOPIC_TYPE_NAME.
 * @p name must remain valid for the lifetime of the topic struct.
 */
static void make_name_topic(MQTTSN_topic* t,
                             const char* name, uint16_t len)
{
    t->type              = MQTTSN_TOPIC_TYPE_NAME;
    t->alt.string.data   = (char*)name;
    t->alt.string.len    = len;
}

/**
 * Return non-zero if two MQTTSN_topic values carry the same content.
 */
static int topics_equal(const MQTTSN_topic* a, const MQTTSN_topic* b)
{
    if (a->type != b->type)
        return 0;
    if (a->type == MQTTSN_TOPIC_TYPE_NAME)
        return a->alt.string.len == b->alt.string.len &&
               memcmp(a->alt.string.data,
                      b->alt.string.data,
                      a->alt.string.len) == 0;
    return a->alt.alias == b->alt.alias;
}


/* ---------------------------------------------------------------------------
 * Test 1 – QoS 0, Session Topic Alias
 * ---------------------------------------------------------------------------*/
int test1(struct Options opts)
{
    (void)opts;
    const char* testname = "test1";
    uint8_t  buf[BUF_SIZE];
    uint8_t  rbuf[BUF_SIZE];
    int32_t  buflen = BUF_SIZE;

    uint8_t  dup_in    = 0;
    int32_t  qos_in    = 0;
    uint8_t  ret_in    = 0;
    uint16_t pktid_in  = 0;   /* no packet-id for QoS 0 */
    MQTTSN_topic topic_in;
    const char* payload_in   = "hello world";
    int32_t     payloadlen_in = (int32_t)strlen(payload_in);

    uint8_t  dup_out;
    int32_t  qos_out;
    uint8_t  ret_out;
    uint16_t pktid_out;
    MQTTSN_topic topic_out;
    uint8_t* payload_out;
    int32_t  payloadlen_out;
    int32_t  len;
    int      rc;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"QoS 0 Session Topic Alias\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - QoS 0, Session Topic Alias", testname);

    make_alias_topic(&topic_in, MQTTSN_TOPIC_TYPE_SESSION, 42u);

    len = MQTTSNSerialize_publish(buf, buflen, dup_in, qos_in, ret_in,
                                  pktid_in, &topic_in,
                                  (uint8_t*)payload_in, payloadlen_in);
    assert("Serialize returns positive length", len > 0,
           "len was %d\n", (int)len);

    memcpy(rbuf, buf, (size_t)len);
    rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                   &topic_out, &payload_out, &payloadlen_out,
                                   rbuf, len);
    assert("Deserialize returns 1", rc == 1, "rc was %d\n", rc);
    assert("QoS round-trips",    qos_out == qos_in,    "got %d\n", qos_out);
    assert("DUP round-trips",    dup_out == dup_in,    "got %d\n", dup_out);
    assert("Retain round-trips", ret_out == ret_in,    "got %d\n", ret_out);
    assert("PacketId is 0 for QoS 0", pktid_out == 0,  "got %u\n", pktid_out);
    assert("Topic type round-trips",  topics_equal(&topic_in, &topic_out),
           "type %d alias %u vs type %d alias %u\n",
           topic_in.type,  topic_in.alt.alias,
           topic_out.type, topic_out.alt.alias);
    assert("Payload length round-trips",
           payloadlen_out == payloadlen_in, "got %d\n", payloadlen_out);
    assert("Payload data round-trips",
           memcmp(payload_out, payload_in, (size_t)payloadlen_in) == 0,
           "payload mismatch\n");

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 2 – QoS 0, Predefined Topic Alias, Retain set
 * ---------------------------------------------------------------------------*/
int test2(struct Options opts)
{
    (void)opts;
    const char* testname = "test2";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;

    uint8_t  dup_in = 0, ret_in = 1;
    int32_t  qos_in = 0;
    uint16_t pktid_in = 0;
    MQTTSN_topic topic_in;
    const uint8_t payload_in[] = { 0x01, 0x02, 0x03, 0x04 };

    uint8_t  dup_out, ret_out;
    int32_t  qos_out;
    uint16_t pktid_out;
    MQTTSN_topic topic_out;
    uint8_t* payload_out;
    int32_t  payloadlen_out;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"QoS 0 Predefined Topic Alias Retain\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - QoS 0, Predefined Topic Alias, Retain", testname);

    make_alias_topic(&topic_in, MQTTSN_TOPIC_TYPE_PREDEFINED, 7u);

    len = MQTTSNSerialize_publish(buf, BUF_SIZE, dup_in, qos_in, ret_in,
                                  pktid_in, &topic_in,
                                  (uint8_t*)payload_in, (int32_t)sizeof(payload_in));
    assert("Serialize returns positive length", len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                   &topic_out, &payload_out, &payloadlen_out,
                                   buf, len);
    assert("Deserialize returns 1",         rc == 1,               "rc was %d\n", rc);
    assert("QoS round-trips",              qos_out == qos_in,      "got %d\n", qos_out);
    assert("Retain round-trips",           ret_out == ret_in,      "got %d\n", ret_out);
    assert("Predefined alias round-trips", topics_equal(&topic_in, &topic_out),
           "alias %u vs %u\n", topic_in.alt.alias, topic_out.alt.alias);
    assert("Payload length round-trips",
           payloadlen_out == (int32_t)sizeof(payload_in),
           "got %d\n", payloadlen_out);
    assert("Payload data round-trips",
           memcmp(payload_out, payload_in, sizeof(payload_in)) == 0,
           "payload mismatch\n");

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 3 – QoS 0, Topic Name
 * ---------------------------------------------------------------------------*/
int test3(struct Options opts)
{
    (void)opts;
    const char* testname = "test3";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;

    uint8_t  dup_in = 0, ret_in = 0;
    int32_t  qos_in = 0;
    uint16_t pktid_in = 0;
    const char*  topic_name = "sensors/temperature/living-room";
    MQTTSN_topic topic_in;
    const char*  payload_in    = "23.5";
    int32_t      payloadlen_in = (int32_t)strlen(payload_in);

    uint8_t  dup_out, ret_out;
    int32_t  qos_out;
    uint16_t pktid_out;
    MQTTSN_topic topic_out;
    uint8_t* payload_out;
    int32_t  payloadlen_out;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"QoS 0 Topic Name\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - QoS 0, Topic Name", testname);

    make_name_topic(&topic_in, topic_name, (uint16_t)strlen(topic_name));

    len = MQTTSNSerialize_publish(buf, BUF_SIZE, dup_in, qos_in, ret_in,
                                  pktid_in, &topic_in,
                                  (uint8_t*)payload_in, payloadlen_in);
    assert("Serialize returns positive length", len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                   &topic_out, &payload_out, &payloadlen_out,
                                   buf, len);
    assert("Deserialize returns 1",         rc == 1,     "rc was %d\n", rc);
    assert("Topic type is NAME",
           topic_out.type == MQTTSN_TOPIC_TYPE_NAME,
           "got type %d\n", topic_out.type);
    assert("Topic name round-trips", topics_equal(&topic_in, &topic_out),
           "len %u vs %u\n",
           (unsigned)topic_in.alt.string.len,
           (unsigned)topic_out.alt.string.len);
    assert("PacketId is 0 for QoS 0", pktid_out == 0, "got %u\n", pktid_out);
    assert("Payload length round-trips",
           payloadlen_out == payloadlen_in, "got %d\n", payloadlen_out);
    assert("Payload data round-trips",
           memcmp(payload_out, payload_in, (size_t)payloadlen_in) == 0,
           "payload mismatch\n");

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 4 – QoS 1, Session Topic Alias, PUBACK (no reason code)
 * ---------------------------------------------------------------------------*/
int test4(struct Options opts)
{
    (void)opts;
    const char* testname = "test4";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;

    uint8_t  dup_in = 0, ret_in = 0;
    int32_t  qos_in    = 1;
    uint16_t pktid_in  = 1234u;
    MQTTSN_topic topic_in;
    const char* payload_in    = "qos1-message";
    int32_t     payloadlen_in = (int32_t)strlen(payload_in);

    /* PUBACK fields */
    uint16_t puback_pktid_in  = pktid_in;
    uint8_t  puback_rc_in     = MQTT_SN_RC_SUCCESS;

    uint8_t  dup_out, ret_out;
    int32_t  qos_out;
    uint16_t pktid_out;
    MQTTSN_topic topic_out;
    uint8_t* payload_out;
    int32_t  payloadlen_out;
    uint16_t puback_pktid_out;
    uint8_t  puback_rc_out;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"QoS 1 Session Alias PUBACK no reason code\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - QoS 1, Session Alias, PUBACK", testname);

    make_alias_topic(&topic_in, MQTTSN_TOPIC_TYPE_SESSION, 10u);

    /* --- PUBLISH --- */
    len = MQTTSNSerialize_publish(buf, BUF_SIZE, dup_in, qos_in, ret_in,
                                  pktid_in, &topic_in,
                                  (uint8_t*)payload_in, payloadlen_in);
    assert("Serialize publish returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                   &topic_out, &payload_out, &payloadlen_out,
                                   buf, len);
    assert("Deserialize publish returns 1",   rc == 1,   "rc was %d\n", rc);
    assert("QoS 1 round-trips",          qos_out == 1,         "got %d\n", qos_out);
    assert("DUP 0 round-trips",          dup_out == 0,         "got %d\n", dup_out);
    assert("PacketId round-trips",       pktid_out == pktid_in,"got %u\n", pktid_out);
    assert("Topic alias round-trips",    topics_equal(&topic_in, &topic_out),
           "alias %u vs %u\n",
           topic_in.alt.alias, topic_out.alt.alias);
    assert("Payload length round-trips",
           payloadlen_out == payloadlen_in, "got %d\n", payloadlen_out);
    assert("Payload data round-trips",
           memcmp(payload_out, payload_in, (size_t)payloadlen_in) == 0,
           "payload mismatch\n");

    /* --- PUBACK (success, no reason code on wire) --- */
    len = MQTTSNSerialize_puback(buf, BUF_SIZE, puback_pktid_in, puback_rc_in);
    assert("Serialize puback returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_puback(&puback_pktid_out, &puback_rc_out, buf, len);
    assert("Deserialize puback returns 1",       rc == 1, "rc was %d\n", rc);
    assert("PUBACK PacketId round-trips",
           puback_pktid_out == pktid_in,
           "got %u expected %u\n", puback_pktid_out, pktid_in);
    assert("PUBACK ReasonCode success round-trips",
           puback_rc_out == MQTT_SN_RC_SUCCESS,
           "got 0x%02X\n", puback_rc_out);

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 5 – QoS 1, DUP set, Retain set, PUBACK with reason code
 * ---------------------------------------------------------------------------*/
int test5(struct Options opts)
{
    (void)opts;
    const char* testname = "test5";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;

    uint8_t  dup_in = 1, ret_in = 1;
    int32_t  qos_in   = 1;
    uint16_t pktid_in = 9999u;
    MQTTSN_topic topic_in;
    const char* payload_in    = "dup-retain-test";
    int32_t     payloadlen_in = (int32_t)strlen(payload_in);

    uint8_t  puback_rc_in  = 0x10u; /* No matching subscribers */

    uint8_t  dup_out, ret_out;
    int32_t  qos_out;
    uint16_t pktid_out;
    MQTTSN_topic topic_out;
    uint8_t* payload_out;
    int32_t  payloadlen_out;
    uint16_t puback_pktid_out;
    uint8_t  puback_rc_out;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"QoS 1 DUP Retain PUBACK with reason code\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - QoS 1, DUP, Retain, PUBACK with reason code",
          testname);

    make_alias_topic(&topic_in, MQTTSN_TOPIC_TYPE_PREDEFINED, 3u);

    /* --- PUBLISH --- */
    len = MQTTSNSerialize_publish(buf, BUF_SIZE, dup_in, qos_in, ret_in,
                                  pktid_in, &topic_in,
                                  (uint8_t*)payload_in, payloadlen_in);
    assert("Serialize publish returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                   &topic_out, &payload_out, &payloadlen_out,
                                   buf, len);
    assert("Deserialize publish returns 1", rc == 1,   "rc was %d\n", rc);
    assert("DUP 1 round-trips",             dup_out == 1, "got %d\n", dup_out);
    assert("Retain 1 round-trips",          ret_out == 1, "got %d\n", ret_out);
    assert("QoS 1 round-trips",             qos_out == 1, "got %d\n", qos_out);
    assert("PacketId round-trips",
           pktid_out == pktid_in, "got %u\n", pktid_out);

    /* --- PUBACK with non-success reason code --- */
    len = MQTTSNSerialize_puback(buf, BUF_SIZE, pktid_in, puback_rc_in);
    assert("Serialize puback with RC returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_puback(&puback_pktid_out, &puback_rc_out, buf, len);
    assert("Deserialize puback returns 1",       rc == 1, "rc was %d\n", rc);
    assert("PUBACK PacketId round-trips",
           puback_pktid_out == pktid_in,
           "got %u expected %u\n", puback_pktid_out, pktid_in);
    assert("PUBACK non-success RC round-trips",
           puback_rc_out == puback_rc_in,
           "got 0x%02X expected 0x%02X\n", puback_rc_out, puback_rc_in);

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 6 – QoS 2, Session Topic Alias, Topic Name, full exchange
 * ---------------------------------------------------------------------------*/
int test6(struct Options opts)
{
    (void)opts;
    const char* testname = "test6";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;

    uint8_t  dup_in    = 0, ret_in = 0;
    int32_t  qos_in    = 2;
    uint16_t pktid_in  = 777u;
    const char*  topic_name = "commands/set-point";
    MQTTSN_topic topic_in;
    const char*  payload_in    = "21.0";
    int32_t      payloadlen_in = (int32_t)strlen(payload_in);

    uint8_t  dup_out, ret_out;
    int32_t  qos_out;
    uint16_t pktid_out;
    MQTTSN_topic topic_out;
    uint8_t* payload_out;
    int32_t  payloadlen_out;
    uint16_t ack_pktid_out;
    uint8_t  ack_rc_out;
    uint8_t  ack_type_out;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"QoS 2 Topic Name full exchange\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - QoS 2, Topic Name, PUBREC/PUBREL/PUBCOMP",
          testname);

    make_name_topic(&topic_in, topic_name, (uint16_t)strlen(topic_name));

    /* --- PUBLISH --- */
    len = MQTTSNSerialize_publish(buf, BUF_SIZE, dup_in, qos_in, ret_in,
                                  pktid_in, &topic_in,
                                  (uint8_t*)payload_in, payloadlen_in);
    assert("Serialize publish QoS 2 returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                   &topic_out, &payload_out, &payloadlen_out,
                                   buf, len);
    assert("Deserialize publish QoS 2 returns 1", rc == 1,   "rc was %d\n", rc);
    assert("QoS 2 round-trips",    qos_out == 2,         "got %d\n", qos_out);
    assert("PacketId round-trips", pktid_out == pktid_in,"got %u\n", pktid_out);
    assert("Topic Name round-trips", topics_equal(&topic_in, &topic_out),
           "name len %u vs %u\n",
           (unsigned)topic_in.alt.string.len,
           (unsigned)topic_out.alt.string.len);
    assert("Payload data round-trips",
           memcmp(payload_out, payload_in, (size_t)payloadlen_in) == 0,
           "payload mismatch\n");

    /* --- PUBREC (success, no reason code on wire) --- */
    len = MQTTSNSerialize_pubrec(buf, BUF_SIZE, pktid_in, MQTT_SN_RC_SUCCESS);
    assert("Serialize pubrec returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_ack(&ack_type_out, &ack_pktid_out, &ack_rc_out,
                               buf, len);
    assert("Deserialize pubrec returns 1",   rc == 1, "rc was %d\n", rc);
    assert("PUBREC packet type round-trips",
           ack_type_out == MQTTSN_PUBREC,
           "got 0x%02X expected 0x%02X\n", ack_type_out, MQTTSN_PUBREC);
    assert("PUBREC PacketId round-trips",
           ack_pktid_out == pktid_in, "got %u\n", ack_pktid_out);
    assert("PUBREC success RC round-trips",
           ack_rc_out == MQTT_SN_RC_SUCCESS, "got 0x%02X\n", ack_rc_out);

    /* --- PUBREL (success, no reason code on wire) --- */
    len = MQTTSNSerialize_pubrel(buf, BUF_SIZE, pktid_in, MQTT_SN_RC_SUCCESS);
    assert("Serialize pubrel returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_ack(&ack_type_out, &ack_pktid_out, &ack_rc_out,
                               buf, len);
    assert("Deserialize pubrel returns 1",   rc == 1, "rc was %d\n", rc);
    assert("PUBREL packet type round-trips",
           ack_type_out == MQTTSN_PUBREL,
           "got 0x%02X expected 0x%02X\n", ack_type_out, MQTTSN_PUBREL);
    assert("PUBREL PacketId round-trips",
           ack_pktid_out == pktid_in, "got %u\n", ack_pktid_out);
    assert("PUBREL success RC round-trips",
           ack_rc_out == MQTT_SN_RC_SUCCESS, "got 0x%02X\n", ack_rc_out);

    /* --- PUBCOMP (success, no reason code on wire) --- */
    len = MQTTSNSerialize_pubcomp(buf, BUF_SIZE, pktid_in, MQTT_SN_RC_SUCCESS);
    assert("Serialize pubcomp returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_ack(&ack_type_out, &ack_pktid_out, &ack_rc_out,
                               buf, len);
    assert("Deserialize pubcomp returns 1",  rc == 1, "rc was %d\n", rc);
    assert("PUBCOMP packet type round-trips",
           ack_type_out == MQTTSN_PUBCOMP,
           "got 0x%02X expected 0x%02X\n", ack_type_out, MQTTSN_PUBCOMP);
    assert("PUBCOMP PacketId round-trips",
           ack_pktid_out == pktid_in, "got %u\n", ack_pktid_out);
    assert("PUBCOMP success RC round-trips",
           ack_rc_out == MQTT_SN_RC_SUCCESS, "got 0x%02X\n", ack_rc_out);

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 7 – QoS 2, DUP set, PUBREC with failure reason code
 * ---------------------------------------------------------------------------*/
int test7(struct Options opts)
{
    (void)opts;
    const char* testname = "test7";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;

    uint8_t  dup_in   = 1, ret_in = 0;
    int32_t  qos_in   = 2;
    uint16_t pktid_in = 55u;
    MQTTSN_topic topic_in;
    uint8_t  pubrec_rc_in = 0x80u; /* Unspecified error */

    uint8_t  dup_out, ret_out;
    int32_t  qos_out;
    uint16_t pktid_out;
    MQTTSN_topic topic_out;
    uint8_t* payload_out;
    int32_t  payloadlen_out;
    uint16_t ack_pktid_out;
    uint8_t  ack_rc_out;
    uint8_t  ack_type_out;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"QoS 2 DUP PUBREC with failure reason code\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - QoS 2, DUP set, PUBREC failure RC", testname);

    make_alias_topic(&topic_in, MQTTSN_TOPIC_TYPE_SESSION, 1u);

    /* --- PUBLISH --- */
    len = MQTTSNSerialize_publish(buf, BUF_SIZE, dup_in, qos_in, ret_in,
                                  pktid_in, &topic_in,
                                  (uint8_t*)"", 0);
    assert("Serialize publish DUP QoS 2 returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                   &topic_out, &payload_out, &payloadlen_out,
                                   buf, len);
    assert("Deserialize publish DUP QoS 2 returns 1", rc == 1, "rc was %d\n", rc);
    assert("DUP 1 round-trips",    dup_out == 1,        "got %d\n", dup_out);
    assert("QoS 2 round-trips",    qos_out == 2,        "got %d\n", qos_out);
    assert("PacketId round-trips", pktid_out == pktid_in,"got %u\n", pktid_out);
    assert("Zero payload length",  payloadlen_out == 0, "got %d\n", payloadlen_out);

    /* --- PUBREC with failure reason code --- */
    len = MQTTSNSerialize_pubrec(buf, BUF_SIZE, pktid_in, pubrec_rc_in);
    assert("Serialize pubrec with failure RC returns positive length",
           len > 0, "len was %d\n", (int)len);

    rc = MQTTSNDeserialize_ack(&ack_type_out, &ack_pktid_out, &ack_rc_out,
                               buf, len);
    assert("Deserialize pubrec with failure RC returns 1",
           rc == 1, "rc was %d\n", rc);
    assert("PUBREC packet type round-trips",
           ack_type_out == MQTTSN_PUBREC,
           "got 0x%02X expected 0x%02X\n", ack_type_out, MQTTSN_PUBREC);
    assert("PUBREC PacketId round-trips",
           ack_pktid_out == pktid_in,
           "got %u expected %u\n", ack_pktid_out, pktid_in);
    assert("PUBREC failure RC round-trips",
           ack_rc_out == pubrec_rc_in,
           "got 0x%02X expected 0x%02X\n", ack_rc_out, pubrec_rc_in);

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 8 – Zero-length payload, all three topic types
 * ---------------------------------------------------------------------------*/
int test8(struct Options opts)
{
    (void)opts;
    const char* testname = "test8";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;
    int      t;

    /* topic types to iterate: SESSION, PREDEFINED, NAME */
    static const uint8_t types[]  = {
        MQTTSN_TOPIC_TYPE_SESSION,
        MQTTSN_TOPIC_TYPE_PREDEFINED,
        MQTTSN_TOPIC_TYPE_NAME
    };
    static const char* type_names[] = {
        "SESSION", "PREDEFINED", "NAME"
    };
    const char* topic_str = "a/b/c";

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"zero-length payload all topic types\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - zero-length payload, all topic types", testname);

    for (t = 0; t < (int)ARRAY_SIZE(types); ++t)
    {
        MQTTSN_topic topic_in, topic_out;
        uint8_t* payload_out;
        int32_t  payloadlen_out;
        uint8_t  dup_out, ret_out;
        int32_t  qos_out;
        uint16_t pktid_out;

        if (types[t] == MQTTSN_TOPIC_TYPE_NAME)
            make_name_topic(&topic_in, topic_str, (uint16_t)strlen(topic_str));
        else
            make_alias_topic(&topic_in, types[t], (uint16_t)(t + 1));

        len = MQTTSNSerialize_publish(buf, BUF_SIZE, 0, 0, 0, 0,
                                      &topic_in, (uint8_t*)"", 0);

        {
            char desc[64];
            snprintf(desc, sizeof(desc),
                     "Serialize zero-payload %s returns positive length",
                     type_names[t]);
            assert(desc, len > 0, "len was %d\n", (int)len);
        }

        rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out, &pktid_out,
                                       &topic_out, &payload_out, &payloadlen_out,
                                       buf, len);
        {
            char desc[64];
            snprintf(desc, sizeof(desc),
                     "Deserialize zero-payload %s returns 1", type_names[t]);
            assert(desc, rc == 1, "rc was %d\n", rc);
        }
        {
            char desc[64];
            snprintf(desc, sizeof(desc),
                     "Zero payload length preserved for %s", type_names[t]);
            assert(desc, payloadlen_out == 0, "got %d\n", payloadlen_out);
        }
        {
            char desc[64];
            snprintf(desc, sizeof(desc),
                     "Topic round-trips for %s", type_names[t]);
            assert(desc, topics_equal(&topic_in, &topic_out),
                   "topic mismatch for %s\n", type_names[t]);
        }
    }

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 9 – All QoS levels × both alias topic types in a loop
 * ---------------------------------------------------------------------------*/
int test9(struct Options opts)
{
    (void)opts;
    const char* testname = "test9";
    uint8_t  buf[BUF_SIZE];
    int32_t  len;
    int      rc;
    int      qos;
    int      tt;

    static const uint8_t  topic_types[] = {
        MQTTSN_TOPIC_TYPE_SESSION, MQTTSN_TOPIC_TYPE_PREDEFINED
    };
    static const char* tt_names[] = { "SESSION", "PREDEFINED" };
    uint16_t pktid = 100u;

    const char* payload_in    = "sweep-test-payload";
    int32_t     payloadlen_in = (int32_t)strlen(payload_in);

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"all QoS x both alias topic types\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - all QoS × alias topic types", testname);

    for (qos = 0; qos <= 2; ++qos)
    {
        for (tt = 0; tt < (int)ARRAY_SIZE(topic_types); ++tt)
        {
            MQTTSN_topic topic_in, topic_out;
            uint8_t* payload_out;
            int32_t  payloadlen_out;
            uint8_t  dup_out, ret_out;
            int32_t  qos_out;
            uint16_t pktid_out;
            char     desc[80];
            uint16_t use_pktid = (qos > 0) ? pktid++ : 0u;

            make_alias_topic(&topic_in, topic_types[tt], (uint16_t)(tt + 5));

            len = MQTTSNSerialize_publish(buf, BUF_SIZE, 0, qos, 0,
                                          use_pktid, &topic_in,
                                          (uint8_t*)payload_in, payloadlen_in);
            snprintf(desc, sizeof(desc),
                     "Serialize QoS %d %s positive length", qos, tt_names[tt]);
            assert(desc, len > 0, "len was %d\n", (int)len);

            rc = MQTTSNDeserialize_publish(&dup_out, &qos_out, &ret_out,
                                           &pktid_out, &topic_out,
                                           &payload_out, &payloadlen_out,
                                           buf, len);
            snprintf(desc, sizeof(desc),
                     "Deserialize QoS %d %s returns 1", qos, tt_names[tt]);
            assert(desc, rc == 1, "rc was %d\n", rc);

            snprintf(desc, sizeof(desc),
                     "QoS %d round-trips for %s", qos, tt_names[tt]);
            assert(desc, qos_out == qos, "got %d\n", qos_out);

            snprintf(desc, sizeof(desc),
                     "PacketId round-trips QoS %d %s", qos, tt_names[tt]);
            assert(desc,
                   qos == 0 ? pktid_out == 0u : pktid_out == use_pktid,
                   "got %u expected %u\n", pktid_out, use_pktid);

            snprintf(desc, sizeof(desc),
                     "Payload round-trips QoS %d %s", qos, tt_names[tt]);
            assert(desc, payloadlen_out == payloadlen_in &&
                         memcmp(payload_out, payload_in,
                                (size_t)payloadlen_in) == 0,
                   "mismatch len %d\n", payloadlen_out);
        }
    }

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * Test 10 – Buffer too small: serialize must return a negative error
 * ---------------------------------------------------------------------------*/
int test10(struct Options opts)
{
    (void)opts;
    const char* testname = "test10";
    uint8_t  buf[4];   /* deliberately too small */
    int32_t  len;
    MQTTSN_topic topic_in;

    fprintf(xml, "<testcase classname=\"test_mqttsn_publish\" "
                 "name=\"buffer too small returns error\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting %s - buffer too small", testname);

    make_alias_topic(&topic_in, MQTTSN_TOPIC_TYPE_SESSION, 1u);

    len = MQTTSNSerialize_publish(buf, (int32_t)sizeof(buf),
                                  0, 1, 0, 1u, &topic_in,
                                  (uint8_t*)"hello world", 11);
    assert("Serialize into tiny buffer returns negative length",
           len < 0, "len was %d (expected < 0)\n", (int)len);

    MyLog(LOGA_INFO, "%s: %s. %d tests run, %d failures.",
          (failures == 0) ? "passed" : "FAILED", testname, tests, failures);
    write_test_result();
    return failures;
}


/* ---------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    int rc = 0;
    int (*tests[])(struct Options) = {
        NULL,   /* 0: placeholder so test_no matches index */
        test1,  /* 1:  QoS 0, Session alias */
        test2,  /* 2:  QoS 0, Predefined alias, Retain */
        test3,  /* 3:  QoS 0, Topic Name */
        test4,  /* 4:  QoS 1, Session alias, PUBACK no RC */
        test5,  /* 5:  QoS 1, DUP+Retain, PUBACK with RC */
        test6,  /* 6:  QoS 2, Topic Name, full PUBREC/PUBREL/PUBCOMP */
        test7,  /* 7:  QoS 2, DUP, PUBREC failure RC */
        test8,  /* 8:  Zero-length payload, all topic types */
        test9,  /* 9:  All QoS × both alias topic types */
        test10, /* 10: Buffer too small */
    };
    int num_tests = (int)ARRAY_SIZE(tests) - 1;
    int i;

    getopts(argc, argv);

    xml = fopen("TEST-test_mqttsn_publish.xml", "w");
    fprintf(xml, "<testsuite name=\"test_mqttsn_publish\" tests=\"%d\">\n",
            num_tests);

    if (options.test_no == 0)
    {
        /* Run all tests */
        for (i = 1; i <= num_tests; ++i)
            rc += tests[i](options);
    }
    else if (options.test_no >= 1 && options.test_no <= num_tests)
    {
        rc = tests[options.test_no](options);
    }
    else
    {
        fprintf(stderr, "test_no %d out of range (1-%d)\n",
                options.test_no, num_tests);
        rc = 1;
    }

    if (rc == 0)
        MyLog(LOGA_INFO, "verdict pass");
    else
        MyLog(LOGA_INFO, "verdict fail");

    fprintf(xml, "</testsuite>\n");
    fclose(xml);

    return rc;
}
