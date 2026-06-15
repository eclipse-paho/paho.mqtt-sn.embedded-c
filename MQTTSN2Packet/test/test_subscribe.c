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
 * Round-trip tests for MQTTSNSerialize_subscribe / MQTTSNDeserialize_subscribe.
 *
 * Each test calls MQTTSNSerialize_subscribe with a specific combination of
 * option values, then passes the resulting wire buffer straight to
 * MQTTSNDeserialize_subscribe and asserts that every field survives the
 * round-trip unchanged.
 *
 * Coverage:
 *   - QoS 0, 1, 2
 *   - retain_handling 0, 1, 2
 *   - rap 0 and 1
 *   - no_local 0 and 1
 *   - all topic types: FILTER, NAME, SESSION, PREDEFINED
 *   - non-trivial packet identifiers (including 0xFFFF boundary)
 *   - topic filter strings: short, long, containing wildcards
 *   - topic aliases: minimum (1) and maximum (0xFFFF)
 *   - buffer too short rejection
 *   - all flags set simultaneously
 */

#include "MQTTSNPacket.h"
#include "MQTTSNSubscribe.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Test infrastructure
 * ------------------------------------------------------------------------- */

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

/* Run one serialize→deserialize round-trip and check every field.
 *
 * Returns 1 if all checks pass, 0 otherwise.
 */
static int subscribe_roundtrip(
        const char* test_name,
        int32_t       in_qos,
        uint8_t       in_retain_handling,
        uint8_t       in_rap,
        uint8_t       in_no_local,
        uint16_t      in_packetid,
        MQTTSN_topic* in_topic)
{
	uint8_t  buf[512];
	int32_t  buflen = (int32_t)sizeof(buf);
	int32_t  slen;

	int32_t  out_qos             = -1;
	uint8_t  out_retain_handling = 0xFF;
	uint8_t  out_rap             = 0xFF;
	uint8_t  out_no_local        = 0xFF;
	uint16_t out_packetid        = 0;
	MQTTSN_topic out_topic;
	int32_t  drc;
	int      ok = 1;

	printf("%-55s ", test_name);

	memset(buf, 0, sizeof(buf));
	memset(&out_topic, 0, sizeof(out_topic));

	/* --- serialize --- */
	slen = MQTTSNSerialize_subscribe(buf, buflen,
	        in_qos, in_retain_handling, in_rap, in_no_local,
	        in_packetid, in_topic);

	if (slen <= 0) {
		printf("FAIL  (serialize returned %d)\n", (int)slen);
		++tests_run; ++tests_failed;
		return 0;
	}

	/* --- deserialize --- */
	drc = MQTTSNDeserialize_subscribe(&out_qos, &out_retain_handling,
	        &out_rap, &out_no_local, &out_packetid,
	        &out_topic, buf, slen);

	++tests_run;
	if (drc != 1) {
		printf("FAIL  (deserialize returned %d)\n", (int)drc);
		++tests_failed;
		return 0;
	}
	++tests_passed;

	/* --- field checks --- */
#define RT_CHECK(label, expr) \
	do { ++tests_run; if (expr) { ++tests_passed; } \
	     else { ++tests_failed; ok = 0; \
	            printf("\n  FAIL  field: %s", (label)); } } while (0)

	RT_CHECK("qos",             out_qos             == in_qos);
	RT_CHECK("retain_handling", out_retain_handling == in_retain_handling);
	RT_CHECK("rap",             out_rap             == in_rap);
	RT_CHECK("no_local",        out_no_local        == in_no_local);
	RT_CHECK("packetid",        out_packetid        == in_packetid);
	RT_CHECK("topic.type",      out_topic.type      == in_topic->type);

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
	else
	{
		RT_CHECK("alias",
		         out_topic.alt.alias == in_topic->alt.alias);
	}

#undef RT_CHECK

	if (ok)
		printf("ok\n");
	else
		printf("\n");

	return ok;
}


/* -------------------------------------------------------------------------
 * Individual test cases
 * ------------------------------------------------------------------------- */

/* QoS variants ------------------------------------------------------------ */

static void test_qos_0(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 8, "sensors/#" } } };
	subscribe_roundtrip("QoS 0 / filter topic",
	        0, 0, 0, 0, 1, &t);
}

static void test_qos_1(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 8, "sensors/#" } } };
	subscribe_roundtrip("QoS 1 / filter topic",
	        1, 0, 0, 0, 2, &t);
}

static void test_qos_2(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 8, "sensors/#" } } };
	subscribe_roundtrip("QoS 2 / filter topic",
	        2, 0, 0, 0, 3, &t);
}

/* retain_handling variants ------------------------------------------------ */

static void test_retain_handling_0(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "test" } } };
	subscribe_roundtrip("retain_handling 0 (send retained on subscribe)",
	        1, 0, 0, 0, 10, &t);
}

static void test_retain_handling_1(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "test" } } };
	subscribe_roundtrip("retain_handling 1 (only if new subscription)",
	        1, 1, 0, 0, 11, &t);
}

static void test_retain_handling_2(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "test" } } };
	subscribe_roundtrip("retain_handling 2 (never send retained)",
	        1, 2, 0, 0, 12, &t);
}

/* rap variants ------------------------------------------------------------ */

static void test_rap_0(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "test" } } };
	subscribe_roundtrip("rap 0 (RETAIN cleared on forwarded messages)",
	        1, 0, 0, 0, 20, &t);
}

static void test_rap_1(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "test" } } };
	subscribe_roundtrip("rap 1 (keep RETAIN flag on forwarded messages)",
	        1, 0, 1, 0, 21, &t);
}

/* no_local variants ------------------------------------------------------- */

static void test_no_local_0(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "test" } } };
	subscribe_roundtrip("no_local 0 (deliver own publications)",
	        1, 0, 0, 0, 30, &t);
}

static void test_no_local_1(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "test" } } };
	subscribe_roundtrip("no_local 1 (suppress own publications)",
	        1, 0, 0, 1, 31, &t);
}

/* Topic type: FILTER ------------------------------------------------------ */

static void test_topic_filter_short(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 1, "#" } } };
	subscribe_roundtrip("topic type FILTER / single-char wildcard",
	        1, 0, 0, 0, 40, &t);
}

static void test_topic_filter_multilevel_wildcard(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 14, "building/+/temp" } } };
	subscribe_roundtrip("topic type FILTER / multilevel wildcard",
	        1, 0, 0, 0, 41, &t);
}

static void test_topic_filter_long(void)
{
	/* 48-character filter to exercise multi-byte length handling */
	static const char filter[] = "very/long/topic/filter/path/for/testing/purposes";
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, (uint16_t)(sizeof(filter) - 1), (char*)filter } } };
	subscribe_roundtrip("topic type FILTER / long path (48 chars)",
	        2, 1, 1, 1, 42, &t);
}

/* Topic type: NAME -------------------------------------------------------- */

static void test_topic_name(void)
{
	/* MQTTSN_TOPIC_TYPE_NAME == MQTTSN_TOPIC_TYPE_FILTER on the wire (both 0b11);
	 * verify that the enum alias round-trips correctly */
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_NAME,
	                   { .string = { false, 12, "device/status" } } };
	subscribe_roundtrip("topic type NAME (alias for FILTER, 0b11)",
	        0, 0, 0, 0, 43, &t);
}

/* Topic type: SESSION ----------------------------------------------------- */

static void test_topic_session_min(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 1 } };
	subscribe_roundtrip("topic type SESSION / alias 1 (minimum)",
	        1, 0, 0, 0, 50, &t);
}

static void test_topic_session_typical(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 0x1234 } };
	subscribe_roundtrip("topic type SESSION / alias 0x1234",
	        2, 0, 0, 0, 51, &t);
}

static void test_topic_session_max(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 0xFFFF } };
	subscribe_roundtrip("topic type SESSION / alias 0xFFFF (maximum)",
	        0, 0, 0, 0, 52, &t);
}

/* Topic type: PREDEFINED -------------------------------------------------- */

static void test_topic_predefined_min(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 1 } };
	subscribe_roundtrip("topic type PREDEFINED / alias 1 (minimum)",
	        1, 0, 0, 0, 60, &t);
}

static void test_topic_predefined_typical(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 0x00AB } };
	subscribe_roundtrip("topic type PREDEFINED / alias 0x00AB",
	        0, 0, 0, 0, 61, &t);
}

static void test_topic_predefined_max(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_PREDEFINED, { .alias = 0xFFFF } };
	subscribe_roundtrip("topic type PREDEFINED / alias 0xFFFF (maximum)",
	        2, 0, 0, 0, 62, &t);
}

/* Packet identifier boundary values -------------------------------------- */

static void test_packetid_1(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 1 } };
	subscribe_roundtrip("packetid 1 (minimum non-zero)",
	        0, 0, 0, 0, 1, &t);
}

static void test_packetid_max(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_SESSION, { .alias = 1 } };
	subscribe_roundtrip("packetid 0xFFFF (maximum)",
	        0, 0, 0, 0, 0xFFFF, &t);
}

/* All flags set simultaneously ------------------------------------------- */

static void test_all_flags(void)
{
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 4, "a/b#" } } };
	subscribe_roundtrip("all flags set: QoS 2 / rh 2 / rap 1 / no_local 1",
	        2, 2, 1, 1, 0x5A5A, &t);
}

/* Buffer too short -------------------------------------------------------- */

static void test_buffer_too_short(void)
{
	uint8_t  buf[3]; /* deliberately too small */
	MQTTSN_topic t = { MQTTSN_TOPIC_TYPE_FILTER,
	                   { .string = { false, 10, "some/topic" } } };
	int32_t  slen;

	printf("%-55s ", "buffer too short (serialize must return error)");
	++tests_run;

	slen = MQTTSNSerialize_subscribe(buf, (int32_t)sizeof(buf),
	        1, 0, 0, 0, 100, &t);

	if (slen == MQTTSNPACKET_BUFFER_TOO_SHORT) {
		++tests_passed;
		printf("ok\n");
	} else {
		++tests_failed;
		printf("FAIL  (expected %d, got %d)\n",
		       MQTTSNPACKET_BUFFER_TOO_SHORT, (int)slen);
	}
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
	printf("=== MQTT-SN 2.0 SUBSCRIBE serialize/deserialize round-trip tests ===\n\n");

	printf("--- QoS ---\n");
	test_qos_0();
	test_qos_1();
	test_qos_2();

	printf("\n--- retain_handling ---\n");
	test_retain_handling_0();
	test_retain_handling_1();
	test_retain_handling_2();

	printf("\n--- rap (Retain as Published) ---\n");
	test_rap_0();
	test_rap_1();

	printf("\n--- no_local ---\n");
	test_no_local_0();
	test_no_local_1();

	printf("\n--- topic type: FILTER ---\n");
	test_topic_filter_short();
	test_topic_filter_multilevel_wildcard();
	test_topic_filter_long();

	printf("\n--- topic type: NAME ---\n");
	test_topic_name();

	printf("\n--- topic type: SESSION ---\n");
	test_topic_session_min();
	test_topic_session_typical();
	test_topic_session_max();

	printf("\n--- topic type: PREDEFINED ---\n");
	test_topic_predefined_min();
	test_topic_predefined_typical();
	test_topic_predefined_max();

	printf("\n--- packet identifier boundary values ---\n");
	test_packetid_1();
	test_packetid_max();

	printf("\n--- combined / boundary ---\n");
	test_all_flags();
	test_buffer_too_short();

	printf("\n=== Results: %d run, %d passed, %d failed ===\n",
	       tests_run, tests_passed, tests_failed);

	return (tests_failed == 0) ? 0 : 1;
}
