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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(_WINDOWS)
	#include <sys/time.h>
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

struct Options
{
	char* connection;         /**< connection to system under test. */
	char** haconnections;
	int hacount;
	int verbose;
	int test_no;
} options =
{
	"tcp://m2m.eclipse.org:1883",
	NULL,
	0,
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
		else if (strcmp(argv[count], "--connection") == 0)
		{
			if (++count < argc)
			{
				options.connection = argv[count];
				printf("\nSetting connection to %s\n", options.connection);
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--haconnections") == 0)
		{
			if (++count < argc)
			{
				char* tok = strtok(argv[count], " ");
				options.hacount = 0;
				options.haconnections = malloc(sizeof(char*) * 5);
				while (tok)
				{
					options.haconnections[options.hacount] = malloc(strlen(tok) + 1);
					strcpy(options.haconnections[options.hacount], tok);
					options.hacount++;
					tok = strtok(NULL, " ");
				}
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

int checkMQTTSNStrings(MQTTSNString a, MQTTSNString b)
{
	if (!a.lenstring.data)
	{
		a.lenstring.data = a.cstring;
		if (a.cstring)
			a.lenstring.len = strlen(a.cstring);
	}
	if (!b.lenstring.data)
	{
		b.lenstring.data = b.cstring;
		if (b.cstring)
			b.lenstring.len = strlen(b.cstring);
	}
	return memcmp(a.lenstring.data, b.lenstring.data, min(a.lenstring.len, b.lenstring.len)) == 0;
}

int checkMQTTSNTopics(MQTTSN_topicid a, MQTTSN_topicid b)
{
	int rc = 0;

	if (a.type != b.type)
		goto exit;

	if (a.data.long_.name)
		rc = memcmp(a.data.long_.name, b.data.long_.name, a.data.long_.len) == 0;
	else if (a.type == MQTTSN_TOPIC_TYPE_SHORT)
		rc = (memcpy(a.data.short_name, b.data.short_name, 2) == 0);
	else
		rc = (a.data.id == b.data.id);

exit:
	return rc;
}


int checkConnectPackets(MQTTSNPacket_connectData* before, MQTTSNPacket_connectData* after)
{
	int rc = 0;
	int start_failures = failures;

	assert("struct_ids should be the same",
			memcmp(before->struct_id, after->struct_id, 4) == 0, "struct_ids were different %.4s\n", after->struct_id);

	assert("struct_versions should be the same",
			before->struct_version == after->struct_version, "struct_versions were different\n", rc);

	assert("ClientIDs should be the same",
			checkMQTTSNStrings(before->clientID, after->clientID), "ClientIDs were different\n", rc);

	assert("durations should be the same",
			before->duration == after->duration, "durations were different\n", rc);

	assert("cleansessions should be the same",
			before->cleansession == after->cleansession, "cleansessions were different\n", rc);

	assert("willFlags should be the same",
				before->willFlag == after->willFlag, "willFlags were different\n", rc);

	return failures == start_failures;
}

int test1(struct Options options)
{
	MQTTSNPacket_connectData data = MQTTSNPacket_connectData_initializer;
	MQTTSNPacket_connectData data_after = MQTTSNPacket_connectData_initializer;
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	MQTTSNString clientid = MQTTSNString_initializer, clientid_after = MQTTSNString_initializer;
	int duration_after = -1;

	fprintf(xml, "<testcase classname=\"test1\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 1 - serialization of connect and back");

	data.clientID.cstring = "me too";

	data.duration = 20;
	data.cleansession = 1;

	data.willFlag = 1;

	rc = MQTTSNSerialize_connect(buf, buflen, &data);
	assert("good rc from serialize connect", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_connect(&data_after, buf, buflen);
	assert("good rc from deserialize connect", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	rc = checkConnectPackets(&data, &data_after);
	assert("packets should be the same",  rc == 1, "packets were different\n", rc);

	/* Pingreq without clientid */
	rc = MQTTSNSerialize_pingreq(buf, buflen, clientid);
	assert("good rc from serialize pingreq", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_pingreq(&clientid_after, buf, buflen);
	assert("good rc from deserialize pingreq", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("ClientIDs should be the same",
			checkMQTTSNStrings(clientid, clientid_after), "ClientIDs were different\n", rc);

	/* Pingreq with clientid */
	clientid.cstring = "this is me";
	rc = MQTTSNSerialize_pingreq(buf, buflen, clientid);
	assert("good rc from serialize pingreq", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_pingreq(&clientid_after, buf, buflen);
	assert("good rc from deserialize pingreq", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("ClientIDs should be the same",
			checkMQTTSNStrings(clientid, clientid_after), "ClientIDs were different\n", rc);

	rc = MQTTSNSerialize_pingresp(buf, buflen);
	assert("good rc from serialize pingresp", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_pingresp(buf, buflen);
	assert("good rc from deserialize pingresp", rc == 1, "rc was %d\n", rc);

	/* Disconnect without duration */
	rc = MQTTSNSerialize_disconnect(buf, buflen, 0);
	assert("good rc from serialize disconnect", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_disconnect(&duration_after, buf, buflen);
	assert("good rc from deserialize disconnect", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("durations should be the same", -1 == duration_after, "durations were different\n", rc);

	/* Disconnect with duration */
	rc = MQTTSNSerialize_disconnect(buf, buflen, 33);
	assert("good rc from serialize disconnect", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_disconnect(&duration_after, buf, buflen);
	assert("good rc from deserialize disconnect", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("durations should be the same", 33 == duration_after, "durations were different\n", rc);

	/* Pingreq with clientid */
	clientid.cstring = "this is me";
	rc = MQTTSNSerialize_pingreq(buf, buflen, clientid);
	assert("good rc from serialize pingreq", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_pingreq(&clientid_after, buf, buflen);
	assert("good rc from deserialize pingreq", rc == 1, "rc was %d\n", rc);

/* exit: */
	MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test2(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);

	unsigned char dup = 0;
	int qos = 2;
	unsigned char retained = 0;
	unsigned short msgid = 23;
	MQTTSN_topicid topic;
	unsigned char *payload = (unsigned char*)"kkhkhkjkj jkjjk jk jk ";
	int payloadlen = strlen((char*)payload);

	unsigned char dup2 = 1;
	int qos2 = 1;
	unsigned char retained2 = 1;
	unsigned short msgid2 = 3243;
	MQTTSN_topicid topic2;
	unsigned char *payload2 = NULL;
	int payloadlen2 = 0;
	unsigned char acktype;

	unsigned char returncode = 3, returncode2 = -99;

	fprintf(xml, "<testcase classname=\"test1\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 2 - serialization of publish and back");

	memset(&topic, 0, sizeof(topic));
	memset(&topic2, 0, sizeof(topic2));
	topic.type = MQTTSN_TOPIC_TYPE_SHORT;
	memcpy(topic.data.short_name, "my", 2);
	rc = MQTTSNSerialize_publish(buf, buflen, dup, qos, retained, msgid, topic,
			payload, payloadlen);
	assert("good rc from serialize publish", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_publish(&dup2, &qos2, &retained2, &msgid2, &topic2,
			&payload2, &payloadlen2, buf, buflen);
	assert("good rc from deserialize publish", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("dups should be the same", dup == dup2, "dups were different %d\n", dup2);
	assert("qoss should be the same", qos == qos2, "qoss were different %d\n", qos2);
	assert("retaineds should be the same", retained == retained2, "retaineds were different %d\n", retained2);
	assert("msgids should be the same", msgid == msgid2, "msgids were different %d\n", msgid2);

	assert("topics should be the same",
			checkMQTTSNTopics(topic, topic2), "topics were different %s\n", "");

	assert("payload lengths should be the same",
				payloadlen == payloadlen2, "payload lengths were different %d\n", payloadlen2);

	assert("payloads should be the same",
						memcmp(payload, payload2, payloadlen) == 0, "payloads were different %s\n", "");

	rc = MQTTSNSerialize_puback(buf, buflen, topic.data.id, msgid, returncode);
	assert("good rc from serialize puback", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_puback(&topic2.data.id, &msgid2, &returncode2, buf, buflen);
	assert("good rc from deserialize puback", rc > 0, "rc was %d\n", rc);
	assert("msgids should be the same", msgid == msgid2, "msgids were different %d\n", msgid2);
	assert("return codes should be the same", returncode == returncode2, "return codes were different %d\n", returncode2);

	rc = MQTTSNSerialize_pubrec(buf, buflen, msgid);
	assert("good rc from serialize pubrec", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_ack(&acktype, &msgid2, buf, buflen);
	assert("good rc from deserialize pubrec", rc == 1, "rc was %d\n", rc);
	assert("Acktype should be MQTTSN_PUBREC", acktype == MQTTSN_PUBREC, "acktype was %d\n", acktype);
	assert("msgids should be the same", msgid == msgid2, "msgids were different %d\n", msgid2);

/*exit:*/
	MyLog(LOGA_INFO, "TEST2: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


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

	memset(buf, '\0', sizeof(buf));
	int willQoS = 1, willQoS1 = 0;
	unsigned char willRetain = 1, willRetain1 = 0;
	MQTTSNString willTopic = MQTTSNString_initializer, willTopic1 = MQTTSNString_initializer;
	willTopic.cstring = "a will topic";
	rc = MQTTSNSerialize_willtopic(buf, buflen, willQoS, willRetain, willTopic);
	assert("good rc from serialize willtopic", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willtopic(&willQoS1, &willRetain1, &willTopic1, buf, rc);
	assert("good rc from deserialize willtopic", rc == 1, "rc was %d\n", rc);
	assert("willQoSs are the same", willQoS == willQoS1, "willQoS1 was %d\n", willQoS1);
	assert("willRetains are the same", willRetain == willRetain1, "willRetain1 was %d\n", willRetain1);
	assert("willTopics are the same", checkMQTTSNStrings(willTopic, willTopic1), "willTopic1 was %.s\n", willTopic1.lenstring.data);

	memset(buf, '\0', sizeof(buf));
	willQoS = 2; willRetain = 1; willQoS1 = 0; willRetain1 = 0;
	MQTTSNString initTopic = MQTTSNString_initializer;
	memcpy(&willTopic, &initTopic, sizeof(initTopic));
	memcpy(&willTopic1, &initTopic, sizeof(initTopic));
	willTopic.cstring = "a will topic update";
	rc = MQTTSNSerialize_willtopicupd(buf, buflen, willQoS, willRetain, willTopic);
	assert("good rc from serialize willtopicupd", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willtopicupd(&willQoS1, &willRetain1, &willTopic1, buf, rc);
	assert("good rc from deserialize willtopicupd", rc == 1, "rc was %d\n", rc);
	assert("willQoSs are the same", willQoS == willQoS1, "willQoS1 was %d\n", willQoS1);
	assert("willRetains are the same", willRetain == willRetain1, "willRetain1 was %d\n", willRetain1);
	assert("willTopics are the same", checkMQTTSNStrings(willTopic, willTopic1), "willTopic1 was %.s\n", willTopic1.lenstring.data);

	memset(buf, '\0', sizeof(buf));
	MQTTSNString willMsg = MQTTSNString_initializer, willMsg1 = MQTTSNString_initializer;
	willMsg.cstring = "a will message";
	rc = MQTTSNSerialize_willmsg(buf, buflen, willMsg);
	assert("good rc from serialize willmsg", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willmsg(&willMsg1, buf, rc);
	assert("good rc from deserialize willmsg", rc == 1, "rc was %d\n", rc);
	assert("willMsgs are the same", checkMQTTSNStrings(willMsg, willMsg1), "willMsg1 was %.s\n", willMsg1.lenstring.data);

	memset(buf, '\0', sizeof(buf));
	memcpy(&willMsg, &initTopic, sizeof(initTopic));
	memcpy(&willMsg1, &initTopic, sizeof(initTopic));
	willMsg.cstring = "a will message";
	rc = MQTTSNSerialize_willmsgupd(buf, buflen, willMsg);
	assert("good rc from serialize willmsgupd", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willmsgupd(&willMsg1, buf, rc);
	assert("good rc from deserialize willmsgupd", rc == 1, "rc was %d\n", rc);
	assert("willMsgs are the same", checkMQTTSNStrings(willMsg, willMsg1), "willMsg1 was %.s\n", willMsg1.lenstring.data);

	int resp_rc = 33, resp_rc2 = 0;
	rc = MQTTSNSerialize_willmsgresp(buf, buflen, resp_rc);
	assert("good rc from serialize willmsgresp", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willmsgresp(&resp_rc2, buf, buflen);
	assert("good rc from deserialize willmsgresp", rc == 1, "rc was %d\n", rc);
	assert("resp rcs should be the same", resp_rc == resp_rc2, "resp rcs were different %d\n", resp_rc2);

	resp_rc = 67, resp_rc2 = 0;
	rc = MQTTSNSerialize_willtopicresp(buf, buflen, resp_rc);
	assert("good rc from serialize willmsgresp", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_willtopicresp(&resp_rc2, buf, buflen);
	assert("good rc from deserialize willmsgresp", rc == 1, "rc was %d\n", rc);
	assert("resp rcs should be the same", resp_rc == resp_rc2, "resp rcs were different %d\n", resp_rc2);

/* exit: */
	MyLog(LOGA_INFO, "TEST3: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}




int test4(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	size_t buflen = sizeof(buf);

	unsigned char dup = 0;
	unsigned short packetid = 23;
	MQTTSN_topicid topicFilter;
	int req_qos = 2;

	unsigned char dup2 = 1;
	unsigned short packetid2 = 2223;
	MQTTSN_topicid topicFilter2;
	int req_qos2 = 0;

	fprintf(xml, "<testcase classname=\"test4\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 4 - serialization of subscribe and back");

	memset(&topicFilter, '\0', sizeof(topicFilter));
	memset(&topicFilter2, '\0', sizeof(topicFilter2));
	topicFilter.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topicFilter.data.long_.name = "mytopic";
	topicFilter.data.long_.len = strlen(topicFilter.data.long_.name);
	rc = MQTTSNSerialize_subscribe(buf, buflen, dup, req_qos, packetid, &topicFilter);
	assert("good rc from serialize subscribe", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_subscribe(&dup2, &req_qos2, &packetid2, &topicFilter2, buf, buflen);
	assert("good rc from deserialize subscribe", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("dups should be the same", dup == dup2, "dups were different %d\n", dup2);
	assert("msgids should be the same", packetid == packetid2, "packetids were different %d\n", packetid2);

	assert("topics should be the same",
					checkMQTTSNTopics(topicFilter, topicFilter2), "topics were different %s\n", "");
	assert("qoss should be the same", req_qos == req_qos2, "qoss were different %d\n", req_qos2);

/*exit:*/
	MyLog(LOGA_INFO, "TEST4: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test5(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	size_t buflen = sizeof(buf);

	unsigned short packetid = 23;
	MQTTSN_topicid topicFilter;

	unsigned short packetid2 = 2223;
	MQTTSN_topicid topicFilter2;

	fprintf(xml, "<testcase classname=\"test5\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 5 - serialization of unsubscribe and back");

	memset(&topicFilter, '\0', sizeof(topicFilter));
	memset(&topicFilter2, '\0', sizeof(topicFilter2));
	topicFilter.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topicFilter.data.long_.name = "mytopic";
	topicFilter.data.long_.len = strlen(topicFilter.data.long_.name);
	rc = MQTTSNSerialize_unsubscribe(buf, buflen, packetid, &topicFilter);
	assert("good rc from serialize unsubscribe", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_unsubscribe(&packetid2, &topicFilter2, buf, buflen);
	assert("good rc from deserialize unsubscribe", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("msgids should be the same", packetid == packetid2, "packetids were different %d\n", packetid2);

	assert("topics should be the same",
					checkMQTTSNTopics(topicFilter, topicFilter2), "topics were different %s\n", "");

/*exit:*/
	MyLog(LOGA_INFO, "TEST5: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test6(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);

	int connack_rc = 77;

	int connack_rc2 = 0;

	fprintf(xml, "<testcase classname=\"test6\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 6 - serialization of connack and back");

	rc = MQTTSNSerialize_connack(buf, buflen, connack_rc);
	assert("good rc from serialize connack", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_connack(&connack_rc2, buf, buflen);
	assert("good rc from deserialize connack", rc == 1, "rc was %d\n", rc);

	/* data after should be the same as data before */
	assert("connack rcs should be the same", connack_rc == connack_rc2, "connack rcs were different %d\n", connack_rc2);

/* exit: */
	MyLog(LOGA_INFO, "TEST6: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test7(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	unsigned short packetid = 255, packetid2 = 0;
	int qos = 2, qos2 = 0;
	unsigned short topicid = 233, topicid2 = 0;
	unsigned char return_code = 32, return_code2 = 0;

	fprintf(xml, "<testcase classname=\"test7\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 7 - serialization of suback and back");

	rc = MQTTSNSerialize_suback(buf, buflen, qos, topicid, packetid, return_code);
	assert("good rc from serialize suback", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_suback(&qos2, &topicid2, &packetid2, &return_code2, buf, buflen);
	assert("good rc from deserialize suback", rc == 1, "rc was %d\n", rc);

	assert("packetids should be the same", packetid == packetid2, "packetids were different %d\n", packetid2);
	assert("qoss should be the same", qos == qos2, "qoss were different %d\n", qos2);
	assert("topicids should be the same", topicid == topicid2, "topicids were different %d\n", topicid2);
	assert("return codes should be the same", return_code == return_code2, "return codes were different %d\n", return_code2);

/* exit: */
	MyLog(LOGA_INFO, "TEST7: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test8(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	unsigned short packetid = 255, packetid2 = 0;

	fprintf(xml, "<testcase classname=\"test8\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 8 - serialization of unsuback and back");

	rc = MQTTSNSerialize_unsuback(buf, buflen, packetid);
	assert("good rc from serialize unsuback", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_unsuback(&packetid2, buf, buflen);
	assert("good rc from deserialize unsuback", rc == 1, "rc was %d\n", rc);

	assert("packetids should be the same", packetid == packetid2, "packetids were different %d\n", packetid2);

/* exit: */
	MyLog(LOGA_INFO, "TEST8: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test9(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	unsigned short packetid = 255, packetid2 = 0;
	unsigned short topicid = 233, topicid2 = 0;
	MQTTSNString topicname = MQTTSNString_initializer, topicname2 = MQTTSNString_initializer;

	fprintf(xml, "<testcase classname=\"test9\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 9 - serialization of register and back");

	rc = MQTTSNSerialize_register(buf, buflen, topicid, packetid, &topicname);
	assert("good rc from serialize register", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_register(&topicid2, &packetid2, &topicname2, buf, buflen);
	assert("good rc from deserialize register", rc == 1, "rc was %d\n", rc);

	assert("packetids should be the same", packetid == packetid2, "packetids were different %d\n", packetid2);
	assert("topicids should be the same", topicid == topicid2, "topicids were different %d\n", topicid2);
	assert("topicnames should be the same",
			checkMQTTSNStrings(topicname, topicname2), "topicnames were different\n", rc);


/* exit: */
	MyLog(LOGA_INFO, "TEST9: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test10(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	unsigned short packetid = 255, packetid2 = 0;
	unsigned short topicid = 233, topicid2 = 0;
	unsigned char return_code = 127, return_code2 = 0;

	fprintf(xml, "<testcase classname=\"test10\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 10 - serialization of regack and back");

	rc = MQTTSNSerialize_regack(buf, buflen, topicid, packetid, return_code);
	assert("good rc from serialize regack", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_regack(&topicid2, &packetid2, &return_code2, buf, buflen);
	assert("good rc from deserialize regack", rc == 1, "rc was %d\n", rc);

	assert("packetids should be the same", packetid == packetid2, "packetids were different %d\n", packetid2);
	assert("topicids should be the same", topicid == topicid2, "topicids were different %d\n", topicid2);
	assert("return codes should be the same", return_code == return_code2, "return_codes were different %d\n", return_code2);

/* exit: */
	MyLog(LOGA_INFO, "TEST10: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test11(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	unsigned char gatewayid = 255, gatewayid2 = 0;
	unsigned short duration = 3233, duration2 = 0;

	fprintf(xml, "<testcase classname=\"test11\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 11 - serialization of advertise and back");

	rc = MQTTSNSerialize_advertise(buf, buflen, gatewayid, duration);
	assert("good rc from serialize advertise", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_advertise(&gatewayid2, &duration2, buf, buflen);
	assert("good rc from deserialize advertise", rc == 1, "rc was %d\n", rc);

	assert("gatewayids should be the same", gatewayid == gatewayid2, "gatewayids were different %d\n", gatewayid2);
	assert("return codes should be the same", duration == duration2, "return_codes were different %d\n", duration2);

/* exit: */
	MyLog(LOGA_INFO, "TEST11: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test12(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	unsigned char radius = 255, radius2 = 0;

	fprintf(xml, "<testcase classname=\"test12\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 12 - serialization of searchgw and back");

	rc = MQTTSNSerialize_searchgw(buf, buflen, radius);
	assert("good rc from serialize searchgw", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_searchgw(&radius2, buf, buflen);
	assert("good rc from deserialize searchgw", rc == 1, "rc was %d\n", rc);

	assert("radiuss should be the same", radius == radius2, "radiuss were different %d\n", radius2);

/* exit: */
	MyLog(LOGA_INFO, "TEST12: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int test13(struct Options options)
{
	int rc = 0;
	unsigned char buf[100];
	int buflen = sizeof(buf);
	unsigned char gatewayid = 255, gatewayid2 = 0;
	unsigned short gatewayaddress_len = 16, gatewayaddress_len2 = 0;
	unsigned char gatewayaddress[40] = "a gateway address", *gatewayaddress2 = NULL;

	fprintf(xml, "<testcase classname=\"test11\" name=\"de/serialization\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 13 - serialization of gwinfo and back");

	rc = MQTTSNSerialize_gwinfo(buf, buflen, gatewayid, gatewayaddress_len, gatewayaddress);
	assert("good rc from serialize gwinfo", rc > 0, "rc was %d\n", rc);

	rc = MQTTSNDeserialize_gwinfo(&gatewayid2, &gatewayaddress_len2, &gatewayaddress2, buf, buflen);
	assert("good rc from deserialize gwinfo", rc == 1, "rc was %d\n", rc);

	assert("gatewayids should be the same", gatewayid == gatewayid2, "gatewayids were different %d\n", gatewayid2);
	assert("gateway lengths should be the same", gatewayaddress_len == gatewayaddress_len2, "gateway lengths were different %d\n", gatewayaddress_len2);
	assert("gateway addresses should be the same", memcmp(gatewayaddress, gatewayaddress2, gatewayaddress_len) == 0,
			"gateway addresses were different %.10s\n", gatewayaddress2);

/* exit: */
	MyLog(LOGA_INFO, "TEST13: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


int main(int argc, char** argv)
{
	int rc = 0;
 	int (*tests[])() = {NULL, test1, test2, test3, test4, test5, test6, test7, test8, test9, test10, test11, test12, test13};

	xml = fopen("TEST-test1.xml", "w");
	fprintf(xml, "<testsuite name=\"test1\" tests=\"%d\">\n", (int)(ARRAY_SIZE(tests) - 1));

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
