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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Tieto Poland Sp. z o.o. - Gateway improvements
 **************************************************************************************/
#include "MQTTSNGWTopic.h"
#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include <string.h>

using namespace MQTTSNGW;

/*=====================================
 Class Topic
 ======================================*/
Topic::Topic()
{
    _type = MQTTSN_TOPIC_TYPE_NORMAL;
    _topicName = nullptr;
    _topicId = 0;
    _next = nullptr;
}

Topic::Topic(string* topic, MQTTSN_topicTypes type)
{
    _type = type;
    _topicName = topic;
    _topicId = 0;
    _next = nullptr;
}

Topic::~Topic()
{
    if (_topicName)
    {
        delete _topicName;
    }
}

string* Topic::getTopicName(void)
{
    return _topicName;
}

uint16_t Topic::getTopicId(void)
{
    return _topicId;
}

MQTTSN_topicTypes Topic::getType(void)
{
    return _type;
}

Topic* Topic::duplicate(void)
{
    Topic* newTopic = new Topic();
    newTopic->_type = _type;
    newTopic->_topicId = _topicId;
    newTopic->_topicName = new string(_topicName->c_str());
    return newTopic;
}

bool Topic::isMatch(string* topicName)
{
    string::size_type tlen = _topicName->size();

    string::size_type tpos = 0;
    string::size_type tloc = 0;
    string::size_type pos = 0;
    string::size_type loc = 0;
    string wildcard = "#";
    string wildcards = "+";

    while (true)
    {
        loc = topicName->find('/', pos);
        tloc = _topicName->find('/', tpos);

        if (loc != string::npos && tloc != string::npos)
        {
            string subtopic = topicName->substr(pos, loc - pos);
            string subtopict = _topicName->substr(tpos, tloc - tpos);
            if (subtopict == wildcard)
            {
                return true;
            }
            else if (subtopict == wildcards)
            {
                if ((tpos = tloc + 1) > tlen)
                {
                    pos = loc + 1;
                    loc = topicName->find('/', pos);
                    if (loc == string::npos)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                pos = loc + 1;
            }
            else if (subtopic != subtopict)
            {
                return false;
            }
            else
            {
                if ((tpos = tloc + 1) > tlen)
                {
                    return false;
                }

                pos = loc + 1;
            }
        }
        else if (loc == string::npos && tloc == string::npos)
        {
            string subtopic = topicName->substr(pos);
            string subtopict = _topicName->substr(tpos);
            if (subtopict == wildcard || subtopict == wildcards)
            {
                return true;
            }
            else if (subtopic == subtopict)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (loc == string::npos && tloc != string::npos)
        {
            string subtopic = topicName->substr(pos);
            string subtopict = _topicName->substr(tpos, tloc - tpos);
            if (subtopic != subtopict)
            {
                return false;
            }

            tpos = tloc + 1;

            return _topicName->substr(tpos) == wildcard;
        }
        else if (loc != string::npos && tloc == string::npos)
        {
            return _topicName->substr(tpos) == wildcard;
        }
    }
}

void Topic::print(void)
{
    WRITELOG("TopicName=%s  ID=%d  Type=%d\n", _topicName->c_str(), _topicId, _type);
}

/*=====================================
 Class Topics
 ======================================*/
Topics::Topics()
{
    _first = nullptr;
    _nextTopicId = 0;
    _cnt = 0;
}

Topics::~Topics()
{
    Topic* p = _first;
    while (p)
    {
        Topic* q = p->_next;
        delete p;
        p = q;
    }
}

Topic* Topics::getTopicByName(const MQTTSN_topicid* topicid)
{
    Topic* p = _first;
    char* ch = topicid->data.long_.name;

    string sname = string(ch, ch + topicid->data.long_.len);
    while (p)
    {
        if (p->_topicName->compare(sname) == 0)
        {
            return p;
        }
        p = p->_next;
    }
    return 0;
}

Topic* Topics::getTopicById(const MQTTSN_topicid* topicid)
{
    Topic* p = _first;

    while (p)
    {
        if (p->_type == topicid->type && p->_topicId == topicid->data.id)
        {
            return p;
        }
        p = p->_next;
    }
    return 0;
}

// For MQTTSN_TOPIC_TYPE_NORMAL */
Topic* Topics::add(const MQTTSN_topicid* topicid)
{
    if (topicid->type != MQTTSN_TOPIC_TYPE_NORMAL)
    {
        return 0;
    }

    Topic* topic = getTopicByName(topicid);

    if (topic)
    {
        return topic;
    }
    string name(topicid->data.long_.name, topicid->data.long_.len);
    return add(name.c_str(), 0);
}

Topic* Topics::add(const char* topicName, uint16_t id)
{
    MQTTSN_topicid topicId;

    if (_cnt >= MAX_TOPIC_PAR_CLIENT)
    {
        return 0;
    }

    topicId.data.long_.name = (char*) const_cast<char*>(topicName);
    topicId.data.long_.len = strlen(topicName);

    Topic* topic = getTopicByName(&topicId);

    if (topic)
    {
        return topic;
    }

    topic = new Topic();

    if (topic == nullptr)
    {
        return nullptr;
    }

    string* name = new string(topicName);
    topic->_topicName = name;

    if (id == 0)
    {
        topic->_type = MQTTSN_TOPIC_TYPE_NORMAL;
        topic->_topicId = getNextTopicId();
    }
    else
    {
        topic->_type = MQTTSN_TOPIC_TYPE_PREDEFINED;
        topic->_topicId = id;
    }

    _cnt++;

    if (_first == nullptr)
    {
        _first = topic;
    }
    else
    {
        Topic* tp = _first;
        while (tp)
        {
            if (tp->_next == nullptr)
            {
                tp->_next = topic;
                break;
            }
            else
            {
                tp = tp->_next;
            }
        }
    }
    return topic;
}

uint16_t Topics::getNextTopicId()
{
    return ++_nextTopicId == 0xffff ? _nextTopicId += 2 : _nextTopicId;
}

Topic* Topics::match(const MQTTSN_topicid* topicid)
{
    if (topicid->type != MQTTSN_TOPIC_TYPE_NORMAL)
    {
        return 0;
    }
    string topicName(topicid->data.long_.name, topicid->data.long_.len);

    Topic* topic = _first;
    while (topic)
    {
        if (topic->isMatch(&topicName))
        {
            return topic;
        }
        topic = topic->_next;
    }
    return 0;
}

void Topics::eraseNormal(void)
{
    Topic* topic = _first;
    Topic* next = nullptr;
    Topic* prev = nullptr;

    while (topic)
    {
        if (topic->_type == MQTTSN_TOPIC_TYPE_NORMAL)
        {
            next = topic->_next;
            if (_first == topic)
            {
                _first = next;
            }
            if (prev)
            {
                prev->_next = next;
            }
            delete topic;
            _cnt--;
            topic = next;
        }
        else
        {
            prev = topic;
            topic = topic->_next;
        }
    }
}

Topic* Topics::getFirstTopic(void)
{
    return _first;
}

Topic* Topics::getNextTopic(Topic* topic)
{
    return topic->_next;
}

void Topics::print(void)
{
    Topic* topic = _first;
    if (topic == nullptr)
    {
        WRITELOG("No Topic.\n");
    }
    else
    {
        while (topic)
        {
            topic->print();
            topic = topic->_next;
        }
    }
}

uint8_t Topics::getCount(void)
{
    return _cnt;
}

/*=====================================
 Class TopicIdMap
 =====================================*/
TopicIdMapElement::TopicIdMapElement(uint16_t msgId, uint16_t topicId, MQTTSN_topicid* topic)
{
    _msgId = msgId;
    _topicId = topicId;
    _type = topic->type;
    _wildcard = 0;
    _next = nullptr;
    _prev = nullptr;

    if (_type == MQTTSN_TOPIC_TYPE_NORMAL)
    {
        if (strchr(topic->data.long_.name, '#') != 0 || strchr(topic->data.long_.name, '+') != 0)
        {
            _wildcard = 1;
        }
    }
}

TopicIdMapElement::~TopicIdMapElement()
{

}

MQTTSN_topicTypes TopicIdMapElement::getTopicType(void)
{
    return _type;
}

uint16_t TopicIdMapElement::getTopicId(void)
{
    if (_wildcard > 0)
    {
        return 0;
    }
    else
    {
        return _topicId;
    }
}

TopicIdMap::TopicIdMap()
{
    _maxInflight = MAX_INFLIGHTMESSAGES;
    _msgIds = 0;
    _first = nullptr;
    _end = nullptr;
    _cnt = 0;
}

TopicIdMap::~TopicIdMap()
{
    TopicIdMapElement* p = _first;
    while (p)
    {
        TopicIdMapElement* q = p->_next;
        delete p;
        p = q;
    }
}

TopicIdMapElement* TopicIdMap::getElement(uint16_t msgId)
{
    TopicIdMapElement* p = _first;
    while (p)
    {
        if (p->_msgId == msgId)
        {
            return p;
        }
        p = p->_next;
    }
    return 0;
}

TopicIdMapElement* TopicIdMap::add(uint16_t msgId, uint16_t topicId, MQTTSN_topicid* topic)
{
    if (_cnt > _maxInflight * 2 || (topicId == 0 && topic->type != MQTTSN_TOPIC_TYPE_SHORT))
    {
        return 0;
    }
    if (getElement(msgId))
    {
        erase(msgId);
    }

    TopicIdMapElement* elm = new TopicIdMapElement(msgId, topicId, topic);
    if (elm == 0)
    {
        return 0;
    }
    if (_first == nullptr)
    {
        _first = elm;
        _end = elm;
    }
    else
    {
        elm->_prev = _end;
        _end->_next = elm;
        _end = elm;
    }
    _cnt++;
    return elm;
}

void TopicIdMap::erase(uint16_t msgId)
{
    TopicIdMapElement* p = _first;
    while (p)
    {
        if (p->_msgId == msgId)
        {
            if (p->_prev == nullptr)
            {
                _first = p->_next;
            }
            else
            {
                p->_prev->_next = p->_next;
            }

            if (p->_next == nullptr)
            {
                _end = p->_prev;
            }
            else
            {
                p->_next->_prev = p->_prev;
            }
            delete p;
            break;

        }
        p = p->_next;
    }
    _cnt--;
}

void TopicIdMap::clear(void)
{
    TopicIdMapElement* p = _first;
    while (p)
    {
        TopicIdMapElement* q = p->_next;
        delete p;
        p = q;
    }
    _first = nullptr;
    _end = nullptr;
    _cnt = 0;
}

