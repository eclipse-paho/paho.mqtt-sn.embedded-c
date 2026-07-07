/*******************************************************************************
 * Copyright (c) 2026 IBM Corp., Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

/*
 * mqttsn_sub - MQTT-SN 2.0 command-line subscriber
 *
 * Usage: mqttsn_sub [topicfilter] [-t topic] [-h host] [-p port]
 *                   [-q qos] [-i clientid] [-k keepalive]
 *                   [--no-local] [--retain-as-published]
 *                   [--retain-handling 0|1|2]
 *                   [--predefined id] [--session id]
 *                   [--no-delimiter] [--verbose] [--quiet]
 *
 * Connects to an MQTT-SN gateway over UDP, sends CONNECT + SUBSCRIBE,
 * then prints each incoming PUBLISH payload to stdout until interrupted.
 *
 * Transport: UDP via transport.c (transport_open / transport_sendPacketBuffer /
 * transport_getdata / transport_close).
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"
#include "MQTTSNSubscribe.h"
#include "MQTTSNPublish.h"
#include "transport.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(WIN32)
#  define sleep(s) Sleep((s) * 1000)
#else
#  include <unistd.h>
#endif

/* -------------------------------------------------------------------------
 * Program options
 * ------------------------------------------------------------------------- */

/* Topic type selector for the subscription */
typedef enum
{
	TOPIC_FILTER     = 0,   /* default: use a topic filter string */
	TOPIC_PREDEFINED = 1,   /* use a predefined topic alias        */
	TOPIC_SESSION    = 2    /* use a session topic alias           */
} topic_mode_t;

struct mqttsn_sub_opts
{
	/* application */
	int         quiet;
	int         verbose;
	const char* delimiter;      /* printed after each payload; default "\n" */

	/* connection */
	const char* host;
	int         port;

	/* MQTT-SN session */
	const char* clientid;
	int         keepalive;      /* seconds */
	int         clean_start;

	/* subscription */
	const char* topic;          /* filter string (TOPIC_FILTER mode)   */
	uint16_t    topic_alias;    /* alias value  (PREDEFINED/SESSION)   */
	topic_mode_t topic_mode;
	int         qos;
	uint8_t     no_local;
	uint8_t     rap;            /* retain as published */
	uint8_t     retain_handling;/* 0 / 1 / 2 */
};

static struct mqttsn_sub_opts opts =
{
	/* quiet, verbose */         0, 0,
	/* delimiter */              "\n",
	/* host, port */             "localhost", 1883,
	/* clientid, keepalive, clean_start */ "mqttsn-sub", 60, 1,
	/* topic, topic_alias, topic_mode */ NULL, 0, TOPIC_FILTER,
	/* qos, no_local, rap, retain_handling */ 0, 0, 0, 0
};

/* -------------------------------------------------------------------------
 * Signal handling
 * ------------------------------------------------------------------------- */

static volatile int finished = 0;

static void cfinish(int sig)
{
	signal(SIGINT, NULL);
	finished = 1;
}

/* -------------------------------------------------------------------------
 * Usage and option parsing
 * ------------------------------------------------------------------------- */

static void usage(const char* program_name)
{
	printf("Eclipse Paho MQTT-SN 2.0 subscriber\n\n");
	printf("Usage: %s [topicfilter] [-t filter] [-h host] [-p port]\n"
	       "       [-q qos] [-i clientid] [-k keepalive]\n"
	       "       [--no-local] [--retain-as-published]\n"
	       "       [--retain-handling 0|1|2]\n"
	       "       [--predefined id] [--session id]\n"
	       "       [--no-delimiter] [--verbose] [--quiet]\n\n",
	       program_name);
	printf(
	"  -t (--topic)           : MQTT-SN topic filter to subscribe to.  No default.\n"
	"  -h (--host)            : gateway host to connect to.  Default is %s.\n"
	"  -p (--port)            : gateway UDP port.  Default is %d.\n"
	"  -q (--qos)             : requested QoS (0, 1, or 2).  Default is %d.\n"
	"  -i (--clientid)        : MQTT-SN client id.  Default is %s.\n"
	"  -k (--keepalive)       : keepalive timeout in seconds.  Default is %d.\n"
	"  --no-local             : request No Local option (suppress own publishes).\n"
	"  --retain-as-published  : request Retain as Published option.\n"
	"  --retain-handling n    : retain handling (0=send, 1=new only, 2=never).\n"
	"                           Default is 0.\n"
	"  --predefined id        : subscribe using a predefined topic alias.\n"
	"  --session id           : subscribe using a session topic alias.\n"
	"  --no-delimiter         : do not print delimiter between messages.\n"
	"  --verbose              : print topic and packet details to stderr.\n"
	"  --quiet                : do not print error messages.\n",
	opts.host, opts.port, opts.qos, opts.clientid, opts.keepalive);
	printf("\nSee https://eclipse.dev/paho for more information about the Eclipse Paho project.\n");
	exit(EXIT_FAILURE);
}


static int getopts(int argc, char** argv)
{
	int count = 1;

	/* bare first argument with no leading '-' is the topic filter */
	if (count < argc && argv[count][0] != '-')
	{
		opts.topic = argv[count];
		count = 2;
	}

	while (count < argc)
	{
		if (strcmp(argv[count], "--topic") == 0 || strcmp(argv[count], "-t") == 0)
		{
			if (++count < argc)
				opts.topic = argv[count];
			else
				return 1;
		}
		else if (strcmp(argv[count], "--host") == 0 || strcmp(argv[count], "-h") == 0)
		{
			if (++count < argc)
				opts.host = argv[count];
			else
				return 1;
		}
		else if (strcmp(argv[count], "--port") == 0 || strcmp(argv[count], "-p") == 0)
		{
			if (++count < argc)
				opts.port = atoi(argv[count]);
			else
				return 1;
		}
		else if (strcmp(argv[count], "--qos") == 0 || strcmp(argv[count], "-q") == 0)
		{
			if (++count < argc)
			{
				int q = atoi(argv[count]);
				if (q < 0 || q > 2)
				{
					fprintf(stderr, "Invalid QoS value %s; must be 0, 1, or 2\n", argv[count]);
					return 1;
				}
				opts.qos = q;
			}
			else
				return 1;
		}
		else if (strcmp(argv[count], "--clientid") == 0 || strcmp(argv[count], "-i") == 0)
		{
			if (++count < argc)
				opts.clientid = argv[count];
			else
				return 1;
		}
		else if (strcmp(argv[count], "--keepalive") == 0 || strcmp(argv[count], "-k") == 0)
		{
			if (++count < argc)
				opts.keepalive = atoi(argv[count]);
			else
				return 1;
		}
		else if (strcmp(argv[count], "--no-local") == 0)
			opts.no_local = 1;
		else if (strcmp(argv[count], "--retain-as-published") == 0)
			opts.rap = 1;
		else if (strcmp(argv[count], "--retain-handling") == 0)
		{
			if (++count < argc)
			{
				int rh = atoi(argv[count]);
				if (rh < 0 || rh > 2)
				{
					fprintf(stderr, "Invalid retain-handling value %s; must be 0, 1, or 2\n",
					        argv[count]);
					return 1;
				}
				opts.retain_handling = (uint8_t)rh;
			}
			else
				return 1;
		}
		else if (strcmp(argv[count], "--predefined") == 0)
		{
			if (++count < argc)
			{
				opts.topic_alias = (uint16_t)atoi(argv[count]);
				opts.topic_mode  = TOPIC_PREDEFINED;
			}
			else
				return 1;
		}
		else if (strcmp(argv[count], "--session") == 0)
		{
			if (++count < argc)
			{
				opts.topic_alias = (uint16_t)atoi(argv[count]);
				opts.topic_mode  = TOPIC_SESSION;
			}
			else
				return 1;
		}
		else if (strcmp(argv[count], "--no-delimiter") == 0)
			opts.delimiter = NULL;
		else if (strcmp(argv[count], "--verbose") == 0 || strcmp(argv[count], "-v") == 0)
			opts.verbose = 1;
		else if (strcmp(argv[count], "--quiet") == 0)
			opts.quiet = 1;
		else
		{
			fprintf(stderr, "Unknown option: %s\n", argv[count]);
			return 1;
		}
		++count;
	}
	return 0;
}

/* -------------------------------------------------------------------------
 * MQTT-SN packet helpers
 * ------------------------------------------------------------------------- */

#define BUF_SIZE 1024

/* Send a CONNECT packet and wait for CONNACK.
 * Returns 0 on success, -1 on error. */
static int do_connect(void)
{
	uint8_t                    buf[BUF_SIZE];
	uint8_t                    rbuf[BUF_SIZE];
	MQTTSNPacket_connectData   options = MQTTSNPacket_connectData_initializer;
	MQTTSNPacket_connackData   connack;
	int                        slen, rlen;

	options.clientID.len          = (uint16_t)strlen(opts.clientid);
	options.clientID.data         = (char*)opts.clientid;
	options.keepAlive             = (uint16_t)opts.keepalive;
	options.flags.bits.cleanStart = (opts.clean_start != 0);

	slen = MQTTSNSerialize_connect(buf, (int)sizeof(buf), &options);
	if (slen <= 0)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to serialize CONNECT (%d)\n", slen);
		return -1;
	}

	if (opts.verbose)
		fprintf(stderr, "Sending CONNECT (clientid=%s, keepalive=%d)\n",
		        opts.clientid, opts.keepalive);

	if (transport_sendPacketBuffer((char*)opts.host, opts.port, buf, slen) != 0)
		return -1;

	/* wait for CONNACK */
	rlen = transport_getdata(rbuf, sizeof(rbuf));
	if (rlen < 2)
	{
		if (!opts.quiet)
			fprintf(stderr, "No CONNACK received\n");
		return -1;
	}

	if (MQTTSNDeserialize_connack(&connack, rbuf, rlen) != 1)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to deserialize CONNACK\n");
		return -1;
	}

	if (connack.reasonCode != MQTT_SN_RC_SUCCESS)
	{
		if (!opts.quiet)
			fprintf(stderr, "CONNACK reason code 0x%02X\n", connack.reasonCode);
		return -1;
	}

	if (opts.verbose)
		fprintf(stderr, "Connected\n");
	return 0;
}


/* Send a SUBSCRIBE packet and wait for SUBACK.
 * Returns 0 on success, -1 on error. */
static int do_subscribe(void)
{
	uint8_t      buf[BUF_SIZE];
	uint8_t      rbuf[BUF_SIZE];
	int32_t      slen, rlen;
	MQTTSN_topic topic;
	uint16_t     packetid = 1;

	memset(&topic, 0, sizeof(topic));

	switch (opts.topic_mode)
	{
	case TOPIC_PREDEFINED:
		topic.type      = MQTTSN_TOPIC_TYPE_PREDEFINED;
		topic.alt.alias = opts.topic_alias;
		break;
	case TOPIC_SESSION:
		topic.type      = MQTTSN_TOPIC_TYPE_SESSION;
		topic.alt.alias = opts.topic_alias;
		break;
	default: /* TOPIC_FILTER */
		topic.type            = MQTTSN_TOPIC_TYPE_FILTER;
		topic.alt.string.len  = (uint16_t)strlen(opts.topic);
		topic.alt.string.data = (char*)opts.topic;
		break;
	}

	slen = MQTTSNSerialize_subscribe(buf, (int32_t)sizeof(buf),
	        opts.qos, opts.retain_handling, opts.rap, opts.no_local,
	        packetid, &topic);
	if (slen <= 0)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to serialize SUBSCRIBE (%d)\n", (int)slen);
		return -1;
	}

	if (opts.verbose)
	{
		if (opts.topic_mode == TOPIC_FILTER)
			fprintf(stderr, "Subscribing to topic filter \"%s\" QoS %d\n",
			        opts.topic, opts.qos);
		else
			fprintf(stderr, "Subscribing to %s alias %u QoS %d\n",
			        opts.topic_mode == TOPIC_PREDEFINED ? "predefined" : "session",
			        opts.topic_alias, opts.qos);
	}

	if (transport_sendPacketBuffer((char*)opts.host, opts.port, buf, (int)slen) != 0)
		return -1;

	/* wait for SUBACK */
	rlen = transport_getdata(rbuf, sizeof(rbuf));
	if (rlen < 2)
	{
		if (!opts.quiet)
			fprintf(stderr, "No SUBACK received\n");
		return -1;
	}

	int lenlen = (rbuf[0] == 1) ? 3 : 1;
	if (rbuf[lenlen] != MQTTSN_SUBACK)
	{
		if (!opts.quiet)
			fprintf(stderr, "Expected SUBACK (0x%02X), got 0x%02X\n",
			        MQTTSN_SUBACK, rbuf[lenlen]);
		return -1;
	}

	uint16_t ret_topicid         = 0;
	uint8_t  topic_alias_present = 0;
	uint16_t ret_packetid        = 0;
	uint8_t  reasoncode          = 0;

	if (MQTTSNDeserialize_suback(&ret_topicid, &topic_alias_present,
	                              &ret_packetid, &reasoncode,
	                              rbuf, (int32_t)rlen) != 1)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to deserialize SUBACK\n");
		return -1;
	}

	if (reasoncode >= 0x80)
	{
		if (!opts.quiet)
			fprintf(stderr, "SUBACK reason code 0x%02X\n", reasoncode);
		return -1;
	}

	if (opts.verbose)
	{
		fprintf(stderr, "Subscribed (granted QoS %d", reasoncode);
		if (topic_alias_present)
			fprintf(stderr, ", topic alias %u", ret_topicid);
		fprintf(stderr, ")\n");
	}

	return 0;
}


/* Send a DISCONNECT packet. */
static void do_disconnect(void)
{
	uint8_t buf[2] = { 2, MQTTSN_DISCONNECT };
	transport_sendPacketBuffer((char*)opts.host, opts.port, buf, sizeof(buf));
	if (opts.verbose)
		fprintf(stderr, "Disconnected\n");
}


/* Handle one incoming PUBLISH packet, printing the payload to stdout.
 * Returns 1 if a message was printed, 0 if the packet was not a PUBLISH. */
static int handle_publish(const uint8_t* buf, int32_t len)
{
	int      lenlen     = (buf[0] == 1) ? 3 : 1;
	uint8_t  ptype      = buf[lenlen];
	uint8_t  dup, retained;
	int32_t  qos;
	uint16_t packetid   = 0;
	MQTTSN_topic topic;
	uint8_t* payload    = NULL;
	int32_t  payloadlen = 0;

	if (ptype != MQTTSN_PUBLISH)
		return 0;

	if (MQTTSNDeserialize_publish(&dup, &qos, &retained, &packetid, &topic,
	                               &payload, &payloadlen,
	                               (uint8_t*)buf, len) != 1)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to deserialize PUBLISH\n");
		return 0;
	}

	if (opts.verbose)
	{
		if (topic.type == MQTTSN_TOPIC_TYPE_NAME)
			fprintf(stderr, "%d %.*s\t", payloadlen,
			        (int)topic.alt.string.len, topic.alt.string.data);
		else
			fprintf(stderr, "%d [alias:%u]\t", payloadlen, topic.alt.alias);
	}

	printf("%.*s", payloadlen, (char*)payload);
	if (opts.delimiter)
		printf("%s", opts.delimiter);
	fflush(stdout);

	/* send PUBACK for QoS 1 */
	if (qos == 1)
	{
		uint8_t ackbuf[8];
		int32_t acklen = MQTTSNSerialize_puback(ackbuf, (int32_t)sizeof(ackbuf),
		                         packetid, MQTT_SN_RC_SUCCESS);
		if (acklen > 0)
			transport_sendPacketBuffer((char*)opts.host, opts.port, ackbuf, (int)acklen);
	}
	/* QoS 2 full handshake omitted for clarity: send PUBREC, await PUBREL, send PUBCOMP */

	return 1;
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(int argc, char** argv)
{
	uint8_t  rbuf[BUF_SIZE];
	int      rlen;
	int      rc = EXIT_SUCCESS;

	if (argc < 2)
		usage(argv[0]);

	if (getopts(argc, argv) != 0)
		usage(argv[0]);

	/* A topic must be specified in one of the three forms */
	if (opts.topic_mode == TOPIC_FILTER && opts.topic == NULL)
	{
		fprintf(stderr, "No topic specified; use -t, --predefined, or --session\n");
		usage(argv[0]);
	}

	signal(SIGINT,  cfinish);
	signal(SIGTERM, cfinish);

	if (transport_open() < 0)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to open transport\n");
		return EXIT_FAILURE;
	}

	if (do_connect() != 0)
	{
		rc = EXIT_FAILURE;
		goto cleanup;
	}

	if (do_subscribe() != 0)
	{
		rc = EXIT_FAILURE;
		goto disconnect;
	}

	/* receive loop */
	while (!finished)
	{
		rlen = transport_getdata(rbuf, sizeof(rbuf));
		printf("rc from getdata is %d\n", rlen);
		if (rlen > 0)
			handle_publish(rbuf, (int32_t)rlen);
	}

disconnect:
	do_disconnect();

cleanup:
	transport_close();

	return rc;
}
