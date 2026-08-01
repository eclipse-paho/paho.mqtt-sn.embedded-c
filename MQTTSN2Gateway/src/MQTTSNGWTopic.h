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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Tieto Poland Sp. z o.o. - Gateway improvements
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWTOPIC_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWTOPIC_H_

#include "MQTTSNGWPacket.h"
#include "MQTTSNPacket.h"

namespace MQTTSNGW
{

/*=====================================
 Class Topic
 ======================================*/
class Topic
{
    friend class Topics;
    friend class AggregateTopicTable;
public:
    Topic();
    Topic(string* topic, MQTTSN_topicTypes type);
    ~Topic();
    string* getTopicName(void);
    uint16_t getTopicId(void);
    MQTTSN_topicTypes getType(void);
    bool isMatch(string* topicName);
    Topic* duplicate(void);
    void print(void);

private:
    MQTTSN_topicTypes _type;
    uint16_t _topicId;
    string* _topicName;
    Topic* _next;
};

/*=====================================
 Class Topics
 ======================================*/
class Topics
{
public:
    Topics();
    ~Topics();
    Topic* add(const MQTTSN_topic* topicid);
    Topic* add(const char* topicName, uint16_t id = 0);
    Topic* getTopicByName(const MQTTSN_topic* topic);
    Topic* getTopicById(const MQTTSN_topic* topicid);
    Topic* getFirstTopic(void);
    Topic* getNextTopic(Topic* topic);
    Topic* match(const MQTTSN_topic* topicid);
    void eraseNormal(void);
    uint16_t getNextTopicId();
    void print(void);
    uint8_t getCount(void);
private:
    uint16_t _nextTopicId;
    Topic* _first;
    uint8_t _cnt;
};

/*=====================================
 Class TopicIdMapElement
 =====================================*/
class TopicIdMapElement
{
    friend class TopicIdMap;
public:
    TopicIdMapElement(uint16_t msgId, uint16_t topicId, MQTTSN_topic* topic);
    ~TopicIdMapElement();
    MQTTSN_topicTypes getTopicType(void);
    uint16_t getTopicId(void);

private:
    uint16_t _msgId;
    uint16_t _topicId;
    uint8_t _wildcard;
    MQTTSN_topicTypes _type;
    TopicIdMapElement* _next;
    TopicIdMapElement* _prev;
};

class TopicIdMap
{
public:
    TopicIdMap();
    ~TopicIdMap();
    TopicIdMapElement* getElement(uint16_t msgId);
    TopicIdMapElement* add(uint16_t msgId, uint16_t topicId, MQTTSN_topic* topic);
    void erase(uint16_t msgId);
    void clear(void);
private:
    uint16_t* _msgIds;
    TopicIdMapElement* _first;
    TopicIdMapElement* _end;
    int _cnt;
    int _maxInflight;
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWTOPIC_H_ */
