/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi, Ian Craggs
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
 *    Tomoaki Yamaguchi - initial API and implementation
 *    Tieto Poland Sp. z o.o. - Topic test improvements
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <cassert>
#include "TestTopics.h"

using namespace std;
using namespace MQTTSNGW;

TestTopics::TestTopics()
{
	_topics = new Topics();
}

TestTopics::~TestTopics()
{
	delete _topics;
}

bool testIsMatch(const char* topicFilter, const char* topicName)
{
	string* filter = new string(topicFilter);
	string* name = new string(topicName);

	Topic topic(filter, MQTTSN_TOPIC_TYPE_NAME);
	bool isMatch = topic.isMatch(name);

	delete name;

	return isMatch;
}

bool testGetTopicByName(const char* topicName, const char* searchedTopicName)
{
	Topics topics;
	MQTTSN_topic topicid, serchId;
	topicid.type = MQTTSN_TOPIC_TYPE_NAME;
	topicid.alt.string.len = strlen(topicName);
	topicid.alt.string.data = const_cast<char*>(topicName);

	topics.add(&topicid);

	serchId.type = MQTTSN_TOPIC_TYPE_NAME;
	serchId.alt.string.len = strlen(searchedTopicName);
	serchId.alt.string.data = const_cast<char*>(searchedTopicName);

	return topics.getTopicByName(&serchId) != 0;
}

bool testGetTopicById(const char* topicName, const char* searchedTopicName)
{
	Topics topics;
	MQTTSN_topic topicid, stopicid;
	topicid.type = MQTTSN_TOPIC_TYPE_NAME;
	topicid.alt.string.len = strlen(topicName);
	topicid.alt.string.data = const_cast<char*>(topicName);
	stopicid.type = MQTTSN_TOPIC_TYPE_NAME;
	stopicid.alt.string.len = strlen(searchedTopicName);
	stopicid.alt.string.data = const_cast<char*>(searchedTopicName);

	Topic* tp = topics.add(&topicid);
	Topic* stp = topics.add(&stopicid);

	MQTTSN_topic byId, sById;
	byId.type = MQTTSN_TOPIC_TYPE_NAME;
	byId.alt.alias = tp->getTopicId();
	sById.type = MQTTSN_TOPIC_TYPE_NAME;
	sById.alt.alias = stp->getTopicId();

	stp = topics.getTopicById(&sById);

	return stp->getTopicId() == tp->getTopicId();
}

bool testGetPredefinedTopicByName(const char* topicName, const uint16_t id, const char* searchedTopicName)
{
    Topics topics;
    MQTTSN_topic topicid;

    topics.add(topicName, id);

    topicid.type = MQTTSN_TOPIC_TYPE_PREDEFINED;
    topicid.alt.string.len = strlen(searchedTopicName);
    topicid.alt.string.data = const_cast<char*>(searchedTopicName);

    return topics.getTopicByName(&topicid) != 0;
}

bool testGetPredefinedTopicById(const char* topicName, const uint16_t id, uint16_t sid)
{
    Topics topics;
    MQTTSN_topic topicid;

    Topic* t = topics.add(topicName, id);

    topicid.type = MQTTSN_TOPIC_TYPE_PREDEFINED;
    topicid.alt.alias = sid;

    Topic* tp = topics.getTopicById(&topicid);

    if (tp)
    {
        return tp->getTopicId() == id && strcmp(t->getTopicName()->c_str(), topicName) == 0;
    }
    else
    {
        return false;
    }
}

void TestTopics::test(void)
{
	const int TOPIC_COUNT = 13;

	MQTTSN_topic topic[TOPIC_COUNT];
	char tp[TOPIC_COUNT][10];

	/* create Topic */
	strcpy(tp[0], "Topic/+");
	tp[0][7] = 0;
	topic[0].type = MQTTSN_TOPIC_TYPE_NAME;
	topic[0].alt.string.len = strlen(tp[0]);
	topic[0].alt.string.data = tp[0];

	for (int i = 1; i < 10; i++)
	{
		snprintf(tp[i], sizeof(tp[i]), "Topic/+/%d", i);
		topic[i].type = MQTTSN_TOPIC_TYPE_NAME;
		topic[i].alt.string.len = strlen(tp[i]);
		topic[i].alt.string.data = tp[i];
	}

	strcpy(tp[10], "TOPIC/#");
	tp[10][7] = 0;
	topic[10].type = MQTTSN_TOPIC_TYPE_NAME;
	topic[10].alt.string.len = strlen(tp[10]);
	topic[10].alt.string.data = tp[10];

	strcpy(tp[11], "+/0/#");
	tp[11][7] = 0;
	topic[11].type = MQTTSN_TOPIC_TYPE_NAME;
	topic[11].alt.string.len = strlen(tp[11]);
	topic[11].alt.string.data = tp[11];

	tp[12][0] = '#';
	tp[12][1] = 0;
	topic[12].type = MQTTSN_TOPIC_TYPE_NAME;
	topic[12].alt.string.len = strlen(tp[12]);
	topic[12].alt.string.data = tp[12];


	/* Test EraseNormal() */
	for (int i = 0; i < TOPIC_COUNT; i++)
	{
		MQTTSN_topic pos = topic[i];
		Topic* t = _topics->add(&pos);
		assert(t != 0);
	}
	_topics->eraseNormal();
	assert(_topics->getCount() == 0);

	/* Add Topic to Topics */
	for (int i = 0; i < TOPIC_COUNT; i++)
	{
		MQTTSN_topic pos = topic[i];
		Topic* t = _topics->add(&pos);
		assert(t != 0);
	}

	for (int i = 0; i < 5; i++)
	{
		string str = "Test/";
		str += 0x30 + i;
		Topic* t = _topics->add(str.c_str());
		assert(t != 0);
	}

	/* Get Topic by MQTTSN_topic by Name */
	for (int i = 0; i < TOPIC_COUNT; i++)
	{
		Topic* t = _topics->getTopicByName(&topic[i]);
		assert(strcmp(t->getTopicName()->c_str(), topic[i].alt.string.data) == 0);
	}

	/* Get Topic by MQTTSN_topic by ID */
	for (int i = 0; i < TOPIC_COUNT; i++)
	{
		Topic* t = _topics->getTopicByName(&topic[i]);
		MQTTSN_topic stpid;
		stpid.type = MQTTSN_TOPIC_TYPE_NAME;
		stpid.alt.alias = t->getTopicId();
		Topic* st = _topics->getTopicById(&stpid);
		assert(t->getTopicId() == st->getTopicId());
	}

	/* Test Wildcard */
	for (int i = 0; i < 10; i++)
	{
		MQTTSN_topic tp1;
		char tp0[20];
		snprintf(tp0, sizeof(tp0), "Topic/%d/%d", i, i);
		tp1.type = MQTTSN_TOPIC_TYPE_NAME;
		tp1.alt.string.len = strlen(tp0);
		tp1.alt.string.data = tp0;

		Topic* t = _topics->match(&tp1);
		assert(t != 0);
	}

	for (int i = 0; i < 10; i++)
	{
		MQTTSN_topic tp1;
		char tp0[20];
		snprintf(tp0, sizeof(tp0), "Topic/%d", i);
		tp1.type = MQTTSN_TOPIC_TYPE_NAME;
		tp1.alt.string.len = strlen(tp0);
		tp1.alt.string.data = tp0;

		Topic* t = _topics->match(&tp1);
		assert(t != 0);
		assert(t->getTopicName()->compare(tp[0]) == 0);
	}

	for (int i = 0; i < 10; i++)
	{
		MQTTSN_topic tpid1;
		char tp0[20];
		snprintf(tp0, sizeof(tp0), "TOPIC/%d/%d", i, i);
		tpid1.type = MQTTSN_TOPIC_TYPE_NAME;
		tpid1.alt.string.len = strlen(tp0);
		tpid1.alt.string.data = tp0;

		Topic* t = _topics->match(&tpid1);
		assert(t != 0);
		assert(t->getTopicName()->compare(tp[10]) == 0);
	}

	{
		MQTTSN_topic tp1;
		char tp0[10];
		strcpy(tp0, "Topic");
		tp1.type = MQTTSN_TOPIC_TYPE_NAME;
		tp1.alt.string.len = strlen(tp0);
		tp1.alt.string.data = tp0;

		Topic* t = _topics->match(&tp1);
		assert(t != 0);
		assert(t->getTopicName()->compare(tp[12]) == 0);
	}

	{
		MQTTSN_topic tp1;
		char tp0[20];
		strcpy(tp0, "Topic/multi/level");
		tp1.type = MQTTSN_TOPIC_TYPE_NAME;
		tp1.alt.string.len = strlen(tp0);
		tp1.alt.string.data = tp0;

		Topic* t = _topics->match(&tp1);
		assert(t != 0);
		assert(t->getTopicName()->compare(tp[12]) == 0);
	}


	assert(testIsMatch("#", "one"));
	assert(testIsMatch("#", "one/"));
	assert(testIsMatch("#", "one/two"));
	assert(testIsMatch("#", "one/two/"));
	assert(testIsMatch("#", "one/two/three"));
	assert(testIsMatch("#", "one/two/three/"));

	assert(!testIsMatch("one/+", "one"));
	assert(testIsMatch("one/+", "one/"));
	assert(testIsMatch("one/+", "one/two"));
	assert(!testIsMatch("one/+", "one/two/"));
	assert(!testIsMatch("one/+", "one/two/three"));

	assert(!testIsMatch("one/+/three/+", "one/two/three"));
	assert(testIsMatch("one/+/three/+", "one/two/three/"));
	assert(testIsMatch("one/+/three/+", "one/two/three/four"));
	assert(!testIsMatch("one/+/three/+", "one/two/three/four/"));

	assert(testIsMatch("one/+/three/#", "one/two/three"));
	assert(testIsMatch("one/+/three/#", "one/two/three/"));
	assert(testIsMatch("one/+/three/#", "one/two/three/four"));
	assert(testIsMatch("one/+/three/#", "one/two/three/four/"));
	assert(testIsMatch("one/+/three/#", "one/two/three/four/five"));

	assert(testIsMatch("sport/tennis/player1/#", "sport/tennis/player1"));
	assert(testIsMatch("sport/tennis/player1/#", "sport/tennis/player1/ranking"));
	assert(testIsMatch("sport/tennis/player1/#", "sport/tennis/player1/score/wimbledon"));
	assert(testIsMatch("sport/tennis/+", "sport/tennis/player1"));
	assert(testIsMatch("sport/tennis/+", "sport/tennis/player2"));
	assert(!testIsMatch("sport/tennis/+", "sport/tennis/player1/ranking"));
	assert(testIsMatch("+/+", "/finance"));
	assert(testIsMatch("/+", "/finance"));
	assert(!testIsMatch("+", "/finance"));

	assert(testGetTopicById("mytopic", "mytopic"));
	assert(!testGetTopicById("mytopic", "mytop"));
	assert(!testGetTopicById("mytopic", "mytopiclong"));

	assert(testGetTopicByName("mytopic", "mytopic"));
	assert(!testGetTopicByName("mytopic", "mytop"));
	assert(!testGetTopicByName("mytopic", "mytopiclong"));

	assert(testGetPredefinedTopicByName("mypretopic", 1, "mypretopic"));
	assert(!testGetPredefinedTopicByName("mypretopic", 1, "mypretop"));
	assert(!testGetPredefinedTopicByName("mypretopic", 1, "mypretopiclong"));

	assert(testGetPredefinedTopicById("mypretopic2", 2, 2));
	assert(!testGetPredefinedTopicById("mypretopic2", 2, 1));

	printf("[ OK ]\n");
}
