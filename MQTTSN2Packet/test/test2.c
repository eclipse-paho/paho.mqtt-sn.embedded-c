/*******************************************************************************
 * Copyright (c) 2026 Ian Craggs
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
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

/*
 * Network test suite for MQTT-SN 2.0.
 *
 * Tests connect to an MQTT-SN 2.0 gateway on localhost:1883 over UDP
 * and exercise the functions in:
 *   MQTTSNConnectClient.c    — CONNECT, CONNACK, DISCONNECT, PINGREQ, PINGRESP
 *   MQTTSNSubscribeClient.c  — SUBSCRIBE, SUBACK
 *   MQTTSNSerializePublish.c — PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP
 *   MQTTSNDeserializePublish.c
 *
 * Build:
 *   cc -o test2 test2.c MQTTSNPacket2.c MQTTSNConnectClient.c \
 *      MQTTSNSubscribeClient.c MQTTSNSerializePublish.c \
 *      MQTTSNDeserializePublish.c transport.c \
 *      -DNOSTACKTRACE -DNDEBUG -I. -std=c11
 *
 * Run all tests:
 *   ./test2
 * Run one test:
 *   ./test2 --test_no 5
 * Run with verbose logging:
 *   ./test2 --verbose
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"
#include "MQTTSNSubscribe.h"
#include "MQTTSNPublish.h"
#include "transport.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#if !defined(_WINDOWS)
	#include <sys/time.h>
	#include <sys/timeb.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <errno.h>
#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define MAXHOSTNAMELEN 256
	#define EAGAIN WSAEWOULDBLOCK
	#define EINTR WSAEINTR
	#define EINPROGRESS WSAEINPROGRESS
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define ENOTCONN WSAENOTCONN
	#define ECONNRESET WSAECONNRESET
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/* -------------------------------------------------------------------------
 * Program options
 * ------------------------------------------------------------------------- */

struct Options
{
	char* host;       /**< MQTT-SN gateway host */
	int   port;       /**< MQTT-SN gateway UDP port */
	int   verbose;
	int   test_no;
} options =
{
	"localhost",
	1883,
	0,
	0,
};

static void usage(void)
{
	printf("Usage: test2 [--host h] [--port p] [--test_no n] [--verbose]\n");
}

static void getopts(int argc, char** argv)
{
	int count = 1;

	while (count < argc)
	{
		if (strcmp(argv[count], "--test_no") == 0)
		{
			if (++count < argc)
				options.test_no = atoi(argv[count]);
			else
				usage();
		}
		else if (strcmp(argv[count], "--host") == 0)
		{
			if (++count < argc)
			{
				options.host = argv[count];
				printf("\nSetting host to %s\n", options.host);
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--port") == 0)
		{
			if (++count < argc)
			{
				options.port = atoi(argv[count]);
				printf("\nSetting port to %d\n", options.port);
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--verbose") == 0)
		{
			options.verbose = 1;
			printf("\nSetting verbose on\n");
		}
		count++;
	}
}


/* -------------------------------------------------------------------------
 * Logging
 * ------------------------------------------------------------------------- */

#define LOGA_DEBUG 0
#define LOGA_INFO  1

static void MyLog(int LOGA_level, char* format, ...)
{
	static char msg_buf[256];
	va_list args;
	struct timeb ts;
	struct tm* timeinfo;

	if (LOGA_level == LOGA_DEBUG && options.verbose == 0)
		return;

	ftime(&ts);
	timeinfo = localtime(&ts.time);
	strftime(msg_buf, 80, "%Y%m%d %H%M%S", timeinfo);
	sprintf(&msg_buf[strlen(msg_buf)], ".%.3hu ", ts.millitm);

	va_start(args, format);
	vsnprintf(&msg_buf[strlen(msg_buf)], sizeof(msg_buf) - strlen(msg_buf),
	          format, args);
	va_end(args);

	printf("%s\n", msg_buf);
	fflush(stdout);
}


/* -------------------------------------------------------------------------
 * Timing
 * ------------------------------------------------------------------------- */

#if defined(WIN32) || defined(_WINDOWS)
#define mqsleep(A) Sleep(1000*A)
#define START_TIME_TYPE DWORD
static DWORD start_time = 0;
START_TIME_TYPE start_clock(void)
{
	return GetTickCount();
}
long elapsed(START_TIME_TYPE start_time)
{
	return GetTickCount() - start_time;
}
#else
#define mqsleep sleep
#define START_TIME_TYPE struct timeval
START_TIME_TYPE start_clock(void)
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	return start_time;
}
long elapsed(START_TIME_TYPE start_time)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&now, &start_time, &res);
	return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}
#endif


/* -------------------------------------------------------------------------
 * Assertions and test result recording
 * ------------------------------------------------------------------------- */

#define assert(a, b, c, d)       myassert(__FILE__, __LINE__, a, b, c, d)
#define assert1(a, b, c, d, e)   myassert(__FILE__, __LINE__, a, b, c, d, e)

static int tests = 0;
static int failures = 0;
static FILE* xml;
static START_TIME_TYPE global_start_time;
static char output[3000];
static char* cur_output = output;

static void write_test_result(void)
{
	long duration = elapsed(global_start_time);

	fprintf(xml, " time=\"%ld.%.3ld\" >\n", duration / 1000, duration % 1000);
	if (cur_output != output)
	{
		fprintf(xml, "%s", output);
		cur_output = output;
	}
	fprintf(xml, "</testcase>\n");
}

static void myassert(char* filename, int lineno, char* description,
                     int value, char* format, ...)
{
	++tests;
	if (!value)
	{
		va_list args;
		++failures;
		printf("Assertion failed, file %s, line %d, description: %s\n",
		       filename, lineno, description);
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		cur_output += sprintf(cur_output,
		    "<failure type=\"%s\">file %s, line %d </failure>\n",
		    description, filename, lineno);
	}
	else
		MyLog(LOGA_DEBUG, "Assertion succeeded, file %s, line %d, description: %s",
		      filename, lineno, description);
}


/* -------------------------------------------------------------------------
 * Shared test constants
 * ------------------------------------------------------------------------- */

#define BUF_SIZE  512

/* Topic used throughout — a stable name unlikely to conflict with production */
#define TEST_TOPIC       "test/mqttsn2"
#define TEST_TOPIC_LEN   12             /* strlen("test/mqttsn2") */
#define TEST_PAYLOAD     "hello MQTT-SN 2.0"
#define TEST_PAYLOAD_LEN 17

/* Receive timeout for transport_getdata — 5 seconds */
#define RECV_TIMEOUT_S 5


/* -------------------------------------------------------------------------
 * Transport helpers
 * ------------------------------------------------------------------------- */

/*
 * Open the UDP socket and set a receive timeout so that transport_getdata()
 * returns SOCKET_ERROR / -1 rather than hanging forever if the gateway does
 * not respond.
 */
static int open_transport_with_timeout(void)
{
	int sock = transport_open();
	if (sock < 0)
		return sock;
#if defined(WIN32)
	DWORD tv_ms = RECV_TIMEOUT_S * 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
	           (const char*)&tv_ms, sizeof(tv_ms));
#else
	struct timeval tv = { RECV_TIMEOUT_S, 0 };
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv));
#endif
	return sock;
}

/*
 * Return the packet type byte from a received buffer, or -1 on decode error.
 */
static int packet_type_in_buf(unsigned char* buf, int len)
{
	int mylen = 0;
	int lenlen = MQTTSNPacket_decode(buf, len, &mylen);
	if (lenlen < 0 || len < lenlen + 1)
		return -1;
	return (int)(uint8_t)buf[lenlen];
}


/* -------------------------------------------------------------------------
 * Protocol helpers — shared by multiple tests
 * ------------------------------------------------------------------------- */

/*
 * Serialize and send a CONNECT, then receive and deserialize the CONNACK.
 * Fills *connack_out with the server response.
 * Returns 1 on success, 0 on failure.
 */
static int do_connect(struct Options options, const char* clientid,
                      int clean_start, uint32_t session_expiry_interval,
                      MQTTSNPacket_connackData* connack_out)
{
	MQTTSNPacket_connectData conn = MQTTSNPacket_connectData_initializer;
	unsigned char buf[BUF_SIZE];
	unsigned char rbuf[BUF_SIZE];
	int32_t slen, rlen;
	int ptype;

	conn.clientID.data           = (char*)clientid;
	conn.clientID.len            = (uint16_t)strlen(clientid);
	conn.keepAlive               = 30;
	conn.maxPacketSize           = BUF_SIZE;
	conn.packetId                = 1;
	conn.flags.bits.cleanStart   = (clean_start != 0);

	if (session_expiry_interval > 0)
	{
		conn.flags.bits.sessionExpiryInterval = 1;
		conn.sessionExpiryInterval = session_expiry_interval;
	}

	slen = MQTTSNSerialize_connect(buf, (int32_t)sizeof(buf), &conn);
	if (slen <= 0) return 0;

	MyLog(LOGA_DEBUG, "Sending CONNECT (clientid=%s)", clientid);
	if (transport_sendPacketBuffer(options.host, options.port, buf, (int)slen) != 0)
		return 0;

	rlen = transport_getdata(rbuf, (int)sizeof(rbuf));
	if (rlen < 2) return 0;

	ptype = packet_type_in_buf(rbuf, rlen);
	if (ptype != MQTTSN_CONNACK)
	{
		MyLog(LOGA_DEBUG, "Expected CONNACK (0x%02X) got 0x%02X", MQTTSN_CONNACK, ptype);
		return 0;
	}

	memset(connack_out, 0, sizeof(*connack_out));
	return MQTTSNDeserialize_connack(connack_out, rbuf, rlen);
}

/*
 * Send DISCONNECT and close the transport.
 */
static void do_disconnect(struct Options options, uint8_t reason_code)
{
	unsigned char buf[8];
	int32_t slen = MQTTSNSerialize_disconnect(buf, (int32_t)sizeof(buf), reason_code);
	if (slen > 0)
		transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
	transport_close();
	MyLog(LOGA_DEBUG, "Disconnected (reason=0x%02X)", reason_code);
}

/*
 * Send a SUBSCRIBE and receive the SUBACK.
 * Fills *topicid_out with the assigned Topic Alias (0 if not present).
 * Returns the SUBACK Reason Code, or -1 on error.
 */
static int do_subscribe(struct Options options,
                        const char* filter, int qos, uint16_t packetid,
                        uint16_t* topicid_out)
{
	MQTTSN_topic topic;
	unsigned char buf[BUF_SIZE];
	unsigned char rbuf[BUF_SIZE];
	int32_t slen, rlen;
	uint16_t ret_topicid = 0;
	uint8_t  topic_alias_present = 0;
	uint16_t ret_packetid = 0;
	uint8_t  reason_code = 0xFF;
	int ptype;

	memset(&topic, 0, sizeof(topic));
	topic.type            = MQTTSN_TOPIC_TYPE_FILTER;
	topic.alt.string.data = (char*)filter;
	topic.alt.string.len  = (uint16_t)strlen(filter);

	slen = MQTTSNSerialize_subscribe(buf, (int32_t)sizeof(buf),
	           qos, 0, 0, 0, packetid, &topic);
	if (slen <= 0) return -1;

	MyLog(LOGA_DEBUG, "Sending SUBSCRIBE (filter=\"%s\", qos=%d)", filter, qos);
	if (transport_sendPacketBuffer(options.host, options.port, buf, (int)slen) != 0)
		return -1;

	rlen = transport_getdata(rbuf, (int)sizeof(rbuf));
	if (rlen < 2) return -1;

	ptype = packet_type_in_buf(rbuf, rlen);
	if (ptype != MQTTSN_SUBACK)
	{
		MyLog(LOGA_DEBUG, "Expected SUBACK (0x%02X) got 0x%02X", MQTTSN_SUBACK, ptype);
		return -1;
	}

	if (MQTTSNDeserialize_suback(&ret_topicid, &topic_alias_present,
	                              &ret_packetid, &reason_code,
	                              rbuf, rlen) != 1)
		return -1;

	MyLog(LOGA_DEBUG,
	      "SUBACK: packetid=%u reason_code=0x%02X topic_alias_present=%d topicid=%u",
	      ret_packetid, reason_code, topic_alias_present, ret_topicid);

	if (topicid_out)
		*topicid_out = topic_alias_present ? ret_topicid : 0;

	return (int)reason_code;
}


/* =========================================================================
 * test1 — CONNECT / CONNACK: minimal clean start
 * ========================================================================= */

int test1(struct Options options)
{
	MQTTSNPacket_connackData connack;
	int rc = 0;

	fprintf(xml, "<testcase classname=\"test2\" name=\"connect-connack-minimal\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 1 - CONNECT / CONNACK (minimal clean start)");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test1", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK reason code is success",
	       connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);

	MyLog(LOGA_DEBUG, "CONNACK: packetid=%u reasonCode=0x%02X sessionPresent=%d",
	      connack.packetId, connack.reasonCode, connack.flags.bits.sessionPresent);

	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test2 — CONNECT / CONNACK: with Session Expiry Interval
 * ========================================================================= */

int test2(struct Options options)
{
	MQTTSNPacket_connackData connack;
	int rc = 0;

	fprintf(xml, "<testcase classname=\"test2\" name=\"connect-connack-session-expiry\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 2 - CONNECT / CONNACK (Session Expiry Interval)");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	/* cleanStart=0 so the session persists; SEI=3600 seconds */
	rc = do_connect(options, "mqttsn2-test2", 0, 3600, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK reason code is success",
	       connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);

	/* If the server echoes back a Session Expiry Interval, log it */
	if (connack.flags.bits.sessionExpiryInterval)
		MyLog(LOGA_DEBUG, "Server session expiry interval: %u",
		      connack.sessionExpiryInterval);
	if (connack.flags.bits.serverKeepAlive)
		MyLog(LOGA_DEBUG, "Server keep alive: %u", connack.serverKeepAlive);

	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST2: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test3 — CONNECT + SUBSCRIBE (QoS 0) / SUBACK
 * ========================================================================= */

int test3(struct Options options)
{
	MQTTSNPacket_connackData connack;
	int rc = 0;
	uint16_t topicid = 0;
	int suback_rc;

	fprintf(xml, "<testcase classname=\"test2\" name=\"subscribe-qos0\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 3 - SUBSCRIBE QoS 0 / SUBACK");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test3", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	suback_rc = do_subscribe(options, TEST_TOPIC, 0, 1, &topicid);
	assert("SUBACK received", suback_rc >= 0, "suback_rc was %d\n", suback_rc);
	assert("SUBACK granted QoS 0",
	       suback_rc == MQTT_SN_RC_GRANTED_QOS_0,
	       "suback reason code was 0x%02X\n", suback_rc);

	MyLog(LOGA_DEBUG, "Subscribed to \"" TEST_TOPIC "\" QoS 0, topicid=%u", topicid);

disconnect:
	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST3: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test4 — CONNECT + SUBSCRIBE (QoS 1) / SUBACK
 * ========================================================================= */

int test4(struct Options options)
{
	MQTTSNPacket_connackData connack;
	int rc = 0;
	uint16_t topicid = 0;
	int suback_rc;

	fprintf(xml, "<testcase classname=\"test2\" name=\"subscribe-qos1\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 4 - SUBSCRIBE QoS 1 / SUBACK");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test4", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	suback_rc = do_subscribe(options, TEST_TOPIC, 1, 1, &topicid);
	assert("SUBACK received", suback_rc >= 0, "suback_rc was %d\n", suback_rc);
	/* Granted QoS may be 0 or 1 — either is a success */
	assert("SUBACK granted QoS <= 1",
	       suback_rc == MQTT_SN_RC_GRANTED_QOS_0 ||
	       suback_rc == MQTT_SN_RC_GRANTED_QOS_1,
	       "suback reason code was 0x%02X\n", suback_rc);

	MyLog(LOGA_DEBUG, "Subscribed to \"" TEST_TOPIC "\" QoS 1, topicid=%u", topicid);

disconnect:
	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST4: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test5 — CONNECT + PUBLISH QoS 0 (no acknowledgement expected)
 * ========================================================================= */

int test5(struct Options options)
{
	MQTTSNPacket_connackData connack;
	MQTTSN_topic topic;
	unsigned char buf[BUF_SIZE];
	int32_t slen;
	int rc = 0;

	fprintf(xml, "<testcase classname=\"test2\" name=\"publish-qos0\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 5 - PUBLISH QoS 0");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test5", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	memset(&topic, 0, sizeof(topic));
	topic.type            = MQTTSN_TOPIC_TYPE_NAME;
	topic.alt.string.data = TEST_TOPIC;
	topic.alt.string.len  = TEST_TOPIC_LEN;

	slen = MQTTSNSerialize_publish(buf, (int32_t)sizeof(buf),
	           0, 0, 0, 0,   /* dup=0, qos=0, retained=0, packetid=0 */
	           &topic,
	           (uint8_t*)TEST_PAYLOAD, TEST_PAYLOAD_LEN);
	assert("PUBLISH QoS 0 serialized", slen > 0, "slen was %d\n", (int)slen);

	if (slen > 0)
	{
		rc = transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
		assert("PUBLISH QoS 0 sent", rc == 0, "rc was %d\n", rc);
		MyLog(LOGA_DEBUG, "Published QoS 0 to \"" TEST_TOPIC "\" (%d bytes)", (int)slen);
		/* QoS 0: no acknowledgement; nothing to receive */
	}

disconnect:
	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST5: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test6 — CONNECT + PUBLISH QoS 1 / PUBACK
 * ========================================================================= */

int test6(struct Options options)
{
	MQTTSNPacket_connackData connack;
	MQTTSN_topic topic;
	unsigned char buf[BUF_SIZE];
	unsigned char rbuf[BUF_SIZE];
	int32_t slen, rlen;
	uint16_t packetid = 1;
	uint16_t ret_packetid = 0;
	uint8_t  ret_returncode = 0xFF;
	int rc = 0;
	int ptype;

	fprintf(xml, "<testcase classname=\"test2\" name=\"publish-qos1-puback\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 6 - PUBLISH QoS 1 / PUBACK");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test6", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	memset(&topic, 0, sizeof(topic));
	topic.type            = MQTTSN_TOPIC_TYPE_NAME;
	topic.alt.string.data = TEST_TOPIC;
	topic.alt.string.len  = TEST_TOPIC_LEN;

	slen = MQTTSNSerialize_publish(buf, (int32_t)sizeof(buf),
	           0, 1, 0, packetid,  /* dup=0, qos=1, retained=0, packetid=1 */
	           &topic,
	           (uint8_t*)TEST_PAYLOAD, TEST_PAYLOAD_LEN);
	assert("PUBLISH QoS 1 serialized", slen > 0, "slen was %d\n", (int)slen);
	if (slen <= 0) goto disconnect;

	MyLog(LOGA_DEBUG, "Sending PUBLISH QoS 1 (packetid=%u)", packetid);
	rc = transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
	assert("PUBLISH QoS 1 sent", rc == 0, "rc was %d\n", rc);
	if (rc != 0) goto disconnect;

	/* receive PUBACK */
	rlen = transport_getdata(rbuf, (int)sizeof(rbuf));
	assert("PUBACK data received", rlen > 0, "rlen was %d\n", rlen);
	if (rlen <= 0) goto disconnect;

	ptype = packet_type_in_buf(rbuf, rlen);
	assert("PUBACK packet type", ptype == MQTTSN_PUBACK,
	       "packet type was 0x%02X\n", ptype);

	rc = MQTTSNDeserialize_puback(&ret_packetid, &ret_returncode, rbuf, rlen);
	assert("PUBACK deserialized", rc == 1, "rc was %d\n", rc);
	assert("PUBACK packetid matches",
	       ret_packetid == packetid, "packetid was %u\n", ret_packetid);
	assert("PUBACK return code success",
	       ret_returncode == MQTT_SN_RC_SUCCESS ||
	       ret_returncode == MQTT_SN_RC_NO_MATCHING_SUBSCRIBERS,
	       "return code was 0x%02X\n", ret_returncode);

	MyLog(LOGA_DEBUG, "PUBACK received (packetid=%u returncode=0x%02X)",
	      ret_packetid, ret_returncode);

disconnect:
	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST6: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test7 — CONNECT + PUBLISH QoS 2 (PUBREC / PUBREL / PUBCOMP)
 * ========================================================================= */

int test7(struct Options options)
{
	MQTTSNPacket_connackData connack;
	MQTTSN_topic topic;
	unsigned char buf[BUF_SIZE];
	unsigned char rbuf[BUF_SIZE];
	int32_t slen, rlen;
	uint16_t packetid = 1;
	uint8_t  acktype = 0;
	uint16_t ret_packetid = 0;
	uint8_t  ret_returncode = 0xFF;
	int rc = 0;
	int ptype;

	fprintf(xml, "<testcase classname=\"test2\" name=\"publish-qos2-handshake\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 7 - PUBLISH QoS 2 / PUBREC / PUBREL / PUBCOMP");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test7", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	memset(&topic, 0, sizeof(topic));
	topic.type            = MQTTSN_TOPIC_TYPE_NAME;
	topic.alt.string.data = TEST_TOPIC;
	topic.alt.string.len  = TEST_TOPIC_LEN;

	slen = MQTTSNSerialize_publish(buf, (int32_t)sizeof(buf),
	           0, 2, 0, packetid,  /* dup=0, qos=2, retained=0, packetid=1 */
	           &topic,
	           (uint8_t*)TEST_PAYLOAD, TEST_PAYLOAD_LEN);
	assert("PUBLISH QoS 2 serialized", slen > 0, "slen was %d\n", (int)slen);
	if (slen <= 0) goto disconnect;

	MyLog(LOGA_DEBUG, "Sending PUBLISH QoS 2 (packetid=%u)", packetid);
	rc = transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
	assert("PUBLISH QoS 2 sent", rc == 0, "rc was %d\n", rc);
	if (rc != 0) goto disconnect;

	/* step 1: receive PUBREC */
	rlen = transport_getdata(rbuf, (int)sizeof(rbuf));
	assert("PUBREC data received", rlen > 0, "rlen was %d\n", rlen);
	if (rlen <= 0) goto disconnect;

	ptype = packet_type_in_buf(rbuf, rlen);
	assert("PUBREC packet type", ptype == MQTTSN_PUBREC,
	       "packet type was 0x%02X\n", ptype);

	rc = MQTTSNDeserialize_ack(&acktype, &ret_packetid, &ret_returncode, rbuf, rlen);
	assert("PUBREC deserialized", rc == 1, "rc was %d\n", rc);
	assert("PUBREC acktype", acktype == MQTTSN_PUBREC,
	       "acktype was 0x%02X\n", acktype);
	assert("PUBREC packetid matches",
	       ret_packetid == packetid, "packetid was %u\n", ret_packetid);
	assert("PUBREC return code success",
	       ret_returncode == MQTT_SN_RC_SUCCESS,
	       "return code was 0x%02X\n", ret_returncode);
	MyLog(LOGA_DEBUG, "PUBREC received (packetid=%u)", ret_packetid);

	/* step 2: send PUBREL */
	slen = MQTTSNSerialize_pubrel(buf, (int32_t)sizeof(buf),
	                               packetid, MQTT_SN_RC_SUCCESS);
	assert("PUBREL serialized", slen > 0, "slen was %d\n", (int)slen);
	if (slen <= 0) goto disconnect;

	rc = transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
	assert("PUBREL sent", rc == 0, "rc was %d\n", rc);
	MyLog(LOGA_DEBUG, "PUBREL sent (packetid=%u)", packetid);

	/* step 3: receive PUBCOMP */
	rlen = transport_getdata(rbuf, (int)sizeof(rbuf));
	assert("PUBCOMP data received", rlen > 0, "rlen was %d\n", rlen);
	if (rlen <= 0) goto disconnect;

	ptype = packet_type_in_buf(rbuf, rlen);
	assert("PUBCOMP packet type", ptype == MQTTSN_PUBCOMP,
	       "packet type was 0x%02X\n", ptype);

	rc = MQTTSNDeserialize_ack(&acktype, &ret_packetid, &ret_returncode, rbuf, rlen);
	assert("PUBCOMP deserialized", rc == 1, "rc was %d\n", rc);
	assert("PUBCOMP acktype", acktype == MQTTSN_PUBCOMP,
	       "acktype was 0x%02X\n", acktype);
	assert("PUBCOMP packetid matches",
	       ret_packetid == packetid, "packetid was %u\n", ret_packetid);
	MyLog(LOGA_DEBUG, "PUBCOMP received (packetid=%u)", ret_packetid);

disconnect:
	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST7: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test8 — CONNECT + SUBSCRIBE + PUBLISH QoS 1 + receive PUBLISH echo
 *
 * Subscribes to TEST_TOPIC with no_local=0, then publishes to the same topic.
 * The gateway should deliver the PUBLISH back to the same client.
 * After sending PUBLISH (packetid=2) we expect to receive:
 *   – PUBACK for our PUBLISH (packetid=2)
 *   – PUBLISH from the gateway (with the session alias from SUBACK)
 * The two may arrive in either order.
 * ========================================================================= */

int test8(struct Options options)
{
	MQTTSNPacket_connackData connack;
	MQTTSN_topic pub_topic, recv_topic;
	unsigned char buf[BUF_SIZE];
	unsigned char rbuf[BUF_SIZE];
	int32_t slen, rlen;
	uint16_t sub_packetid = 1;
	uint16_t pub_packetid = 2;
	uint16_t sub_topicid  = 0;
	uint8_t  dup = 0, retained = 0;
	int32_t  recv_qos = 0;
	uint16_t recv_packetid = 0;
	uint8_t* payload = NULL;
	int32_t  payloadlen = 0;
	uint16_t ack_packetid = 0;
	uint8_t  ack_returncode = 0;
	int got_puback = 0, got_publish = 0;
	int rc = 0;
	int ptype;
	int suback_rc;

	fprintf(xml, "<testcase classname=\"test2\" name=\"subscribe-receive-publish\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	      "Starting test 8 - SUBSCRIBE + PUBLISH QoS 1 + receive PUBLISH echo");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test8", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	/* subscribe with no_local=0 so we receive our own published messages */
	suback_rc = do_subscribe(options, TEST_TOPIC, 1, sub_packetid, &sub_topicid);
	assert("SUBACK received", suback_rc >= 0, "suback_rc was %d\n", suback_rc);
	assert("SUBACK success",
	       suback_rc == MQTT_SN_RC_GRANTED_QOS_0 ||
	       suback_rc == MQTT_SN_RC_GRANTED_QOS_1,
	       "suback reason code was 0x%02X\n", suback_rc);
	if (suback_rc < 0) goto disconnect;

	/* publish QoS 1 to the same topic using full topic name */
	memset(&pub_topic, 0, sizeof(pub_topic));
	pub_topic.type            = MQTTSN_TOPIC_TYPE_NAME;
	pub_topic.alt.string.data = TEST_TOPIC;
	pub_topic.alt.string.len  = TEST_TOPIC_LEN;

	slen = MQTTSNSerialize_publish(buf, (int32_t)sizeof(buf),
	           0, 1, 0, pub_packetid,
	           &pub_topic,
	           (uint8_t*)TEST_PAYLOAD, TEST_PAYLOAD_LEN);
	assert("PUBLISH QoS 1 serialized", slen > 0, "slen was %d\n", (int)slen);
	if (slen <= 0) goto disconnect;

	MyLog(LOGA_DEBUG, "Sending PUBLISH QoS 1 (packetid=%u)", pub_packetid);
	rc = transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
	assert("PUBLISH QoS 1 sent", rc == 0, "rc was %d\n", rc);
	if (rc != 0) goto disconnect;

	/* receive packets until we have both PUBACK and the echoed PUBLISH,
	   or we time out (RECV_TIMEOUT_S per receive attempt, max 4 attempts) */
	for (int i = 0; i < 4 && (!got_puback || !got_publish); i++)
	{
		rlen = transport_getdata(rbuf, (int)sizeof(rbuf));
		if (rlen <= 0)
		{
			MyLog(LOGA_DEBUG, "No more data (rlen=%d)", rlen);
			break;
		}

		ptype = packet_type_in_buf(rbuf, rlen);
		MyLog(LOGA_DEBUG, "Received packet type 0x%02X (%d bytes)", ptype, rlen);

		if (ptype == MQTTSN_PUBACK && !got_puback)
		{
			rc = MQTTSNDeserialize_puback(&ack_packetid, &ack_returncode,
			                              rbuf, rlen);
			assert("PUBACK deserialized", rc == 1, "rc was %d\n", rc);
			assert("PUBACK packetid matches pub_packetid",
			       ack_packetid == pub_packetid,
			       "packetid was %u\n", ack_packetid);
			MyLog(LOGA_DEBUG, "PUBACK received (packetid=%u returncode=0x%02X)",
			      ack_packetid, ack_returncode);
			got_puback = 1;
		}
		else if (ptype == MQTTSN_PUBLISH && !got_publish)
		{
			memset(&recv_topic, 0, sizeof(recv_topic));
			rc = MQTTSNDeserialize_publish(&dup, &recv_qos, &retained,
			                               &recv_packetid, &recv_topic,
			                               &payload, &payloadlen,
			                               rbuf, rlen);
			assert("received PUBLISH deserialized", rc == 1, "rc was %d\n", rc);

			assert("received payload matches",
			       payloadlen == TEST_PAYLOAD_LEN &&
			       memcmp(payload, TEST_PAYLOAD, (size_t)payloadlen) == 0,
			       "payload was different: len=%d\n", payloadlen);

			MyLog(LOGA_DEBUG,
			      "Received PUBLISH (qos=%d packetid=%u payloadlen=%d)",
			      (int)recv_qos, recv_packetid, payloadlen);
			got_publish = 1;

			/* send PUBACK for QoS 1 PUBLISH from the server */
			if (recv_qos == 1)
			{
				slen = MQTTSNSerialize_puback(buf, (int32_t)sizeof(buf),
				                              recv_packetid, MQTT_SN_RC_SUCCESS);
				if (slen > 0)
					transport_sendPacketBuffer(options.host, options.port,
					                           buf, (int)slen);
				MyLog(LOGA_DEBUG, "Sent PUBACK for received PUBLISH (packetid=%u)",
				      recv_packetid);
			}
		}
	}

	assert("PUBACK for our PUBLISH was received", got_puback,
	       "PUBACK not received\n", 0);
	assert("PUBLISH echo from gateway was received", got_publish,
	       "PUBLISH echo not received (no_local=0 required)\n", 0);

disconnect:
	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST8: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test9 — CONNECT + PINGREQ / PINGRESP
 *
 * PINGREQ now carries a Packet Identifier (§3.11.2).
 * PINGRESP echoes that Packet Identifier (§3.12.2) and optionally includes
 * an Application Messages Remaining field (§3.12.3).
 * ========================================================================= */

int test9(struct Options options)
{
	MQTTSNPacket_connackData connack;
	unsigned char buf[BUF_SIZE];
	unsigned char rbuf[BUF_SIZE];
	int32_t slen, rlen;
	uint16_t ping_packetid      = 0x1234;
	uint16_t ret_packetid       = 0;
	int      messages_remaining = 0;
	int rc = 0;
	int ptype;

	fprintf(xml, "<testcase classname=\"test2\" name=\"pingreq-pingresp\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 9 - PINGREQ / PINGRESP");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test9", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	/* serialize PINGREQ with a non-trivial Packet Identifier */
	slen = MQTTSNSerialize_pingreq(buf, (int32_t)sizeof(buf), ping_packetid);
	assert("PINGREQ serialized", slen > 0, "slen was %d\n", (int)slen);
	/* PINGREQ wire length: length(1) + type(1) + packetid(2) = 4 bytes */
	assert("PINGREQ wire length is 4", slen == 4, "slen was %d\n", (int)slen);
	if (slen <= 0) goto disconnect;

	MyLog(LOGA_DEBUG, "Sending PINGREQ (packetid=0x%04X)", ping_packetid);
	rc = transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
	assert("PINGREQ sent", rc == 0, "rc was %d\n", rc);
	if (rc != 0) goto disconnect;

	rlen = transport_getdata(rbuf, (int)sizeof(rbuf));
	assert("PINGRESP data received", rlen > 0, "rlen was %d\n", rlen);
	if (rlen <= 0) goto disconnect;

	ptype = packet_type_in_buf(rbuf, rlen);
	assert("PINGRESP packet type", ptype == MQTTSN_PINGRESP,
	       "packet type was 0x%02X\n", ptype);

	rc = MQTTSNDeserialize_pingresp(&ret_packetid, &messages_remaining,
	                                 rbuf, rlen);
	assert("PINGRESP deserialized", rc == 1, "rc was %d\n", rc);

	/* Packet Identifier in PINGRESP MUST match the one we sent */
	assert1("PINGRESP packetid echoes PINGREQ packetid",
	       ret_packetid == ping_packetid,
	       "packetid was 0x%04X, expected 0x%04X\n",
	       ret_packetid, ping_packetid);

	/* Application Messages Remaining is optional — just log it */
	if (messages_remaining >= 0)
		MyLog(LOGA_DEBUG,
		      "PINGRESP received: packetid=0x%04X messages_remaining=%d",
		      ret_packetid, messages_remaining);
	else
		MyLog(LOGA_DEBUG,
		      "PINGRESP received: packetid=0x%04X (AMR field absent)",
		      ret_packetid);

disconnect:
	do_disconnect(options, MQTT_SN_RC_SUCCESS);

exit:
	MyLog(LOGA_INFO, "TEST9: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test10 — CONNECT + DISCONNECT (with reason code)
 * ========================================================================= */

int test10(struct Options options)
{
	MQTTSNPacket_connackData connack;
	unsigned char buf[BUF_SIZE];
	int32_t slen;
	int rc = 0;

	fprintf(xml, "<testcase classname=\"test2\" name=\"disconnect-with-reason-code\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 10 - CONNECT + DISCONNECT (with reason code)");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect(options, "mqttsn2-test10", 1, 0, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto close;

	/* DISCONNECT with Normal Disconnection (reason code 0x00, omitted from wire) */
	slen = MQTTSNSerialize_disconnect(buf, (int32_t)sizeof(buf), MQTT_SN_RC_SUCCESS);
	assert("DISCONNECT serialized", slen > 0, "slen was %d\n", (int)slen);
	/* Normal DISCONNECT: length(1) + type(1) + flags(1) = 3 bytes; no reason code */
	assert("DISCONNECT wire length is 3", slen == 3, "slen was %d\n", (int)slen);

	if (slen > 0)
	{
		rc = transport_sendPacketBuffer(options.host, options.port, buf, (int)slen);
		assert("DISCONNECT sent", rc == 0, "rc was %d\n", rc);
		MyLog(LOGA_DEBUG, "DISCONNECT sent (reason=0x00, wire length=%d)", (int)slen);
	}

close:
	transport_close();

exit:
	MyLog(LOGA_INFO, "TEST10: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * main
 * ========================================================================= */

int main(int argc, char** argv)
{
	int rc = 0;
	int (*testfuncs[])(struct Options) = {
		NULL,    /* 0: placeholder */
		test1,   /* 1: CONNECT/CONNACK minimal */
		test2,   /* 2: CONNECT/CONNACK with Session Expiry Interval */
		test3,   /* 3: SUBSCRIBE QoS 0 / SUBACK */
		test4,   /* 4: SUBSCRIBE QoS 1 / SUBACK */
		test5,   /* 5: PUBLISH QoS 0 (fire and forget) */
		test6,   /* 6: PUBLISH QoS 1 / PUBACK */
		test7,   /* 7: PUBLISH QoS 2 / PUBREC / PUBREL / PUBCOMP */
		test8,   /* 8: SUBSCRIBE + PUBLISH QoS 1 + receive echo */
		test9,   /* 9: PINGREQ / PINGRESP */
		test10,  /* 10: DISCONNECT with reason code */
	};

	xml = fopen("TEST-test2.xml", "w");
	fprintf(xml, "<testsuite name=\"test2\" tests=\"%d\">\n",
	        (int)(ARRAY_SIZE(testfuncs) - 1));

	getopts(argc, argv);
	MyLog(LOGA_INFO, "MQTT-SN 2.0 network test suite: host=%s port=%d",
	      options.host, options.port);

	if (options.test_no == 0)
	{
		/* run all tests */
		for (options.test_no = 1;
		     options.test_no < (int)ARRAY_SIZE(testfuncs);
		     ++options.test_no)
			rc += testfuncs[options.test_no](options);
	}
	else
		rc = testfuncs[options.test_no](options);

	if (rc == 0)
		MyLog(LOGA_INFO, "verdict pass");
	else
		MyLog(LOGA_INFO, "verdict fail");

	fprintf(xml, "</testsuite>\n");
	fclose(xml);
	return rc;
}
