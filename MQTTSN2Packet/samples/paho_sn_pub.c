/*******************************************************************************
 * Copyright (c) 2026 IBM Corp.
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
 * mqttsn_pub - MQTT-SN 2.0 command-line publisher
 *
 * Usage: mqttsn_pub -t topic [-m message | -f file | -s | -n]
 *                   [-h host] [-p port] [-q qos] [-r]
 *                   [-i clientid] [-k keepalive]
 *                   [--predefined id] [--session id]
 *                   [-d delimiter] [--maxdatalen n]
 *                   [--verbose] [--quiet]
 *
 * Connects to an MQTT-SN gateway over UDP, publishes one or more messages,
 * then disconnects.
 *
 * Payload sources (one required):
 *   -m  literal string on the command line
 *   -f  entire contents of a file
 *   -n  null (zero-length) payload
 *   -l  read stdin, publishing one message per delimiter (default "\n")
 *
 * Transport: UDP via transport.c.
 */

#include "MQTTSNPacket.h"
#include "MQTTSNConnect.h"
#include "MQTTSNPublish.h"
#include "transport.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(WIN32)
#  define sleep(s) Sleep((s) * 1000)
#  define mysleep(ms) Sleep(ms)
#else
#  include <unistd.h>
#  define mysleep(ms) usleep((ms) * 1000)
#endif

/* -------------------------------------------------------------------------
 * Program options
 * ------------------------------------------------------------------------- */

typedef enum
{
	TOPIC_NAME       = 0,   /* default: full topic name string   */
	TOPIC_PREDEFINED = 1,   /* predefined topic alias            */
	TOPIC_SESSION    = 2    /* session topic alias               */
} topic_mode_t;

struct mqttsn_pub_opts
{
	/* application */
	int         quiet;
	int         verbose;

	/* connection */
	const char* host;
	int         port;

	/* MQTT-SN session */
	const char* clientid;
	int         keepalive;
	int         clean_start;

	/* topic */
	const char* topic;          /* topic name string (TOPIC_NAME mode)      */
	uint16_t    topic_alias;    /* alias value (PREDEFINED / SESSION mode)   */
	topic_mode_t topic_mode;

	/* publish */
	int         qos;
	int         retained;
	const char* message;        /* -m: literal payload string               */
	const char* filename;       /* -f: read payload from file               */
	int         null_message;   /* -n: publish zero-length payload          */
	int         stdin_lines;    /* -l: read stdin, one message per delimiter*/
	const char* delimiter;      /* line delimiter for -l mode (default "\n")*/
	int         maxdatalen;     /* maximum payload size for -l mode         */
};

static struct mqttsn_pub_opts opts =
{
	/* quiet, verbose */                     0, 0,
	/* host, port */                         "localhost", 1883,
	/* clientid, keepalive, clean_start */   "mqttsn-pub", 60, 1,
	/* topic, topic_alias, topic_mode */     NULL, 0, TOPIC_NAME,
	/* qos, retained */                      0, 0,
	/* message, filename, null_message */    NULL, NULL, 0,
	/* stdin_lines, delimiter, maxdatalen */ 0, "\n", 1024
};

/* -------------------------------------------------------------------------
 * Signal handling
 * ------------------------------------------------------------------------- */

static volatile int toStop = 0;

static void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}

/* -------------------------------------------------------------------------
 * Usage and option parsing
 * ------------------------------------------------------------------------- */

static void usage(const char* program_name)
{
	printf("Eclipse Paho MQTT-SN 2.0 publisher\n\n");
	printf("Usage: %s -t topic [-m message | -f file | -s | -n]\n"
	       "       [-h host] [-p port] [-q qos] [-r]\n"
	       "       [-i clientid] [-k keepalive]\n"
	       "       [--predefined id] [--session id]\n"
	       "       [-d delimiter] [--maxdatalen n]\n"
	       "       [--verbose] [--quiet]\n\n",
	       program_name);
	printf(
	"  -t (--topic)       : MQTT-SN topic name to publish to.  No default.\n"
	"  -m (--message)     : message payload to send.\n"
	"  -f (--filename)    : send the contents of a file as the payload.\n"
	"  -n (--null)        : send a null (zero-length) payload.\n"
	"  -l (--stdin-lines) : read stdin; publish one message per delimiter.\n"
	"  -h (--host)        : gateway host to connect to.  Default is %s.\n"
	"  -p (--port)        : gateway UDP port.  Default is %d.\n"
	"  -q (--qos)         : QoS level (0, 1, or 2).  Default is %d.\n"
	"  -r (--retained)    : set the Retain flag on the published message.\n"
	"  -i (--clientid)    : MQTT-SN client id.  Default is %s.\n"
	"  -k (--keepalive)   : keepalive timeout in seconds.  Default is %d.\n"
	"  --predefined id    : publish to a predefined topic alias.\n"
	"  --session id       : publish to a session topic alias.\n"
	"  -d (--delimiter)   : delimiter for -l mode.  Default is newline.\n"
	"  --maxdatalen n     : maximum payload size for -l mode.  Default is %d.\n"
	"  --verbose          : print details to stderr.\n"
	"  --quiet            : suppress error messages.\n",
	opts.host, opts.port, opts.qos, opts.clientid, opts.keepalive,
	opts.maxdatalen);
	printf("\nSee https://eclipse.dev/paho for more information about the Eclipse Paho project.\n");
	exit(EXIT_FAILURE);
}


static int getopts(int argc, char** argv)
{
	int count = 1;

	/* bare first argument with no leading '-' is the topic name */
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
		else if (strcmp(argv[count], "--retained") == 0 || strcmp(argv[count], "-r") == 0)
			opts.retained = 1;
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
		else if (strcmp(argv[count], "--message") == 0 || strcmp(argv[count], "-m") == 0)
		{
			if (++count < argc)
				opts.message = argv[count];
			else
				return 1;
		}
		else if (strcmp(argv[count], "--filename") == 0 || strcmp(argv[count], "-f") == 0)
		{
			if (++count < argc)
				opts.filename = argv[count];
			else
				return 1;
		}
		else if (strcmp(argv[count], "--null") == 0 || strcmp(argv[count], "-n") == 0)
			opts.null_message = 1;
		else if (strcmp(argv[count], "--stdin-lines") == 0 || strcmp(argv[count], "-l") == 0)
			opts.stdin_lines = 1;
		else if (strcmp(argv[count], "--delimiter") == 0 || strcmp(argv[count], "-d") == 0)
		{
			if (++count < argc)
				opts.delimiter = argv[count];
			else
				return 1;
		}
		else if (strcmp(argv[count], "--maxdatalen") == 0)
		{
			if (++count < argc)
				opts.maxdatalen = atoi(argv[count]);
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

#define BUF_SIZE 65535

/* Packet identifier counter — starts at 1, wraps at 65535 (never 0). */
static uint16_t next_packetid = 1;
static uint16_t get_packetid(void)
{
	uint16_t id = next_packetid;
	next_packetid = (next_packetid == 65535) ? 1 : next_packetid + 1;
	return id;
}


/* Send a CONNECT packet and wait for CONNACK.
 * Returns 0 on success, -1 on error. */
static int do_connect(void)
{
	uint8_t                   buf[BUF_SIZE];
	uint8_t                   rbuf[BUF_SIZE];
	MQTTSNPacket_connectData  options = MQTTSNPacket_connectData_initializer;
	MQTTSNPacket_connackData  connack;
	int                       slen, rlen;

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


/* Publish one message and handle QoS acknowledgement.
 * Returns 0 on success, -1 on error. */
static int do_publish(const uint8_t* payload, int32_t payloadlen)
{
	uint8_t      buf[BUF_SIZE];
	uint8_t      rbuf[BUF_SIZE];
	MQTTSN_topic topic;
	uint16_t     packetid = 0;
	int32_t      slen, rlen;

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
	default: /* TOPIC_NAME */
		topic.type            = MQTTSN_TOPIC_TYPE_NAME;
		topic.alt.string.len  = (uint16_t)strlen(opts.topic);
		topic.alt.string.data = (char*)opts.topic;
		break;
	}

	if (opts.qos > 0)
		packetid = get_packetid();

	slen = MQTTSNSerialize_publish(buf, (int32_t)sizeof(buf),
	        0, opts.qos, (uint8_t)opts.retained, packetid,
	        &topic, (uint8_t*)payload, payloadlen);
	if (slen <= 0)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to serialize PUBLISH (%d)\n", (int)slen);
		return -1;
	}

	if (opts.verbose)
		fprintf(stderr, "Publishing %d bytes\n", (int)payloadlen);

	if (transport_sendPacketBuffer((char*)opts.host, opts.port, buf, (int)slen) != 0)
		return -1;

	/* QoS 0: fire and forget */
	if (opts.qos == 0)
		return 0;

	/* QoS 1: wait for PUBACK */
	if (opts.qos == 1)
	{
		uint16_t ret_packetid = 0;
		uint8_t  returncode   = 0;

		rlen = transport_getdata(rbuf, sizeof(rbuf));
		if (rlen < 2)
		{
			if (!opts.quiet)
				fprintf(stderr, "No PUBACK received\n");
			return -1;
		}

		if (MQTTSNDeserialize_puback(&ret_packetid, &returncode, rbuf, rlen) != 1)
		{
			if (!opts.quiet)
				fprintf(stderr, "Failed to deserialize PUBACK\n");
			return -1;
		}

		if (ret_packetid != packetid)
		{
			if (!opts.quiet)
				fprintf(stderr, "PUBACK packet id mismatch: expected %u got %u\n",
				        packetid, ret_packetid);
			return -1;
		}

		if (returncode != MQTT_SN_RC_SUCCESS)
		{
			if (!opts.quiet)
				fprintf(stderr, "PUBACK reason code 0x%02X\n", returncode);
			return -1;
		}

		if (opts.verbose)
			fprintf(stderr, "PUBACK received\n");
		return 0;
	}

	/* QoS 2: PUBLISH → PUBREC → PUBREL → PUBCOMP */
	{
		uint8_t  acktype     = 0;
		uint16_t ret_packetid = 0;
		uint8_t  returncode   = 0;
		int32_t  acklen;

		/* step 1: receive PUBREC */
		rlen = transport_getdata(rbuf, sizeof(rbuf));
		if (rlen < 2)
		{
			if (!opts.quiet)
				fprintf(stderr, "No PUBREC received\n");
			return -1;
		}

		if (MQTTSNDeserialize_ack(&acktype, &ret_packetid, &returncode,
		                           rbuf, rlen) != 1
		    || acktype != MQTTSN_PUBREC)
		{
			if (!opts.quiet)
				fprintf(stderr, "Expected PUBREC, got 0x%02X\n", acktype);
			return -1;
		}

		if (returncode != MQTT_SN_RC_SUCCESS)
		{
			if (!opts.quiet)
				fprintf(stderr, "PUBREC reason code 0x%02X\n", returncode);
			return -1;
		}

		/* step 2: send PUBREL */
		acklen = MQTTSNSerialize_pubrel(buf, (int32_t)sizeof(buf),
		                                packetid, MQTT_SN_RC_SUCCESS);
		if (acklen <= 0 ||
		    transport_sendPacketBuffer((char*)opts.host, opts.port, buf, (int)acklen) != 0)
		{
			if (!opts.quiet)
				fprintf(stderr, "Failed to send PUBREL\n");
			return -1;
		}

		/* step 3: receive PUBCOMP */
		rlen = transport_getdata(rbuf, sizeof(rbuf));
		if (rlen < 2)
		{
			if (!opts.quiet)
				fprintf(stderr, "No PUBCOMP received\n");
			return -1;
		}

		if (MQTTSNDeserialize_ack(&acktype, &ret_packetid, &returncode,
		                           rbuf, rlen) != 1
		    || acktype != MQTTSN_PUBCOMP)
		{
			if (!opts.quiet)
				fprintf(stderr, "Expected PUBCOMP, got 0x%02X\n", acktype);
			return -1;
		}

		if (opts.verbose)
			fprintf(stderr, "PUBCOMP received\n");
		return 0;
	}
}


/* Send a DISCONNECT packet. */
static void do_disconnect(void)
{
	uint8_t buf[2] = { 2, MQTTSN_DISCONNECT };
	transport_sendPacketBuffer((char*)opts.host, opts.port, buf, sizeof(buf));
	if (opts.verbose)
		fprintf(stderr, "Disconnected\n");
}


/* Read the entire contents of a file into a heap buffer.
 * Caller must free the returned pointer.  Returns NULL on error. */
static char* readfile(int* datalen)
{
	FILE*  fp;
	char*  buffer;
	long   filelen;

	fp = fopen(opts.filename, "rb");
	if (fp == NULL)
	{
		if (!opts.quiet)
			fprintf(stderr, "Could not open file %s\n", opts.filename);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	rewind(fp);

	buffer = malloc((size_t)filelen);
	if (buffer == NULL)
	{
		if (!opts.quiet)
			fprintf(stderr, "Out of memory reading %s\n", opts.filename);
		fclose(fp);
		return NULL;
	}

	*datalen = (int)fread(buffer, 1, (size_t)filelen, fp);
	fclose(fp);
	return buffer;
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(int argc, char** argv)
{
	char*  buffer = NULL;
	int    rc     = EXIT_SUCCESS;

	if (argc < 2)
		usage(argv[0]);

	if (getopts(argc, argv) != 0)
		usage(argv[0]);

	/* A topic must be supplied in one of the three forms */
	if (opts.topic_mode == TOPIC_NAME && opts.topic == NULL)
	{
		fprintf(stderr, "No topic specified; use -t, --predefined, or --session\n");
		usage(argv[0]);
	}

	/* Exactly one payload source must be given */
	if ((opts.message == NULL) + (opts.filename == NULL) +
	    (opts.null_message == 0) + (opts.stdin_lines == 0) == 4)
	{
		fprintf(stderr, "No payload source specified; use -m, -f, -n, or -l\n");
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

	/* --- one-shot payload sources: publish once then stop --- */

	if (opts.null_message)
	{
		if (do_publish(NULL, 0) != 0)
			rc = EXIT_FAILURE;
	}
	else if (opts.message)
	{
		if (do_publish((uint8_t*)opts.message, (int32_t)strlen(opts.message)) != 0)
			rc = EXIT_FAILURE;
	}
	else if (opts.filename)
	{
		int datalen = 0;
		buffer = readfile(&datalen);
		if (buffer == NULL)
			rc = EXIT_FAILURE;
		else if (do_publish((uint8_t*)buffer, (int32_t)datalen) != 0)
			rc = EXIT_FAILURE;
	}

	/* --- stdin_lines: read and publish until EOF or Ctrl-C --- */

	if (opts.stdin_lines)
	{
		int  delim_len = (int)strlen(opts.delimiter);
		int  data_len;
		int  ch;

		buffer = malloc((size_t)opts.maxdatalen);
		if (buffer == NULL)
		{
			if (!opts.quiet)
				fprintf(stderr, "Out of memory\n");
			rc = EXIT_FAILURE;
			goto disconnect;
		}

		while (!toStop)
		{
			data_len = 0;
			while (data_len < opts.maxdatalen)
			{
				ch = getchar();
				if (ch == EOF)
				{
					toStop = 1;
					break;
				}
				buffer[data_len++] = (char)ch;
				if (data_len >= delim_len &&
				    strncmp(opts.delimiter,
				            &buffer[data_len - delim_len],
				            (size_t)delim_len) == 0)
					break;
			}
			if (data_len > 0)
			{
				if (do_publish((uint8_t*)buffer, (int32_t)data_len) != 0)
				{
					rc = EXIT_FAILURE;
					break;
				}
			}
		}
	}

disconnect:
	do_disconnect();

cleanup:
	transport_close();
	free(buffer);

	return rc;
}
