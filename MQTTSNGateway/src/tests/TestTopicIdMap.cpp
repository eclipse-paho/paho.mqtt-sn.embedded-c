/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi
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
 **************************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include "TestTopicIdMap.h"

using namespace std;
using namespace MQTTSNGW;

TestTopicIdMap::TestTopicIdMap()
{
	_map = new TopicIdMap();
}

TestTopicIdMap::~TestTopicIdMap()
{
	delete _map;
}


bool TestTopicIdMap::testGetElement(uint16_t msgid, uint16_t id, MQTTSN_topicid* topic)
{
    TopicIdMapElement* elm = _map->getElement((uint16_t)msgid );
    if ( elm )
    {
       //printf("msgid=%d id=%d type=%d\n", msgid, elm->getTopicId(), elm->getTopicType());
       return elm->getTopicId() == id && elm->getTopicType() == topic->type;
    }
    //printf("msgid=%d\n", msgid);
    return false;
}

#define MAXID 30

void TestTopicIdMap::test(void)
{
	uint16_t id[MAXID];
	MQTTSN_topicid topicId;
	topicId.data.long_.name = const_cast<char*>("topic/test");
	topicId.type = MQTTSN_TOPIC_TYPE_NORMAL;

	for ( int i = 0; i < MAXID; i++ )
	{
		id[i] = i + 1;

		_map->add(id[i], id[i], &topicId);
	}

	for ( int i = 0; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
	{
		assert(testGetElement(id[i], id[i], &topicId));
	}

	for ( int i = MAX_INFLIGHTMESSAGES * 2 + 1; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

	topicId.type = MQTTSN_TOPIC_TYPE_PREDEFINED;
    for ( int i = 0; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

	for ( int i = 0; i < 5; i++ )
	{
		_map->erase(id[i]);
	}

	topicId.type = MQTTSN_TOPIC_TYPE_NORMAL;
	for ( int i = 0; i < 5; i++ )
	{

	    assert(!testGetElement(id[i], id[i],  &topicId));
	}

	for ( int i = 5; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(testGetElement(id[i], id[i],  &topicId));
    }

	for ( int i = MAX_INFLIGHTMESSAGES * 2 + 1; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i],  &topicId));
    }

	_map->clear();

    for ( int i = 0; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i],  &topicId));
    }

    topicId.type = MQTTSN_TOPIC_TYPE_SHORT;

    for ( int i = 0; i < MAXID; i++ )
    {
        _map->add(id[i], id[i], &topicId);
    }

    for ( int i = 0; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = MAX_INFLIGHTMESSAGES * 2 + 1; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    topicId.type = MQTTSN_TOPIC_TYPE_NORMAL;
    for ( int i = 0; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = 0; i < 5; i++ )
    {
        _map->erase(id[i]);
    }

    topicId.type = MQTTSN_TOPIC_TYPE_SHORT;
    for ( int i = 0; i < 5; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = 5; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = MAX_INFLIGHTMESSAGES * 2 + 1; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    _map->clear();

    for ( int i = 0; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    topicId.type = MQTTSN_TOPIC_TYPE_PREDEFINED;
    for ( int i = 0; i < MAXID; i++ )
    {
        _map->add(id[i], id[i], &topicId);
    }

    for ( int i = 0; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = MAX_INFLIGHTMESSAGES * 2 + 1; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    topicId.type = MQTTSN_TOPIC_TYPE_SHORT;
    for ( int i = 0; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = 0; i < 5; i++ )
    {
        _map->erase(id[i]);
    }

    topicId.type = MQTTSN_TOPIC_TYPE_PREDEFINED;
    for ( int i = 0; i < 5; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = 5; i < MAX_INFLIGHTMESSAGES * 2 + 1; i++ )
    {
        assert(testGetElement(id[i], id[i], &topicId));
    }

    for ( int i = MAX_INFLIGHTMESSAGES * 2 + 1; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }

    _map->clear();

    for ( int i = 0; i < MAXID; i++ )
    {
        assert(!testGetElement(id[i], id[i], &topicId));
    }
	printf("[ OK ]\n");
}

