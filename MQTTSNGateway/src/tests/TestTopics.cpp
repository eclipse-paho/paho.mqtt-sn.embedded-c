/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
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
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation 
 *    Tieto Poland Sp. z o.o. - Topic test improvements
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

	Topic topic(filter, MQTTSN_TOPIC_TYPE_NORMAL);
	bool isMatch = topic.isMatch(name);

	delete name;

	return isMatch;
}

bool testGetTopicByName(const char* topicName, const char* searchedTopicName)
{
	Topics topics;
	MQTTSN_topicid topicid, serchId;
	topicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topicid.data.long_.len = strlen(topicName);
	topicid.data.long_.name = const_cast<char*>(topicName);

	topics.add(&topicid);

	serchId.type = MQTTSN_TOPIC_TYPE_NORMAL;
	serchId.data.long_.len = strlen(searchedTopicName);
	serchId.data.long_.name = const_cast<char*>(searchedTopicName);

	return topics.getTopicByName(&serchId) != 0;
}

bool testGetTopicById(const char* topicName, const char* searchedTopicName)
{
	Topics topics;
	MQTTSN_topicid topicid, stopicid;
	topicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topicid.data.long_.len = strlen(topicName);
	topicid.data.long_.name = const_cast<char*>(topicName);
	stopicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
    stopicid.data.long_.len = strlen(searchedTopicName);
    stopicid.data.long_.name = const_cast<char*>(searchedTopicName);

	Topic* tp = topics.add(&topicid);
	Topic*stp = topics.add(&stopicid);
	topicid.data.id  = tp->getTopicId();
	stopicid.data.id  = stp->getTopicId();

	stp = topics.getTopicById(&stopicid);

	return stp->getTopicId() == tp->getTopicId();
}

bool testGetPredefinedTopicByName(const char* topicName, const uint16_t id, const char* searchedTopicName)
{
    Topics topics;
    MQTTSN_topicid topicid;

    topics.add(topicName, id);

    topicid.type = MQTTSN_TOPIC_TYPE_PREDEFINED;
    topicid.data.long_.len = strlen(searchedTopicName);
    topicid.data.long_.name = const_cast<char*>(searchedTopicName);

    return topics.getTopicByName(&topicid) != 0;
}

bool testGetPredefinedTopicById(const char* topicName, const uint16_t id, uint16_t sid)
{
    Topics topics;
    MQTTSN_topicid topicid;

    Topic* t = topics.add(topicName, id);

    topicid.type = MQTTSN_TOPIC_TYPE_PREDEFINED;
    topicid.data.id  = sid;

    Topic* tp = topics.getTopicById(&topicid);

    if ( tp )
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

	MQTTSN_topicid topic[TOPIC_COUNT];
	char tp[TOPIC_COUNT][10];

	/* create Topic */
	strcpy(tp[0], "Topic/+");
	tp[0][7] = 0;
	topic[0].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[0].data.long_.len = strlen(tp[0]);
	topic[0].data.long_.name = tp[0];

	for ( int i = 1; i < 10 ; i++ )
	{
		sprintf(tp[i], "Topic/+/%d", i);
		topic[i].type = MQTTSN_TOPIC_TYPE_NORMAL;
		topic[i].data.long_.len = strlen(tp[i]);
		topic[i].data.long_.name = tp[i];
	}

	strcpy(tp[10], "TOPIC/#");
	tp[10][7] = 0;
	topic[10].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[10].data.long_.len = strlen(tp[10]);
	topic[10].data.long_.name = tp[10];

	strcpy(tp[11], "+/0/#");
	tp[11][7] = 0;
	topic[11].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[11].data.long_.len = strlen(tp[11]);
	topic[11].data.long_.name = tp[11];

	tp[12][0] = '#';
	tp[12][1] = 0;
	topic[12].type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic[12].data.long_.len = strlen(tp[12]);
	topic[12].data.long_.name = tp[12];


	/* Test EraseNorma() */
	for ( int i = 0; i < TOPIC_COUNT; i++ )
    {
        MQTTSN_topicid pos = topic[i];
        Topic* t = _topics->add(&pos);
        //printf("Topic=%s ID=%d\n", t->getTopicName()->c_str(), t->getTopicId());
        assert(t !=0);
    }
	_topics->eraseNormal();
	assert(_topics->getCount() == 0);

	/* Add Topic to Topics */
	for ( int i = 0; i < TOPIC_COUNT; i++ )
	{
		MQTTSN_topicid pos = topic[i];
		Topic* t = _topics->add(&pos);
		//printf("Topic=%s ID=%d\n", t->getTopicName()->c_str(), t->getTopicId());
		assert(t !=0);
	}

	for ( int i = 0; i < 5; i++ )
	{
		string str = "Test/";
		str += 0x30 + i;
		Topic* t = _topics->add(str.c_str());
		//printf("Topic=%s ID=%d\n", t->getTopicName()->c_str(), t->getTopicId());
		assert(t !=0);
	}

	/* Get Topic by MQTTSN_topicid  by Name*/
	for ( int i = 0; i < TOPIC_COUNT; i++ )
	{
		Topic* t = _topics->getTopicByName(&topic[i]);
		//printf("Topic=%s ID=%d\n", t->getTopicName()->c_str(), t->getTopicId());
		assert(strcmp(t->getTopicName()->c_str(), topic[i].data.long_.name) == 0 );
	}

    /* Get Topic by MQTTSN_topicid  by ID*/
    for ( int i = 0; i < TOPIC_COUNT; i++ )
    {
        Topic* t = _topics->getTopicByName(&topic[i]);
        MQTTSN_topicid stpid;
        stpid.type = MQTTSN_TOPIC_TYPE_NORMAL;
        stpid.data.id =t->getTopicId();
        Topic* st = _topics->getTopicById(&stpid);
        //printf("Topic=%s ID=%d ID=%d\n", t->getTopicName()->c_str(), t->getTopicId(), st->getTopicId());
        assert(t->getTopicId() == st->getTopicId() );
    }

	/* Test Wildcard */
	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tp1;
		char tp0[20];
		sprintf(tp0, "Topic/%d/%d", i, i);
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

		Topic* t = _topics->match(&tp1);
/*
		if (t)
		{
			printf("Topic=%s  match to %s\n", tp0, t->getTopicName()->c_str());
		}
		else
		{
			printf("Topic=%s unmatch\n", tp0);
		}
*/
		assert(t != 0);
	}

	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tp1;
		char tp0[20];
		sprintf(tp0, "Topic/%d", i);
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

		Topic* t = _topics->match(&tp1);
/*
		if (t)
		{
			printf("Topic=%s  match to %s\n", tp0, t->getTopicName()->c_str());
		}
		else
		{
			printf("Topic=%s unmatch\n", tp0);
		}
*/
		assert(t != 0);
		assert(t->getTopicName()->compare(tp[0]) == 0);
	}

	for ( int i = 0; i < 10 ; i++ )
	{
		MQTTSN_topicid tpid1;
		char tp0[20];
		sprintf(tp0, "TOPIC/%d/%d", i, i);
		tpid1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tpid1.data.long_.len = strlen(tp0);
		tpid1.data.long_.name = tp0;

		Topic* t = _topics->match(&tpid1);
/*
		if (t)
		{
			printf("Topic=%s  match to %s\n", tp0, t->getTopicName()->c_str());
		}
		else
		{
			printf("Topic=%s unmatch\n", tp0);
		}
*/
		assert( t != 0);
		assert(t->getTopicName()->compare(tp[10]) == 0);
	}

	{
		MQTTSN_topicid tp1;
		char tp0[10];
		strcpy(tp0, "Topic");
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

		Topic* t = _topics->match(&tp1);

		assert(t != 0);
		assert(t->getTopicName()->compare(tp[12]) == 0);
	}

	{
		MQTTSN_topicid tp1;
		char tp0[20];
		strcpy(tp0, "Topic/multi/level");
		tp1.type = MQTTSN_TOPIC_TYPE_NORMAL;
		tp1.data.long_.len = strlen(tp0);
		tp1.data.long_.name = tp0;

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

	// examples from MQTT specification
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
