/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/


#include "MQTTSNPacket.h"

#include <sys/types.h>

#if !defined(SOCKET_ERROR)
	/** error in socket operation */
	#define SOCKET_ERROR -1
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(_WINDOWS)
#define INVALID_SOCKET SOCKET_ERROR
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
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

struct Options
{
	char* host;  
	int port;
	int verbose;
	int test_no;
} options =
{
	"127.0.0.1",
	1884,
	0,
	0,
};

void usage()
{

}

void getopts(int argc, char** argv)
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
				options.port = atoi(argv[count]);
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


#define LOGA_DEBUG 0
#define LOGA_INFO 1
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>
void MyLog(int LOGA_level, char* format, ...)
{
	static char msg_buf[256];
	va_list args;
	struct timeb ts;

	struct tm *timeinfo;

	if (LOGA_level == LOGA_DEBUG && options.verbose == 0)
	  return;

	ftime(&ts);
	timeinfo = localtime(&ts.time);
	strftime(msg_buf, 80, "%Y%m%d %H%M%S", timeinfo);

	sprintf(&msg_buf[strlen(msg_buf)], ".%.3hu ", ts.millitm);

	va_start(args, format);
	vsnprintf(&msg_buf[strlen(msg_buf)], sizeof(msg_buf) - strlen(msg_buf), format, args);
	va_end(args);

	printf("%s\n", msg_buf);
	fflush(stdout);
}


#if defined(WIN32) || defined(_WINDOWS)
#define mqsleep(A) Sleep(1000*A)
#define START_TIME_TYPE DWORD
static DWORD start_time = 0;
START_TIME_TYPE start_clock(void)
{
	return GetTickCount();
}
#elif defined(AIX)
#define mqsleep sleep
#define START_TIME_TYPE struct timespec
START_TIME_TYPE start_clock(void)
{
	static struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
	return start;
}
#else
#define mqsleep sleep
#define START_TIME_TYPE struct timeval
/* TODO - unused - remove? static struct timeval start_time; */
START_TIME_TYPE start_clock(void)
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	return start_time;
}
#endif


#if defined(WIN32)
long elapsed(START_TIME_TYPE start_time)
{
	return GetTickCount() - start_time;
}
#elif defined(AIX)
#define assert(a)
long elapsed(struct timespec start)
{
	struct timespec now, res;

	clock_gettime(CLOCK_REALTIME, &now);
	ntimersub(now, start, res);
	return (res.tv_sec)*1000L + (res.tv_nsec)/1000000L;
}
#else
long elapsed(START_TIME_TYPE start_time)
{
	struct timeval now, res;

	gettimeofday(&now, NULL);
	timersub(&now, &start_time, &res);
	return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}
#endif


#define assert(a, b, c, d) myassert(__FILE__, __LINE__, a, b, c, d)
#define assert1(a, b, c, d, e) myassert(__FILE__, __LINE__, a, b, c, d, e)

int tests = 0;
int failures = 0;
FILE* xml;
START_TIME_TYPE global_start_time;
char output[3000];
char* cur_output = output;


void write_test_result()
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


void myassert(char* filename, int lineno, char* description, int value, char* format, ...)
{
	++tests;
	if (!value)
	{
		va_list args;

		++failures;
		printf("Assertion failed, file %s, line %d, description: %s\n", filename, lineno, description);

		va_start(args, format);
		vprintf(format, args);
		va_end(args);

		cur_output += sprintf(cur_output, "<failure type=\"%s\">file %s, line %d </failure>\n",
                        description, filename, lineno);
	}
    else
    	MyLog(LOGA_DEBUG, "Assertion succeeded, file %s, line %d, description: %s", filename, lineno, description);
}

#define min(a, b) ((a < b) ? a : b)

int Socket_error(char* aString, int sock)
{
#if defined(WIN32)
	int errno;
#endif

#if defined(WIN32)
	errno = WSAGetLastError();
#endif
	if (errno != EINTR && errno != EAGAIN && errno != EINPROGRESS && errno != EWOULDBLOCK)
	{
		if (strcmp(aString, "shutdown") != 0 || (errno != ENOTCONN && errno != ECONNRESET))
		{
			int orig_errno = errno;
			char* errmsg = strerror(errno);

			printf("Socket error %d (%s) in %s for socket %d\n", orig_errno, errmsg, aString, sock);
		}
	}
	return errno;
}


int sendPacketBuffer(int asocket, char* host, int port, unsigned char* buf, int buflen)
{
	struct sockaddr_in cliaddr;
	int rc = 0;

	memset(&cliaddr, 0, sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = inet_addr(host);
	cliaddr.sin_port = htons(port);

	if ((rc = sendto(asocket, buf, buflen, 0, (const struct sockaddr*)&cliaddr, sizeof(cliaddr))) == SOCKET_ERROR)
		Socket_error("sendto", asocket);
	else
		rc = 0;
	return rc;
}


int mysock = 0;

int getdata(unsigned char* buf, int count)
{
	int rc = recvfrom(mysock, buf, count, 0, NULL, NULL);
	//printf("received %d bytes count %d\n", rc, (int)count);
	return rc;
}


int connectDisconnect(struct Options options)
{
	MQTTSNPacket_connectData data = MQTTSNPacket_connectData_initializer;
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	int len = 0;

	mysock = socket(AF_INET, SOCK_DGRAM, 0);
	if (mysock == INVALID_SOCKET)
		rc = Socket_error("socket", mysock);

	data.clientID.cstring = "test2/test1";

	data.cleansession = 0;

	rc = MQTTSNSerialize_connect(buf, buflen, &data);
	assert("good rc from serialize connect", rc > 0, "rc was %d\n", rc);

	rc = sendPacketBuffer(mysock, options.host, options.port, buf, rc);
	assert("good rc from sendPacketBuffer", rc == 0, "rc was %d\n", rc);

	/* wait for connack */
	if (MQTTSNPacket_read(buf, buflen, getdata) == MQTTSN_CONNACK)
	{
		int connack_rc = -1;

		if (MQTTSNDeserialize_connack(&connack_rc, buf, buflen) != 1 || connack_rc != 0)
		{
			printf("Unable to connect, return code %d\n", connack_rc);
			goto exit;
		}
		else 
			printf("connected rc %d\n", connack_rc);
	}
	else
		goto exit;

	len = MQTTSNSerialize_disconnect(buf, buflen, 0);
	rc = sendPacketBuffer(mysock, options.host, options.port, buf, len);

	rc = shutdown(mysock, SHUT_WR);
	rc = close(mysock);

exit:
	return rc;
}

int test1(struct Options options)
{

	fprintf(xml, "<testcase classname=\"test1\" name=\"reconnect\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 1 - reconnection");

	connectDisconnect(options);
	connectDisconnect(options);

	MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int connectSubscribeDisconnect(struct Options options)
{
	MQTTSNPacket_connectData data = MQTTSNPacket_connectData_initializer;
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	int len = 0;
	MQTTSN_topicid topic;

	mysock = socket(AF_INET, SOCK_DGRAM, 0);
	if (mysock == INVALID_SOCKET)
		rc = Socket_error("socket", mysock);

	data.clientID.cstring = "test2/test1";

	data.cleansession = 0;

	rc = MQTTSNSerialize_connect(buf, buflen, &data);
	assert("good rc from serialize connect", rc > 0, "rc was %d\n", rc);

	rc = sendPacketBuffer(mysock, options.host, options.port, buf, rc);
	assert("good rc from sendPacketBuffer", rc == 0, "rc was %d\n", rc);

	/* wait for connack */
	if (MQTTSNPacket_read(buf, buflen, getdata) == MQTTSN_CONNACK)
	{
		int connack_rc = -1;

		rc = MQTTSNDeserialize_connack(&connack_rc, buf, buflen);
		assert("Good rc from deserialize connack", rc == 1, "rc was %d\n", rc);
		assert("Good rc from connect", connack_rc == 0, "connack_rc was %d\n", rc);
		if (connack_rc != 0)
			goto exit;
	}
	else
		goto exit;

	/* subscribe */
	printf("Subscribing\n");
	topic.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic.data.long_.name = "substopic";
	topic.data.long_.len = strlen(topic.data.long_.name);
	len = MQTTSNSerialize_subscribe(buf, buflen, 0, 2, /*msgid*/ 1, &topic);
	rc = sendPacketBuffer(mysock, options.host, options.port, buf, len);

	if (MQTTSNPacket_read(buf, buflen, getdata) == MQTTSN_SUBACK) 	/* wait for suback */
	{
		unsigned short submsgid;
		int granted_qos;
		unsigned char returncode;
		unsigned short topicid;

		rc = MQTTSNDeserialize_suback(&granted_qos, &topicid, &submsgid, &returncode, buf, buflen);
		if (granted_qos != 2 || returncode != 0)
		{
			printf("granted qos != 2, %d return code %d\n", granted_qos, returncode);
			goto exit;
		}
		else
			printf("suback topic id %d\n", topicid);
	}
	else
		goto exit;

	len = MQTTSNSerialize_disconnect(buf, buflen, 0);
	rc = sendPacketBuffer(mysock, options.host, options.port, buf, len);

	rc = shutdown(mysock, SHUT_WR);
	rc = close(mysock);

exit:
	return rc;
}


/*
 * Connect non-cleansession, subscribe, disconnect.
 * Then reconnect non-cleansession.
 */
int test2(struct Options options)
{

	fprintf(xml, "<testcase classname=\"test2\" name=\"clientid free\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 2 - clientid free");

	connectSubscribeDisconnect(options);
	connectDisconnect(options);

	MyLog(LOGA_INFO, "TEST2: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


#if 0
int test3(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);

	fprintf(xml, "<testcase classname=\"test3\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 3 - will messages");

	rc = MQTTSNSerialize_willtopicreq(buf, buflen);
	assert("good rc from serialize willtopicreq", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willtopicreq(buf, buflen);
	assert("good rc from deserialize willtopicreq", rc == 1, "rc was %d\n", rc);

	rc = MQTTSNSerialize_willmsgreq(buf, buflen);
	assert("good rc from serialize willmsgreq", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willmsgreq(buf, rc);
	assert("good rc from deserialize willmsgreq", rc == 1, "rc was %d\n", rc);

/* exit: */
	MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


#if 0
int test3(struct Options options)
{
	int i = 0;
	int rc = 0;
	char buf[100];
	int buflen = sizeof(buf);
#define TOPIC_COUNT 2

	int dup = 0;
	int msgid = 23;
	int count = TOPIC_COUNT;
	MQTTString topicStrings[TOPIC_COUNT] = { MQTTString_initializer, MQTTString_initializer };
	int req_qoss[TOPIC_COUNT] = {2, 1};

	int dup2 = 1;
	int msgid2 = 2223;
	int count2 = 0;
	MQTTString topicStrings2[TOPIC_COUNT] = { MQTTString_initializer, MQTTString_initializer };
	int req_qoss2[TOPIC_COUNT] = {0, 0};

	fprintf(xml, "<testcase classname=\"test1\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 2 - serialization of subscribe and back");

	topicStrings[0].cstring = "mytopic";
	topicStrings[1].cstring = "mytopic2";
	rc = MQTTSerialize_subscribe(buf, buflen, dup, msgid, count, topicStrings, req_qoss);
	assert("good rc from serialize subscribe", rc > 0, "rc was %d\n", rc);

	rc = MQTTDeserialize_subscribe(&dup2, &msgid2, 2, &count2, topicStrings2, req_qoss2, buf, buflen);
	assert("good rc from deserialize subscribe", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("dups should be the same", dup == dup2, "dups were different %d\n", dup2);
	assert("msgids should be the same", msgid == msgid2, "msgids were different %d\n", msgid2);

	assert("count should be the same", count == count2, "counts were different %d\n", count2);

	for (i = 0; i < count2; ++i)
	{
		assert("topics should be the same",
					checkMQTTStrings(topicStrings[i], topicStrings2[i]), "topics were different %s\n", "");

		assert("qoss should be the same", req_qoss[i] == req_qoss2[i], "qoss were different %d\n", req_qoss2[i]);
	}

/*exit:*/
	MyLog(LOGA_INFO, "TEST3: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test4(struct Options options)
{
	int i = 0;
	int rc = 0;
	char buf[100];
	int buflen = sizeof(buf);
#define TOPIC_COUNT 2

	int msgid = 23;
	int count = TOPIC_COUNT;
	int granted_qoss[TOPIC_COUNT] = {2, 1};
;
	int msgid2 = 2223;
	int count2 = 0;
	int granted_qoss2[TOPIC_COUNT] = {0, 0};

	fprintf(xml, "<testcase classname=\"test1\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 4 - serialization of suback and back");

	rc = MQTTSerialize_suback(buf, buflen, msgid, count, granted_qoss);
	assert("good rc from serialize suback", rc > 0, "rc was %d\n", rc);

	rc = MQTTDeserialize_suback(&msgid2, 2, &count2, granted_qoss2, buf, buflen);
	assert("good rc from deserialize suback", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("msgids should be the same", msgid == msgid2, "msgids were different %d\n", msgid2);

	assert("count should be the same", count == count2, "counts were different %d\n", count2);

	for (i = 0; i < count2; ++i)
		assert("qoss should be the same", granted_qoss[i] == granted_qoss2[i], "qoss were different %d\n", granted_qoss2[i]);

/* exit: */
	MyLog(LOGA_INFO, "TEST4: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test5(struct Options options)
{
	int i = 0;
	int rc = 0;
	char buf[100];
	int buflen = sizeof(buf);
#define TOPIC_COUNT 2

	int dup = 0;
	int msgid = 23;
	int count = TOPIC_COUNT;
	MQTTString topicStrings[TOPIC_COUNT] = { MQTTString_initializer, MQTTString_initializer };

	int dup2 = 1;
	int msgid2 = 2223;
	int count2 = 0;
	MQTTString topicStrings2[TOPIC_COUNT] = { MQTTString_initializer, MQTTString_initializer };

	fprintf(xml, "<testcase classname=\"test1\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 2 - serialization of unsubscribe and back");

	topicStrings[0].cstring = "mytopic";
	topicStrings[1].cstring = "mytopic2";
	rc = MQTTSerialize_unsubscribe(buf, buflen, dup, msgid, count, topicStrings);
	assert("good rc from serialize unsubscribe", rc > 0, "rc was %d\n", rc);

	rc = MQTTDeserialize_unsubscribe(&dup2, &msgid2, 2, &count2, topicStrings2, buf, buflen);
	assert("good rc from deserialize unsubscribe", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("dups should be the same", dup == dup2, "dups were different %d\n", dup2);
	assert("msgids should be the same", msgid == msgid2, "msgids were different %d\n", msgid2);

	assert("count should be the same", count == count2, "counts were different %d\n", count2);

	for (i = 0; i < count2; ++i)
		assert("topics should be the same",
					checkMQTTStrings(topicStrings[i], topicStrings2[i]), "topics were different %s\n", "");

/* exit: */
	MyLog(LOGA_INFO, "TEST5: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}
#endif


int test6(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);

	int connack_rc = 77;

	int connack_rc2 = 0;

	fprintf(xml, "<testcase classname=\"test1\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 2 - serialization of connack and back");

	rc = MQTTSNSerialize_connack(buf, buflen, connack_rc);
	assert("good rc from serialize connack", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_connack(&connack_rc2, buf, buflen);
	assert("good rc from deserialize connack", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("dups should be the same", connack_rc == connack_rc2, "dups were different %d\n", connack_rc2);

/* exit: */
	MyLog(LOGA_INFO, "TEST6: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}

#endif

int main(int argc, char** argv)
{
	int rc = 0;
 	int (*tests[])() = {NULL, test1, test2};

	xml = fopen("TEST-test1.xml", "w");
	fprintf(xml, "<testsuite name=\"test2\" tests=\"%d\">\n", (int)(ARRAY_SIZE(tests) - 1));

	getopts(argc, argv);

 	if (options.test_no == 0)
	{ /* run all the tests */
 	   	for (options.test_no = 1; options.test_no < ARRAY_SIZE(tests); ++options.test_no)
			rc += tests[options.test_no](options); /* return number of failures.  0 = test succeeded */
	}
	else
 	   	rc = tests[options.test_no](options); /* run just the selected test */

 	if (rc == 0)
		MyLog(LOGA_INFO, "verdict pass");
	else
		MyLog(LOGA_INFO, "verdict fail");

	fprintf(xml, "</testsuite>\n");
	fclose(xml);
	return rc;
}
