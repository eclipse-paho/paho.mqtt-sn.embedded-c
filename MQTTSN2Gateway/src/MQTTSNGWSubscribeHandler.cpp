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

#include "MQTTSNGWSubscribeHandler.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"

using namespace std;
using namespace MQTTSNGW;

MQTTSNSubscribeHandler::MQTTSNSubscribeHandler(Gateway* gateway)
{
    _gateway = gateway;
}

MQTTSNSubscribeHandler::~MQTTSNSubscribeHandler()
{

}

MQTTGWPacket* MQTTSNSubscribeHandler::handleSubscribe(Client* client, MQTTSNPacket* packet)
{
    int qos;
    uint8_t retain_handling;
    uint8_t rap;
    uint8_t no_local;
    uint16_t msgId;
    MQTTSN_topic topicFilter;
    Topic* topic = nullptr;
    uint16_t topicId = 0;
    MQTTGWPacket* subscribe;
    Event* ev1;
    Event* evsuback;

    if (packet->getSUBSCRIBE(&qos, &retain_handling, &rap, &no_local, &msgId, &topicFilter) == 0)
    {
        return nullptr;
    }

    if (msgId == 0)
    {
        return nullptr;
    }

    if (topicFilter.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
    {
        topic = client->getTopics()->getTopicById(&topicFilter);

        if (!topic)
        {
            topic = _gateway->getTopics()->getTopicById(&topicFilter);
            if (topic)
            {
                topic = client->getTopics()->add(topic->getTopicName()->c_str(), topic->getTopicId());
                if (topic == nullptr)
                {
                    WRITELOG("%s Client(%s) can't add the Topic.%s\n",
                    ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
                    goto RespExit;
                }
            }
            else
            {
                goto RespExit;
            }
        }
        topicId = topic->getTopicId();
        subscribe = new MQTTGWPacket();
        subscribe->setSUBSCRIBE((char*) topic->getTopicName()->c_str(), (uint8_t) qos, (uint16_t) msgId);
    }
    else
    {
        /* MQTTSN_TOPIC_TYPE_NAME or MQTTSN_TOPIC_TYPE_FILTER */
        topic = client->getTopics()->getTopicByName(&topicFilter);
        if (topic == nullptr)
        {
            topic = client->getTopics()->add(&topicFilter);
            if (topic == nullptr)
            {
                WRITELOG("%s Client(%s) can't add the Topic.%s\n",
                ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
                goto RespExit;
            }
        }
        topicId = topic->getTopicId();
        subscribe = new MQTTGWPacket();
        subscribe->setSUBSCRIBE((char*) topic->getTopicName()->c_str(), (uint8_t) qos, (uint16_t) msgId);
    }

    client->setWaitedSubTopicId(msgId, topicId, &topicFilter);

    if (!client->isAggregated())
    {
        ev1 = new Event();
        ev1->setBrokerSendEvent(client, subscribe);
        _gateway->getBrokerSendQue()->post(ev1);
        return nullptr;
    }
    else
    {
        return subscribe;
    }

    RespExit:
    MQTTSNPacket* sSuback = new MQTTSNPacket();
    sSuback->setSUBACK(topicFilter.type, 1, topicFilter.alt.alias, msgId, MQTT_SN_RC_TOPIC_ALIAS_INVALID);
    evsuback = new Event();
    evsuback->setClientSendEvent(client, sSuback);
    _gateway->getClientSendQue()->post(evsuback);
    return nullptr;
}

MQTTGWPacket* MQTTSNSubscribeHandler::handleUnsubscribe(Client* client, MQTTSNPacket* packet)
{
    uint16_t msgId;
    MQTTSN_topic topicFilter;
    MQTTGWPacket* unsubscribe = nullptr;

    if (packet->getUNSUBSCRIBE(&msgId, &topicFilter) == 0)
    {
        return nullptr;
    }

    if (msgId == 0)
    {
        return nullptr;
    }

    Topic* topic = nullptr;

    if (topicFilter.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
    {
        topic = client->getTopics()->getTopicById(&topicFilter);
    }
    else
    {
        topic = client->getTopics()->getTopicByName(&topicFilter);
    }

    if (topic == nullptr)
    {
        MQTTSNPacket* sUnsuback = new MQTTSNPacket();
        sUnsuback->setUNSUBACK(msgId, MQTT_SN_RC_SUCCESS);
        Event* evsuback = new Event();
        evsuback->setClientSendEvent(client, sUnsuback);
        _gateway->getClientSendQue()->post(evsuback);
        return nullptr;
    }
    else
    {
        unsubscribe = new MQTTGWPacket();
        unsubscribe->setUNSUBSCRIBE(topic->getTopicName()->c_str(), msgId);
    }

    if (!client->isAggregated())
    {
        Event* ev1 = new Event();
        ev1->setBrokerSendEvent(client, unsubscribe);
        _gateway->getBrokerSendQue()->post(ev1);
        return nullptr;
    }
    else
    {
        return unsubscribe;
    }
}

void MQTTSNSubscribeHandler::handleAggregateSubscribe(Client* client, MQTTSNPacket* packet)
{
    MQTTGWPacket* subscribe = handleSubscribe(client, packet);

    if (subscribe != nullptr)
    {
        int msgId = 0;
        if (packet->isDuplicate())
        {
            msgId = _gateway->getAdapterManager()->getAggregater()->getMsgId(client, packet->getMsgId());
        }
        else
        {
            msgId = _gateway->getAdapterManager()->getAggregater()->addMessageIdTable(client, packet->getMsgId());
        }

        if (msgId == 0)
        {
            WRITELOG("%s MQTTSNSubscribeHandler can't create MessageIdTableElement  %s%s\n",
            ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
            return;
        }

        UTF8String str = subscribe->getTopic();
        string* topicName = new string(str.data, str.len);
        Topic topic = Topic(topicName, MQTTSN_TOPIC_TYPE_NAME);

        _gateway->getAdapterManager()->getAggregater()->addAggregateTopic(&topic, client);

        subscribe->setMsgId(msgId);
        Event* ev = new Event();
        ev->setBrokerSendEvent(client, subscribe);
        _gateway->getBrokerSendQue()->post(ev);
    }
}

void MQTTSNSubscribeHandler::handleAggregateUnsubscribe(Client* client, MQTTSNPacket* packet)
{
    MQTTGWPacket* unsubscribe = handleUnsubscribe(client, packet);
    if (unsubscribe != nullptr)
    {
        int msgId = 0;
        if (packet->isDuplicate())
        {
            msgId = _gateway->getAdapterManager()->getAggregater()->getMsgId(client, packet->getMsgId());
        }
        else
        {
            msgId = _gateway->getAdapterManager()->getAggregater()->addMessageIdTable(client, packet->getMsgId());
        }

        if (msgId == 0)
        {
            WRITELOG("%s MQTTSNUnsubscribeHandler can't create MessageIdTableElement  %s%s\n",
            ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
            return;
        }

        UTF8String str = unsubscribe->getTopic();
        string* topicName = new string(str.data, str.len);
        Topic topic = Topic(topicName, MQTTSN_TOPIC_TYPE_NAME);
        _gateway->getAdapterManager()->getAggregater()->removeAggregateTopic(&topic, client);

        unsubscribe->setMsgId(msgId);
        Event* ev = new Event();
        ev->setBrokerSendEvent(client, unsubscribe);
        _gateway->getBrokerSendQue()->post(ev);
    }
}
