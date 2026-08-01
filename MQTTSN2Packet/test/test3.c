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
 * Network test suite for MQTT-SN 2.0 Protection Encapsulation (Section 3.17).
 *
 * Tests connect to an MQTT-SN 2.0 gateway on localhost:1883 over UDP and
 * exercise MQTTSNSerialize_protection / MQTTSNDeserialize_protection from
 * MQTTSNProtection.c.  Each MQTT-SN packet is wrapped in a PROTECTION
 * envelope; the helper wrap_with_protection() builds the AAD and computes
 * HMAC-SHA256 using OpenSSL.
 *
 * AAD definition (Sections 3.17.3 and 3.17.9):
 *   Section 3.17.9: "The Authentication Tag authenticates ALL the preceding fields."
 *   Section 3.17.3: nonce/AAD input is "the sequence Byte 1 to Byte 17+C+M".
 *   Byte 1 is the Length field — so the AAD starts from the very first byte
 *   of the packet and runs through to (but not including) the Authentication Tag:
 *   [Length field][0xFF Type][Flags][Scheme][Sender ID][Random]
 *   [Crypto Material, if present][Counter, if present][Protected MQTT-SN Packet]
 *
 * Server configuration required:
 *   The gateway must be configured with:
 *     Sender Identifier: DE AD BE EF CA FE BA BE
 *     Shared key (HMAC-SHA256, 32 bytes):
 *       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
 *       10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
 *
 * Build:
 *   cc -o test3 test3.c MQTTSNPacket2.c MQTTSNConnectClient.c \
 *      MQTTSNSubscribeClient.c MQTTSNSerializePublish.c \
 *      MQTTSNDeserializePublish.c MQTTSNProtection.c transport.c \
 *      -lssl -lcrypto -DNOSTACKTRACE -DNDEBUG -I. -std=c11
 *
 * Coverage:
 *   test1  PROTECTION(CONNECT) HMAC-SHA256 nominal 32-byte tag
 *   test2  PROTECTION(CONNECT) HMAC-SHA256 truncated 64-bit tag
 *   test3  PROTECTION(CONNECT) with 2-byte monotonic counter
 *   test4  PROTECTION(CONNECT) with 12-byte cryptographic material
 *   test5  PROTECTION(CONNECT) with corrupted auth tag → rejected
 *   test6  PROTECTION(CONNECT) + PROTECTION(SUBSCRIBE) → SUBACK
 *   test7  PROTECTION(CONNECT) + PROTECTION(PUBLISH QoS 1) → PUBACK
 *   test8  PROTECTION(CONNECT) + PROTECTION(PUBLISH QoS 2) full handshake
 *   test9  PROTECTION(CONNECT) + PROTECTION(DISCONNECT)
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"
#include "MQTTSNSubscribe.h"
#include "MQTTSNPublish.h"
#include "MQTTSNProtection.h"
#include "transport.h"

#include <openssl/evp.h>
#include <openssl/params.h>

#include <stdint.h>
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
#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


/* -------------------------------------------------------------------------
 * Program options
 * ------------------------------------------------------------------------- */

struct Options
{
	char* host;
	int   port;
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
	printf("Usage: test3 [--host h] [--port p] [--test_no n] [--verbose]\n");
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
			else usage();
		}
		else if (strcmp(argv[count], "--host") == 0)
		{
			if (++count < argc) options.host = argv[count];
			else usage();
		}
		else if (strcmp(argv[count], "--port") == 0)
		{
			if (++count < argc) options.port = atoi(argv[count]);
			else usage();
		}
		else if (strcmp(argv[count], "--verbose") == 0)
			options.verbose = 1;
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
#define START_TIME_TYPE DWORD
static DWORD start_time = 0;
START_TIME_TYPE start_clock(void) { return GetTickCount(); }
long elapsed(START_TIME_TYPE st) { return GetTickCount() - st; }
#else
#define START_TIME_TYPE struct timeval
START_TIME_TYPE start_clock(void)
{
	struct timeval t; gettimeofday(&t, NULL); return t;
}
long elapsed(START_TIME_TYPE st)
{
	struct timeval now, res;
	gettimeofday(&now, NULL);
	timersub(&now, &st, &res);
	return res.tv_sec * 1000 + res.tv_usec / 1000;
}
#endif


/* -------------------------------------------------------------------------
 * Assertions
 * ------------------------------------------------------------------------- */

#define assert(a, b, c, d)     myassert(__FILE__, __LINE__, a, b, c, d)

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
		MyLog(LOGA_DEBUG,
		      "Assertion succeeded, file %s, line %d, description: %s",
		      filename, lineno, description);
}


/* =========================================================================
 * Protection test configuration
 *
 * The gateway must be pre-configured with the key below, indexed by the
 * sender identifier below.
 * ========================================================================= */

/* 32-byte (256-bit) pre-shared key for HMAC-SHA256 */
static const uint8_t test_key[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

/* 8-byte sender identifier (e.g. a device MAC address) */
static const uint8_t test_sender_id[MQTTSN_PROTECTION_SENDER_ID_LEN] = {
	0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE
};

/* Test topic */
#define TEST_TOPIC       "test/mqttsn2-prot"
#define TEST_TOPIC_LEN   18
#define TEST_PAYLOAD     "protected message"
#define TEST_PAYLOAD_LEN 17


/* =========================================================================
 * Transport helpers (same pattern as test2.c)
 * ========================================================================= */

#define BUF_SIZE 1024
#define RECV_TIMEOUT_S 5

static int open_transport_with_timeout(void)
{
	int sock = transport_open();
	if (sock < 0) return sock;
#if defined(WIN32)
	DWORD tv_ms = RECV_TIMEOUT_S * 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
	           (const char*)&tv_ms, sizeof(tv_ms));
#else
	struct timeval tv = { RECV_TIMEOUT_S, 0 };
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
	return sock;
}

static int packet_type_in_buf(unsigned char* buf, int len)
{
	int mylen = 0;
	int lenlen = MQTTSNPacket_decode(buf, len, &mylen);
	if (lenlen < 0 || len < lenlen + 1) return -1;
	return (int)(uint8_t)buf[lenlen];
}


/* =========================================================================
 * HMAC-SHA256 computation (OpenSSL)
 * ========================================================================= */

/*
 * Compute HMAC-SHA256 of (data, data_len) with (key, key_len).
 * Output written to tag[0..31].  tag must be at least EVP_MAX_MD_SIZE bytes.
 * Returns the tag length (32 for HMAC-SHA256) or 0 on error.
 *
 * Uses EVP_Q_mac(), the single-shot EVP_MAC convenience function recommended
 * by OpenSSL 3.x in place of the deprecated HMAC() function.
 */
static uint32_t compute_hmac_sha256(const uint8_t* key,    size_t key_len,
                                    const uint8_t* data,   size_t data_len,
                                    uint8_t*       tag)
{
	size_t tag_len = 0;
	if (EVP_Q_mac(NULL,         /* default library context */
	              "HMAC",       /* MAC algorithm */
	              NULL,         /* no property query */
	              "SHA256",     /* underlying digest */
	              NULL,         /* no extra OSSL_PARAM */
	              key, key_len,
	              data, data_len,
	              tag, EVP_MAX_MD_SIZE, &tag_len) == NULL)
		return 0;
	return (uint32_t)tag_len;
}


/* =========================================================================
 * Protection packet construction
 *
 * wrap_with_protection() assembles the AAD, computes HMAC-SHA256, then
 * calls MQTTSNSerialize_protection to produce the final wire packet.
 *
 * AAD (Section 3.17.9) — "its content authenticates ALL the preceding fields":
 * ALL fields that precede the Authentication Tag, starting from Byte 1
 * (Section 3.17.3: "the sequence Byte 1 to Byte 17+C+M"):
 *
 *   [Length field (1 byte for total < 255, else 3 bytes)]  ← Byte 1
 *   [Packet Type 0xFF (1 byte)]
 *   [Protection Flags (1)]
 *   [Protection Scheme (1)]
 *   [Sender Identifier (8)]
 *   [Random (4)]
 *   [Cryptographic Material (0/2/4/12, from Crypto Length field)]
 *   [Monotonic Counter (0/2/4, from Counter Length field)]
 *   [Protected MQTT-SN Packet (variable)]
 *
 * The length field encodes the TOTAL packet length including the auth tag,
 * so the tag length must be resolved before the length field can be written.
 *
 * Parameters:
 *   inner_buf, inner_len   the already-serialized inner MQTT-SN packet
 *   scheme                 one of the MQTTSN_protectionSchemes enum values
 *   auth_tag_len_field     bits 7-4 of Protection Flags
 *                            MQTTSN_PROTECTION_AUTHTAG_NOMINAL (0x1) = 32 bytes
 *                            0x4 = 8 bytes (4 * 16 bits)
 *   crypto_len_field       bits 3-2 of Protection Flags
 *   crypto_material        pointer to crypto material bytes (may be NULL)
 *   counter_len_field      bits 1-0 of Protection Flags
 *   counter                pointer to counter bytes (may be NULL)
 *   random                 4-byte random field
 *   out_buf, out_buflen    output buffer
 * Returns serialized length, or <= 0 on error.
 * ========================================================================= */

static int32_t wrap_with_protection(
        const uint8_t* inner_buf, uint16_t inner_len,
        uint8_t scheme,
        uint8_t auth_tag_len_field,
        uint8_t crypto_len_field,    const uint8_t* crypto_material,
        uint8_t counter_len_field,   const uint8_t* counter,
        const uint8_t random[MQTTSN_PROTECTION_RANDOM_LEN],
        uint8_t* out_buf, int32_t out_buflen)
{
	MQTTSNPacket_protectionData pd;
	uint8_t  aad[BUF_SIZE];
	uint8_t  full_tag[EVP_MAX_MD_SIZE];
	uint16_t aad_len = 0;
	uint32_t hmac_len;
	uint16_t tag_len;
	uint8_t  crypto_bytes;
	uint8_t  counter_bytes;
	int8_t   lenlen;
	uint16_t total_wire_len;

	memset(&pd, 0, sizeof(pd));
	pd.flags.bits.authTagLen = auth_tag_len_field;
	pd.flags.bits.cryptoLen  = crypto_len_field;
	pd.flags.bits.counterLen = counter_len_field;
	pd.scheme = scheme;

	memcpy(pd.senderid, test_sender_id, MQTTSN_PROTECTION_SENDER_ID_LEN);
	memcpy(pd.random,   random, MQTTSN_PROTECTION_RANDOM_LEN);

	crypto_bytes  = MQTTSNProtection_cryptoMaterialLen(crypto_len_field);
	counter_bytes = MQTTSNProtection_counterLen(counter_len_field);

	if (crypto_bytes > 0 && crypto_material != NULL)
	{
		pd.cryptoMaterial.data = (unsigned char*)crypto_material;
		pd.cryptoMaterial.len  = crypto_bytes;
	}
	if (counter_bytes > 0 && counter != NULL)
	{
		pd.monotonicCounter.data = (unsigned char*)counter;
		pd.monotonicCounter.len  = counter_bytes;
	}

	pd.protectedPacket.data = (unsigned char*)inner_buf;
	pd.protectedPacket.len  = inner_len;

	/* Resolve tag length before building the AAD, because the correct total
	 * wire length (and therefore the length field bytes that open the AAD)
	 * depends on the tag length. */
	if (auth_tag_len_field == MQTTSN_PROTECTION_AUTHTAG_NOMINAL ||
	    auth_tag_len_field == MQTTSN_PROTECTION_AUTHTAG_PROVIDER)
		tag_len = 32u;           /* HMAC-SHA256 nominal/full output = 32 bytes */
	else if (auth_tag_len_field >= 4u)
		tag_len = (uint16_t)(auth_tag_len_field * 2u); /* field * 16 bits */
	else
		tag_len = 32u;

	/* body = type(1)+flags(1)+scheme(1)+senderid(8)+random(4)+C+M+inner+tag */
	uint16_t body_len = (uint16_t)(15u + crypto_bytes + counter_bytes
	                                    + inner_len + tag_len);
	total_wire_len = MQTTSNPacket_len(body_len);

	/* ── Build AAD — starts at Byte 1 (length field) per Section 3.17.3 ── */

	/* Length field: encodes the total wire length of the FINAL packet */
	lenlen = MQTTSNPacket_encode(aad, total_wire_len);
	aad_len = (uint16_t)lenlen;

	/* Packet type byte (0xFF = MQTTSN_PROTECTION) */
	aad[aad_len++] = MQTTSN_PROTECTION;

	aad[aad_len++] = pd.flags.all;
	aad[aad_len++] = pd.scheme;

	memcpy(&aad[aad_len], pd.senderid, MQTTSN_PROTECTION_SENDER_ID_LEN);
	aad_len += MQTTSN_PROTECTION_SENDER_ID_LEN;

	memcpy(&aad[aad_len], pd.random, MQTTSN_PROTECTION_RANDOM_LEN);
	aad_len += MQTTSN_PROTECTION_RANDOM_LEN;

	if (crypto_bytes > 0 && crypto_material != NULL)
	{
		memcpy(&aad[aad_len], crypto_material, crypto_bytes);
		aad_len += crypto_bytes;
	}
	if (counter_bytes > 0 && counter != NULL)
	{
		memcpy(&aad[aad_len], counter, counter_bytes);
		aad_len += counter_bytes;
	}

	/* Protected MQTT-SN Packet is part of the AAD for authentication-only
	 * schemes (Section 3.17.9: "authenticates ALL the preceding fields") */
	memcpy(&aad[aad_len], inner_buf, inner_len);
	aad_len += inner_len;

	/* ── Compute HMAC-SHA256 ──────────────────────────────────────────── */
	hmac_len = compute_hmac_sha256(test_key, sizeof(test_key),
	                               aad, aad_len, full_tag);
	if (hmac_len == 0)
		return -1;

	/* tag_len was already resolved before the HMAC to allow the correct
	 * total wire length to be encoded in the AAD length field. */

	pd.authTag.data = full_tag;
	pd.authTag.len  = tag_len;

	return MQTTSNSerialize_protection(out_buf, out_buflen, &pd);
}


/* =========================================================================
 * receive_protected — receive one PROTECTION-wrapped packet, verify it is
 * wrapped, deserialize the envelope, and expose the inner packet.
 *
 * Enforces [MQTT-SN-3.17-2] and [MQTT-SN-3.17-3]: once a shared key is in
 * use, ALL packets exchanged in that session MUST be Protection-wrapped.
 * An unwrapped packet from the server is therefore a protocol violation and
 * is treated as an error.
 *
 * Returns the inner packet type byte, or -1 on error/timeout/violation.
 * Sets *rlen_out to the effective byte count of the inner packet in rbuf.
 * Sets *pd_out to the deserialized PROTECTION envelope.
 * ========================================================================= */

static int receive_protected(unsigned char* rbuf, int rbuflen,
                             int* rlen_out,
                             MQTTSNPacket_protectionData* pd_out)
{
	int rlen = transport_getdata(rbuf, rbuflen);
	if (rlen <= 0) { *rlen_out = rlen; return -1; }

	int ptype = packet_type_in_buf(rbuf, rlen);

	if (ptype != MQTTSN_PROTECTION)
	{
		/* Server sent an unprotected packet — protocol violation.
		 * [MQTT-SN-3.17-2]: the server MUST protect ALL packets when a
		 * shared key is available for this client. */
		MyLog(LOGA_INFO,
		      "PROTOCOL VIOLATION: received unprotected packet type 0x%02X "
		      "— server MUST wrap all packets in PROTECTION "
		      "[MQTT-SN-3.17-2]", ptype);
		*rlen_out = 0;
		return -1;
	}

	if (pd_out != NULL)
		memset(pd_out, 0, sizeof(*pd_out));

	MQTTSNPacket_protectionData local_pd;
	MQTTSNPacket_protectionData* pd = (pd_out != NULL) ? pd_out : &local_pd;
	memset(pd, 0, sizeof(*pd));

	if (MQTTSNDeserialize_protection(pd, rbuf, rlen) != 1)
	{
		MyLog(LOGA_DEBUG, "MQTTSNDeserialize_protection failed");
		*rlen_out = 0;
		return -1;
	}
	MyLog(LOGA_DEBUG,
	      "Received PROTECTION packet (scheme=0x%02X senderid=%02X%02X%02X%02X... "
	      "inner_len=%u auth_tag_len=%u)",
	      pd->scheme,
	      pd->senderid[0], pd->senderid[1],
	      pd->senderid[2], pd->senderid[3],
	      pd->protectedPacket.len,
	      pd->authTag.len);

	/* Copy the inner packet into rbuf so the caller can deserialize it */
	uint16_t inner_len = pd->protectedPacket.len;
	if ((int)inner_len > rbuflen)
	{
		*rlen_out = 0;
		return -1;
	}
	memmove(rbuf, pd->protectedPacket.data, inner_len);
	*rlen_out = (int)inner_len;
	return packet_type_in_buf(rbuf, *rlen_out);
}


/* =========================================================================
 * Protocol helpers
 * ========================================================================= */

/*
 * Wrap a pre-serialized inner packet, send it, then call receive_protected()
 * which enforces [MQTT-SN-3.17-2]: the server response MUST also be wrapped.
 * Returns the inner response packet type, or -1 on failure/violation.
 * The inner response bytes end up in rbuf[0..*rlen_out-1].
 */
static int send_protected_receive(
        struct Options options,
        const uint8_t* inner_buf, uint16_t inner_len,
        uint8_t scheme, uint8_t auth_tag_len_field,
        uint8_t crypto_len_field,  const uint8_t* crypto_material,
        uint8_t counter_len_field, const uint8_t* counter,
        const uint8_t random[MQTTSN_PROTECTION_RANDOM_LEN],
        unsigned char* rbuf, int rbuflen, int* rlen_out,
        MQTTSNPacket_protectionData* pd_out)
{
	uint8_t prot_buf[BUF_SIZE];
	int32_t prot_len;

	prot_len = wrap_with_protection(inner_buf, inner_len,
	               scheme, auth_tag_len_field,
	               crypto_len_field, crypto_material,
	               counter_len_field, counter,
	               random, prot_buf, (int32_t)sizeof(prot_buf));
	if (prot_len <= 0) return -1;

	MyLog(LOGA_DEBUG, "Sending PROTECTION packet (%d bytes, inner_len=%u, "
	      "scheme=0x%02X authTagLen_field=0x%01X tag_bytes=%u)",
	      (int)prot_len, inner_len, scheme, auth_tag_len_field,
	      (auth_tag_len_field == MQTTSN_PROTECTION_AUTHTAG_NOMINAL) ? 32u :
	      (auth_tag_len_field >= 4u) ? (unsigned)(auth_tag_len_field * 2u) : 32u);

	if (transport_sendPacketBuffer(options.host, options.port,
	                                prot_buf, (int)prot_len) != 0)
		return -1;

	return receive_protected(rbuf, rbuflen, rlen_out, pd_out);
}

/*
 * Serialize CONNECT, wrap it in PROTECTION, send it, receive CONNACK
 * (protected or plain).  Fills *connack_out.  Returns 1 on success.
 */
static int do_connect_protected(
        struct Options options,
        const char* clientid, int clean_start,
        uint8_t scheme, uint8_t auth_tag_len_field,
        uint8_t crypto_len_field,  const uint8_t* crypto_material,
        uint8_t counter_len_field, const uint8_t* counter,
        const uint8_t random[MQTTSN_PROTECTION_RANDOM_LEN],
        MQTTSNPacket_connackData* connack_out)
{
	MQTTSNPacket_connectData conn = MQTTSNPacket_connectData_initializer;
	uint8_t  inner_buf[BUF_SIZE];
	uint8_t  rbuf[BUF_SIZE];
	int32_t  inner_len;
	int      rlen = 0;
	int      ptype;

	conn.clientID.data           = (char*)clientid;
	conn.clientID.len            = (uint16_t)strlen(clientid);
	conn.keepAlive               = 30;
	conn.maxPacketSize           = BUF_SIZE;
	conn.packetId                = 1;
	conn.flags.bits.cleanStart   = (clean_start != 0);

	inner_len = MQTTSNSerialize_connect(inner_buf, (int32_t)sizeof(inner_buf),
	                                    &conn);
	if (inner_len <= 0) return 0;

	ptype = send_protected_receive(options,
	            inner_buf, (uint16_t)inner_len,
	            scheme, auth_tag_len_field,
	            crypto_len_field, crypto_material,
	            counter_len_field, counter, random,
	            rbuf, (int)sizeof(rbuf), &rlen, NULL);

	if (ptype != MQTTSN_CONNACK)
	{
		MyLog(LOGA_DEBUG, "Expected CONNACK (0x%02X) got 0x%02X",
		      MQTTSN_CONNACK, ptype);
		return 0;
	}

	memset(connack_out, 0, sizeof(*connack_out));
	return MQTTSNDeserialize_connack(connack_out, rbuf, rlen);
}

/*
 * Wrap DISCONNECT in PROTECTION and send it; close transport.
 */
static void do_disconnect_protected(
        struct Options options,
        uint8_t scheme,
        const uint8_t random[MQTTSN_PROTECTION_RANDOM_LEN])
{
	uint8_t inner_buf[8];
	uint8_t prot_buf[BUF_SIZE];
	int32_t inner_len = MQTTSNSerialize_disconnect(inner_buf,
	                        (int32_t)sizeof(inner_buf), MQTT_SN_RC_SUCCESS);
	if (inner_len > 0)
	{
		int32_t prot_len = wrap_with_protection(
		    inner_buf, (uint16_t)inner_len,
		    scheme, MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
		    MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
		    MQTTSN_PROTECTION_COUNTER_NONE, NULL,
		    random, prot_buf, (int32_t)sizeof(prot_buf));
		if (prot_len > 0)
			transport_sendPacketBuffer(options.host, options.port,
			                           prot_buf, (int)prot_len);
	}
	transport_close();
	MyLog(LOGA_DEBUG, "Disconnected (protected)");
}

/* 4-byte random values — unique per test to reduce replay risk */
static const uint8_t random_t1[4] = { 0xAB, 0xCD, 0x00, 0x01 };
static const uint8_t random_t2[4] = { 0xAB, 0xCD, 0x00, 0x02 };
static const uint8_t random_t3[4] = { 0xAB, 0xCD, 0x00, 0x03 };
static const uint8_t random_t4[4] = { 0xAB, 0xCD, 0x00, 0x04 };
static const uint8_t random_t5[4] = { 0xAB, 0xCD, 0x00, 0x05 };
static const uint8_t random_t6[4] = { 0xAB, 0xCD, 0x00, 0x06 };
static const uint8_t random_t7[4] = { 0xAB, 0xCD, 0x00, 0x07 };
static const uint8_t random_t8[4] = { 0xAB, 0xCD, 0x00, 0x08 };
static const uint8_t random_t9[4] = { 0xAB, 0xCD, 0x00, 0x09 };


/* =========================================================================
 * test1 — PROTECTION(CONNECT) HMAC-SHA256, nominal 32-byte auth tag
 * ========================================================================= */

int test1(struct Options options)
{
	MQTTSNPacket_connackData connack;
	int rc = 0;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-connect-nominal-tag\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	    "Starting test 1 - PROTECTION(CONNECT) HMAC-SHA256 nominal 32-byte tag");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect_protected(options, "mqttsn2-prot-t1", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	         MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	         MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	         random_t1, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK reason code success",
	       connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);

	do_disconnect_protected(options, MQTTSN_PROTECTION_HMAC_SHA256, random_t1);

exit:
	MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test2 — PROTECTION(CONNECT) HMAC-SHA256, truncated 64-bit (8-byte) tag
 *
 * Authentication Tag Length field = 0x4 → 4 × 16 bits = 64 bits = 8 bytes
 * ========================================================================= */

int test2(struct Options options)
{
	MQTTSNPacket_connackData connack;
	int rc = 0;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-connect-truncated-tag\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	    "Starting test 2 - PROTECTION(CONNECT) HMAC-SHA256 truncated 64-bit tag");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect_protected(options, "mqttsn2-prot-t2", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         0x4u,  /* 4 * 16 bits = 64 bits = 8 bytes */
	         MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	         MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	         random_t2, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK reason code success",
	       connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);

	do_disconnect_protected(options, MQTTSN_PROTECTION_HMAC_SHA256, random_t2);

exit:
	MyLog(LOGA_INFO, "TEST2: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test3 — PROTECTION(CONNECT) with 2-byte monotonic counter
 * ========================================================================= */

int test3(struct Options options)
{
	/* Counter value: sequence number 1, big-endian */
	static const uint8_t counter[2] = { 0x00, 0x01 };
	MQTTSNPacket_connackData connack;
	int rc = 0;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-connect-counter\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	    "Starting test 3 - PROTECTION(CONNECT) with 2-byte monotonic counter");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect_protected(options, "mqttsn2-prot-t3", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	         MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	         MQTTSN_PROTECTION_COUNTER_16, counter,    /* 2-byte counter */
	         random_t3, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK reason code success",
	       connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);

	do_disconnect_protected(options, MQTTSN_PROTECTION_HMAC_SHA256, random_t3);

exit:
	MyLog(LOGA_INFO, "TEST3: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test4 — PROTECTION(CONNECT) with 12-byte cryptographic material
 *
 * Crypto Material (96-bit, field value 0x3) is included in the AAD.
 * In AEAD schemes it contributes entropy to the nonce; for HMAC schemes it
 * is simply an authenticated field.
 * ========================================================================= */

int test4(struct Options options)
{
	/* 12-byte crypto material: 9 random bytes + 3 zero bytes (CCM example) */
	static const uint8_t crypto[12] = {
		0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6,
		0x01, 0x02, 0x03, 0x00, 0x00, 0x00
	};
	MQTTSNPacket_connackData connack;
	int rc = 0;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-connect-crypto-material\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	    "Starting test 4 - PROTECTION(CONNECT) with 12-byte cryptographic material");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect_protected(options, "mqttsn2-prot-t4", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	         MQTTSN_PROTECTION_CRYPTO_96, crypto,       /* 12-byte crypto material */
	         MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	         random_t4, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK reason code success",
	       connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);

	do_disconnect_protected(options, MQTTSN_PROTECTION_HMAC_SHA256, random_t4);

exit:
	MyLog(LOGA_INFO, "TEST4: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test5 — PROTECTION(CONNECT) with deliberately corrupted auth tag
 *
 * Construct a valid PROTECTION packet, then flip a bit in the auth tag
 * before sending.  The server MUST reject the packet.  The test passes if
 * we receive DISCONNECT (or no response within the timeout), and fails if
 * we receive CONNACK.
 * ========================================================================= */

int test5(struct Options options)
{
	MQTTSNPacket_connectData conn = MQTTSNPacket_connectData_initializer;
	uint8_t  inner_buf[BUF_SIZE];
	uint8_t  prot_buf[BUF_SIZE];
	uint8_t  rbuf[BUF_SIZE];
	int32_t  inner_len, prot_len;
	int rlen = 0, rc = 0, ptype;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-connect-bad-auth-tag\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	    "Starting test 5 - PROTECTION(CONNECT) with corrupted auth tag");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	/* Build the inner CONNECT */
	conn.clientID.data         = "mqttsn2-prot-t5";
	conn.clientID.len          = 15;
	conn.keepAlive             = 30;
	conn.maxPacketSize         = BUF_SIZE;
	conn.packetId              = 1;
	conn.flags.bits.cleanStart = 1;

	inner_len = MQTTSNSerialize_connect(inner_buf, (int32_t)sizeof(inner_buf),
	                                    &conn);
	assert("CONNECT serialized", inner_len > 0, "inner_len was %d\n", (int)inner_len);
	if (inner_len <= 0) goto close;

	/* Build the PROTECTION packet with a valid auth tag */
	prot_len = wrap_with_protection(inner_buf, (uint16_t)inner_len,
	               MQTTSN_PROTECTION_HMAC_SHA256,
	               MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	               MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	               MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	               random_t5, prot_buf, (int32_t)sizeof(prot_buf));
	assert("PROTECTION packet built", prot_len > 0, "prot_len was %d\n", (int)prot_len);
	if (prot_len <= 0) goto close;

	/* Corrupt the last byte of the auth tag (the final byte of the packet) */
	prot_buf[prot_len - 1] ^= 0xFF;
	MyLog(LOGA_DEBUG, "Auth tag corrupted (last byte flipped)");

	rc = transport_sendPacketBuffer(options.host, options.port,
	                                prot_buf, (int)prot_len);
	assert("Corrupted PROTECTION packet sent", rc == 0, "rc was %d\n", rc);
	if (rc != 0) goto close;

	/* Expect rejection: DISCONNECT(Protection Scheme Invalid) or timeout (no response) */
	rlen = transport_getdata(rbuf, (int)sizeof(rbuf));

	if (rlen <= 0)
	{
		/* No response within timeout — server silently dropped the packet */
		MyLog(LOGA_DEBUG, "No response received: server silently rejected packet");
		/* Acceptable: the spec does not mandate a DISCONNECT in this case */
		++tests; /* count one assertion as passed */
	}
	else
	{
		ptype = packet_type_in_buf(rbuf, rlen);
		assert("Not CONNACK with bad auth tag", ptype != MQTTSN_CONNACK,
		       "received CONNACK despite corrupted auth tag\n", 0);
		assert("Response is DISCONNECT", ptype == MQTTSN_DISCONNECT,
		       "received packet type 0x%02X\n", ptype);
		if (ptype == MQTTSN_DISCONNECT)
		{
			MQTTSNPacket_disconnectData discdata;
			rc = MQTTSNDeserialize_disconnect(&discdata, rbuf, rlen);
			assert("DISCONNECT deserialized", rc == 1, "rc was %d\n", rc);
			assert("DISCONNECT reason code is Protection Scheme Invalid",
			       discdata.reasonCode == MQTT_SN_RC_PROTECTION_SCHEME_INVALID,
			       "reason code was 0x%02X\n", discdata.reasonCode);
			MyLog(LOGA_DEBUG, "DISCONNECT received: server rejected bad auth tag (reason=0x%02X)",
			      discdata.reasonCode);
		}
	}

close:
	transport_close();

exit:
	MyLog(LOGA_INFO, "TEST5: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test6 — PROTECTION(CONNECT) + PROTECTION(SUBSCRIBE) / SUBACK
 * ========================================================================= */

int test6(struct Options options)
{
	MQTTSNPacket_connackData connack;
	MQTTSN_topic topic;
	uint8_t  inner_buf[BUF_SIZE];
	uint8_t  rbuf[BUF_SIZE];
	int32_t  inner_len;
	uint16_t sub_topicid         = 0;
	uint8_t  topic_alias_present = 0;
	uint16_t ret_packetid        = 0;
	uint8_t  reason_code         = 0xFF;
	int rlen = 0, rc = 0, ptype;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-subscribe\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 6 - PROTECTION(CONNECT + SUBSCRIBE)");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect_protected(options, "mqttsn2-prot-t6", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	         MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	         MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	         random_t6, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	/* Serialize SUBSCRIBE */
	memset(&topic, 0, sizeof(topic));
	topic.type            = MQTTSN_TOPIC_TYPE_FILTER;
	topic.alt.string.data = TEST_TOPIC;
	topic.alt.string.len  = TEST_TOPIC_LEN;

	inner_len = MQTTSNSerialize_subscribe(inner_buf, (int32_t)sizeof(inner_buf),
	                1, 0, 0, 0, 1, &topic);
	assert("SUBSCRIBE serialized", inner_len > 0,
	       "inner_len was %d\n", (int)inner_len);
	if (inner_len <= 0) goto disconnect;

	/* Wrap and send SUBSCRIBE, receive SUBACK */
	ptype = send_protected_receive(options,
	            inner_buf, (uint16_t)inner_len,
	            MQTTSN_PROTECTION_HMAC_SHA256,
	            MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	            MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	            MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	            random_t6, rbuf, (int)sizeof(rbuf), &rlen, NULL);

	assert("SUBACK received", ptype == MQTTSN_SUBACK,
	       "packet type was 0x%02X\n", ptype);

	if (ptype == MQTTSN_SUBACK)
	{
		rc = MQTTSNDeserialize_suback(&sub_topicid, &topic_alias_present,
		                              &ret_packetid, &reason_code,
		                              rbuf, rlen);
		assert("SUBACK deserialized", rc == 1, "rc was %d\n", rc);
		assert("SUBACK reason code success",
		       reason_code == MQTT_SN_RC_GRANTED_QOS_0 ||
		       reason_code == MQTT_SN_RC_GRANTED_QOS_1,
		       "reason code was 0x%02X\n", reason_code);
		MyLog(LOGA_DEBUG, "SUBACK: reason=0x%02X topicid=%u", reason_code, sub_topicid);
	}

disconnect:
	do_disconnect_protected(options, MQTTSN_PROTECTION_HMAC_SHA256, random_t6);

exit:
	MyLog(LOGA_INFO, "TEST6: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test7 — PROTECTION(CONNECT) + PROTECTION(PUBLISH QoS 1) / PUBACK
 * ========================================================================= */

int test7(struct Options options)
{
	MQTTSNPacket_connackData connack;
	MQTTSN_topic topic;
	uint8_t  inner_buf[BUF_SIZE];
	uint8_t  rbuf[BUF_SIZE];
	int32_t  inner_len;
	uint16_t pub_packetid = 1;
	uint16_t ack_packetid = 0;
	uint8_t  ack_rc       = 0xFF;
	int rlen = 0, rc = 0, ptype;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-publish-qos1\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 7 - PROTECTION(CONNECT + PUBLISH QoS 1)");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect_protected(options, "mqttsn2-prot-t7", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	         MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	         MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	         random_t7, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	/* Serialize PUBLISH QoS 1 */
	memset(&topic, 0, sizeof(topic));
	topic.type            = MQTTSN_TOPIC_TYPE_NAME;
	topic.alt.string.data = TEST_TOPIC;
	topic.alt.string.len  = TEST_TOPIC_LEN;

	inner_len = MQTTSNSerialize_publish(inner_buf, (int32_t)sizeof(inner_buf),
	                0, 1, 0, pub_packetid, &topic,
	                (uint8_t*)TEST_PAYLOAD, TEST_PAYLOAD_LEN);
	assert("PUBLISH QoS 1 serialized", inner_len > 0,
	       "inner_len was %d\n", (int)inner_len);
	if (inner_len <= 0) goto disconnect;

	/* Wrap and send PUBLISH, receive PUBACK */
	ptype = send_protected_receive(options,
	            inner_buf, (uint16_t)inner_len,
	            MQTTSN_PROTECTION_HMAC_SHA256,
	            MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	            MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	            MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	            random_t7, rbuf, (int)sizeof(rbuf), &rlen, NULL);

	assert("PUBACK received", ptype == MQTTSN_PUBACK,
	       "packet type was 0x%02X\n", ptype);

	if (ptype == MQTTSN_PUBACK)
	{
		rc = MQTTSNDeserialize_puback(&ack_packetid, &ack_rc, rbuf, rlen);
		assert("PUBACK deserialized", rc == 1, "rc was %d\n", rc);
		assert("PUBACK packetid matches", ack_packetid == pub_packetid,
		       "packetid was %u\n", ack_packetid);
		MyLog(LOGA_DEBUG, "PUBACK: packetid=%u rc=0x%02X", ack_packetid, ack_rc);
	}

disconnect:
	do_disconnect_protected(options, MQTTSN_PROTECTION_HMAC_SHA256, random_t7);

exit:
	MyLog(LOGA_INFO, "TEST7: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test8 — PROTECTION(CONNECT) + PROTECTION(PUBLISH QoS 2) full handshake
 *         PUBLISH → PUBREC → PUBREL → PUBCOMP
 * ========================================================================= */

int test8(struct Options options)
{
	MQTTSNPacket_connackData connack;
	MQTTSN_topic topic;
	uint8_t  inner_buf[BUF_SIZE];
	uint8_t  rbuf[BUF_SIZE];
	int32_t  inner_len;
	uint16_t pub_packetid = 1;
	uint8_t  acktype      = 0;
	uint16_t ack_packetid = 0;
	uint8_t  ack_rc       = 0xFF;
	int rlen = 0, rc = 0, ptype;
	static const uint8_t random_pub[4] = { 0xAB, 0xCD, 0x08, 0x0A };
	static const uint8_t random_rel[4] = { 0xAB, 0xCD, 0x08, 0x0B };

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-publish-qos2\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	    "Starting test 8 - PROTECTION(CONNECT + PUBLISH QoS 2) full handshake");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	rc = do_connect_protected(options, "mqttsn2-prot-t8", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	         MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	         MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	         random_t8, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto disconnect;

	/* Serialize PUBLISH QoS 2 */
	memset(&topic, 0, sizeof(topic));
	topic.type            = MQTTSN_TOPIC_TYPE_NAME;
	topic.alt.string.data = TEST_TOPIC;
	topic.alt.string.len  = TEST_TOPIC_LEN;

	inner_len = MQTTSNSerialize_publish(inner_buf, (int32_t)sizeof(inner_buf),
	                0, 2, 0, pub_packetid, &topic,
	                (uint8_t*)TEST_PAYLOAD, TEST_PAYLOAD_LEN);
	assert("PUBLISH QoS 2 serialized", inner_len > 0,
	       "inner_len was %d\n", (int)inner_len);
	if (inner_len <= 0) goto disconnect;

	/* step 1: PROTECTION(PUBLISH) → PUBREC */
	ptype = send_protected_receive(options,
	            inner_buf, (uint16_t)inner_len,
	            MQTTSN_PROTECTION_HMAC_SHA256,
	            MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	            MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	            MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	            random_pub, rbuf, (int)sizeof(rbuf), &rlen, NULL);

	assert("PUBREC received", ptype == MQTTSN_PUBREC,
	       "packet type was 0x%02X\n", ptype);
	if (ptype != MQTTSN_PUBREC) goto disconnect;

	rc = MQTTSNDeserialize_ack(&acktype, &ack_packetid, &ack_rc, rbuf, rlen);
	assert("PUBREC deserialized", rc == 1, "rc was %d\n", rc);
	assert("PUBREC packetid matches", ack_packetid == pub_packetid,
	       "packetid was %u\n", ack_packetid);
	MyLog(LOGA_DEBUG, "PUBREC received (packetid=%u)", ack_packetid);

	/* step 2: PROTECTION(PUBREL) → PUBCOMP */
	inner_len = MQTTSNSerialize_pubrel(inner_buf, (int32_t)sizeof(inner_buf),
	                pub_packetid, MQTT_SN_RC_SUCCESS);
	assert("PUBREL serialized", inner_len > 0, "inner_len was %d\n", (int)inner_len);
	if (inner_len <= 0) goto disconnect;

	ptype = send_protected_receive(options,
	            inner_buf, (uint16_t)inner_len,
	            MQTTSN_PROTECTION_HMAC_SHA256,
	            MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	            MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	            MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	            random_rel, rbuf, (int)sizeof(rbuf), &rlen, NULL);

	assert("PUBCOMP received", ptype == MQTTSN_PUBCOMP,
	       "packet type was 0x%02X\n", ptype);
	if (ptype == MQTTSN_PUBCOMP)
	{
		rc = MQTTSNDeserialize_ack(&acktype, &ack_packetid, &ack_rc, rbuf, rlen);
		assert("PUBCOMP deserialized", rc == 1, "rc was %d\n", rc);
		assert("PUBCOMP packetid matches", ack_packetid == pub_packetid,
		       "packetid was %u\n", ack_packetid);
		MyLog(LOGA_DEBUG, "PUBCOMP received (packetid=%u)", ack_packetid);
	}

disconnect:
	do_disconnect_protected(options, MQTTSN_PROTECTION_HMAC_SHA256, random_t8);

exit:
	MyLog(LOGA_INFO, "TEST8: test %s. %d tests run, %d failures.",
	      (failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/* =========================================================================
 * test9 — PROTECTION(CONNECT) + PROTECTION(DISCONNECT)
 *
 * Verifies that DISCONNECT can also be wrapped in protection.
 * Uses MQTTSNSerialize_protection directly (not the helper) so that the
 * helper's use of MQTTSNSerialize_protection is tested end-to-end.
 * ========================================================================= */

int test9(struct Options options)
{
	MQTTSNPacket_connectData conn  = MQTTSNPacket_connectData_initializer;
	MQTTSNPacket_connackData connack;
	MQTTSNPacket_protectionData pd;
	uint8_t  conn_inner[BUF_SIZE];
	uint8_t  disc_inner[8];
	uint8_t  prot_buf[BUF_SIZE];
	int32_t  inner_len, prot_len;
	int      rc = 0;

	fprintf(xml,
	    "<testcase classname=\"test3\" name=\"protection-disconnect\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO,
	    "Starting test 9 - PROTECTION(CONNECT) + PROTECTION(DISCONNECT)");

	rc = open_transport_with_timeout();
	assert("transport_open succeeded", rc >= 0, "rc was %d\n", rc);
	if (rc < 0) goto exit;

	/* Connect with protection */
	rc = do_connect_protected(options, "mqttsn2-prot-t9", 1,
	         MQTTSN_PROTECTION_HMAC_SHA256,
	         MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	         MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	         MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	         random_t9, &connack);
	assert("CONNACK received", rc == 1, "rc was %d\n", rc);
	assert("CONNACK success", connack.reasonCode == MQTT_SN_RC_SUCCESS,
	       "reason code was 0x%02X\n", connack.reasonCode);
	if (connack.reasonCode != MQTT_SN_RC_SUCCESS) goto close;

	/* Serialize the inner DISCONNECT (normal disconnection, 3 bytes) */
	inner_len = MQTTSNSerialize_disconnect(disc_inner,
	                (int32_t)sizeof(disc_inner), MQTT_SN_RC_SUCCESS);
	assert("DISCONNECT serialized", inner_len == 3,
	       "inner_len was %d\n", (int)inner_len);
	if (inner_len <= 0) goto close;

	/* Wrap in PROTECTION via wrap_with_protection */
	prot_len = wrap_with_protection(disc_inner, (uint16_t)inner_len,
	               MQTTSN_PROTECTION_HMAC_SHA256,
	               MQTTSN_PROTECTION_AUTHTAG_NOMINAL,
	               MQTTSN_PROTECTION_CRYPTO_NONE, NULL,
	               MQTTSN_PROTECTION_COUNTER_NONE, NULL,
	               random_t9, prot_buf, (int32_t)sizeof(prot_buf));
	assert("PROTECTION(DISCONNECT) serialized", prot_len > 0,
	       "prot_len was %d\n", (int)prot_len);

	/* Verify the packet can be round-tripped through MQTTSNDeserialize_protection */
	if (prot_len > 0)
	{
		memset(&pd, 0, sizeof(pd));
		rc = MQTTSNDeserialize_protection(&pd, prot_buf, prot_len);
		assert("PROTECTION(DISCONNECT) deserializes cleanly", rc == 1,
		       "rc was %d\n", rc);
		assert("Inner packet type is DISCONNECT",
		       packet_type_in_buf(pd.protectedPacket.data,
		                          (int)pd.protectedPacket.len) == MQTTSN_DISCONNECT,
		       "inner type was 0x%02X\n",
		       packet_type_in_buf(pd.protectedPacket.data,
		                           (int)pd.protectedPacket.len));
		assert("Auth tag length is 32 bytes (nominal for HMAC-SHA256)",
		       pd.authTag.len == 32, "auth_tag_len was %u\n", pd.authTag.len);

		/* Send the protected DISCONNECT to the server */
		rc = transport_sendPacketBuffer(options.host, options.port,
		                                prot_buf, (int)prot_len);
		assert("PROTECTION(DISCONNECT) sent", rc == 0, "rc was %d\n", rc);
		MyLog(LOGA_DEBUG, "PROTECTION(DISCONNECT) sent (%d bytes)", (int)prot_len);
	}

	/* We use conn_inner to suppress an unused-variable warning */
	(void)conn; (void)conn_inner;

close:
	transport_close();

exit:
	MyLog(LOGA_INFO, "TEST9: test %s. %d tests run, %d failures.",
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
		NULL,   /* 0: placeholder */
		test1,  /* 1: PROTECTION(CONNECT) nominal 32-byte HMAC tag */
		test2,  /* 2: PROTECTION(CONNECT) truncated 64-bit tag */
		test3,  /* 3: PROTECTION(CONNECT) with 2-byte counter */
		test4,  /* 4: PROTECTION(CONNECT) with 12-byte crypto material */
		test5,  /* 5: PROTECTION(CONNECT) corrupted auth tag → rejected */
		test6,  /* 6: PROTECTION(CONNECT + SUBSCRIBE) */
		test7,  /* 7: PROTECTION(CONNECT + PUBLISH QoS 1) + PUBACK */
		test8,  /* 8: PROTECTION(CONNECT + PUBLISH QoS 2) full handshake */
		test9,  /* 9: PROTECTION(CONNECT + DISCONNECT) + local round-trip */
	};

	xml = fopen("TEST-test3.xml", "w");
	fprintf(xml, "<testsuite name=\"test3\" tests=\"%d\">\n",
	        (int)(ARRAY_SIZE(testfuncs) - 1));

	getopts(argc, argv);
	MyLog(LOGA_INFO,
	    "MQTT-SN 2.0 Protection Encapsulation test suite: host=%s port=%d",
	    options.host, options.port);
	MyLog(LOGA_INFO,
	    "Sender ID: %02X%02X%02X%02X%02X%02X%02X%02X",
	    test_sender_id[0], test_sender_id[1], test_sender_id[2], test_sender_id[3],
	    test_sender_id[4], test_sender_id[5], test_sender_id[6], test_sender_id[7]);

	if (options.test_no == 0)
	{
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
