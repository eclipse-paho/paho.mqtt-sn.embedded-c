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

#include "MQTTSNGWPublishHandler.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWQoSm1Proxy.h"
#include <string.h>
using namespace std;
using namespace MQTTSNGW;

MQTTSNPublishHandler::MQTTSNPublishHandler(Gateway* gateway)
{
    _gateway = gateway;
}

MQTTSNPublishHandler::~MQTTSNPublishHandler()
{

}

MQTTGWPacket* MQTTSNPublishHandler::handlePublish(Client* client, MQTTSNPacket* packet)
{
    uint8_t dup;
    int qos;
    uint8_t retained;
    uint16_t msgId;
    uint16_t tid = 0;
    uint8_t* payload;
    MQTTSN_topic topic;
    int32_t payloadlen;
    Publish pub = MQTTPacket_Publish_Initializer;

    if (!_gateway->getAdapterManager()->getQoSm1Proxy()->isActive())
    {
        if (client->isQoSm1())
        {
            _gateway->getAdapterManager()->getQoSm1Proxy()->savePacket(client, packet);
            return nullptr;
        }
    }

    if (packet->getPUBLISH(&dup, &qos, &retained, &msgId, &topic, &payload, &payloadlen) == 0)
    {
        return nullptr;
    }
    pub.msgId = msgId;
    pub.header.bits.dup = dup;
    pub.header.bits.qos = (qos == 3 ? 0 : qos);
    pub.header.bits.retain = retained;

    Topic* topicEntry = nullptr;

    if (topic.type == MQTTSN_TOPIC_TYPE_SESSION || topic.type == MQTTSN_TOPIC_TYPE_PREDEFINED)
    {
        tid = topic.alt.alias;
        topicEntry = client->getTopics()->getTopicById(&topic);
        if (!topicEntry)
        {
            topicEntry = _gateway->getTopics()->getTopicById(&topic);
            if (topicEntry)
            {
                topicEntry = client->getTopics()->add(topicEntry->getTopicName()->c_str(), topicEntry->getTopicId());
            }
        }

        if (!topicEntry && qos == 3)
        {
            WRITELOG("%s Invalid TopicId.%s %s\n", ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
            return nullptr;
        }

        if ((qos == 0 || qos == 3) && msgId > 0)
        {
            WRITELOG("%s Invalid MsgId.%s %s\n", ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
            return nullptr;
        }

        if (!topicEntry && msgId && qos > 0 && qos < 3)
        {
            MQTTSNPacket* pubAck = new MQTTSNPacket();
            pubAck->setPUBACK(msgId, MQTT_SN_RC_TOPIC_ALIAS_INVALID);
            Event* ev1 = new Event();
            ev1->setClientSendEvent(client, pubAck);
            _gateway->getClientSendQue()->post(ev1);
            return nullptr;
        }
    }
    else
    {
        /* MQTTSN_TOPIC_TYPE_NAME or MQTTSN_TOPIC_TYPE_FILTER */
        topicEntry = client->getTopics()->getTopicByName(&topic);
        if (!topicEntry)
        {
            topicEntry = client->getTopics()->add(&topic);
        }
    }

    if (topicEntry)
    {
        pub.topic = (char*) topicEntry->getTopicName()->data();
        pub.topiclen = topicEntry->getTopicName()->length();
        topic.alt.string.data = pub.topic;
        topic.alt.string.len = pub.topiclen;
    }

    /* Save a msgId & a TopicId pair for PUBACK */
    if (msgId && qos > 0 && qos < 3)
    {
        client->setWaitedPubTopicId(msgId, tid, &topic);
    }

    pub.payload = (char*) payload;
    pub.payloadlen = payloadlen;

    MQTTGWPacket* publish = new MQTTGWPacket();
    publish->setPUBLISH(&pub);

    if (_gateway->getAdapterManager()->isAggregaterActive() && client->isAggregated())
    {
        return publish;
    }
    else
    {
        Event* ev1 = new Event();
        ev1->setBrokerSendEvent(client, publish);
        _gateway->getBrokerSendQue()->post(ev1);
        return nullptr;
    }
}

void MQTTSNPublishHandler::handlePuback(Client* client, MQTTSNPacket* packet)
{
    uint16_t msgId;
    uint8_t rc;

    if (client->isActive())
    {
        if (packet->getPUBACK(&msgId, &rc) == 0)
        {
            return;
        }

        if (rc == MQTT_SN_RC_SUCCESS)
        {
            if (!_gateway->getAdapterManager()->getAggregater()->isActive())
            {
                MQTTGWPacket* pubAck = new MQTTGWPacket();
                pubAck->setAck(PUBACK, msgId);
                Event* ev1 = new Event();
                ev1->setBrokerSendEvent(client, pubAck);
                _gateway->getBrokerSendQue()->post(ev1);
            }
        }
        else if (rc == MQTT_SN_RC_TOPIC_ALIAS_INVALID)
        {
            WRITELOG("  PUBACK   %d : Invalid Topic Alias\n", msgId);
        }
    }
}

void MQTTSNPublishHandler::handleAck(Client* client, MQTTSNPacket* packet, uint8_t packetType)
{
    uint16_t msgId;
    uint8_t rc;

    if (client->isActive())
    {
        if (packet->getACK(&msgId, &rc) == 0)
        {
            return;
        }
        MQTTGWPacket* ackPacket = new MQTTGWPacket();
        ackPacket->setAck(packetType, msgId);
        Event* ev1 = new Event();
        ev1->setBrokerSendEvent(client, ackPacket);
        _gateway->getBrokerSendQue()->post(ev1);
    }
}

void MQTTSNPublishHandler::handleRegister(Client* client, MQTTSNPacket* packet)
{
    uint8_t alias_present;
    uint16_t id;
    uint16_t msgId;
    MQTTSN_string topicName = {0, 0, nullptr};
    MQTTSN_topic topicid;

    if (client->isActive() || client->isAwake())
    {
        if (packet->getREGISTER(&alias_present, &id, &msgId, &topicName) == 0)
        {
            return;
        }

        topicid.type = MQTTSN_TOPIC_TYPE_NAME;
        topicid.alt.string.len = topicName.len;
        topicid.alt.string.data = topicName.data;

        id = client->getTopics()->add(&topicid)->getTopicId();

        MQTTSNPacket* regAck = new MQTTSNPacket();
        regAck->setREGACK(MQTTSN_TOPIC_TYPE_NAME, 1, id, msgId, MQTT_SN_RC_SUCCESS);
        Event* ev = new Event();
        ev->setClientSendEvent(client, regAck);
        _gateway->getClientSendQue()->post(ev);
    }
}

void MQTTSNPublishHandler::handleRegAck(Client* client, MQTTSNPacket* packet)
{
    uint8_t type;
    uint8_t alias_present;
    uint16_t id;
    uint16_t msgId;
    uint8_t rc;

    if (client->isActive() || client->isAwake())
    {
        if (packet->getREGACK(&type, &alias_present, &id, &msgId, &rc) == 0)
        {
            return;
        }

        /* get PUBLISH message */
        MQTTSNPacket* regAck = client->getWaitREGACKPacketList()->getPacket(msgId);

        if (regAck != nullptr)
        {
            client->getWaitREGACKPacketList()->erase(msgId);
            Event* ev = new Event();
            ev->setClientSendEvent(client, regAck);
            _gateway->getClientSendQue()->post(ev);
        }

        if (client->isHoldPingReqest() && client->getWaitREGACKPacketList()->getCount() == 0)
        {
            client->resetPingRequest();
            MQTTGWPacket* pingreq = new MQTTGWPacket();
            pingreq->setHeader(PINGREQ);
            Event* evt = new Event();
            evt->setBrokerSendEvent(client, pingreq);
            _gateway->getBrokerSendQue()->post(evt);
        }
    }
}

void MQTTSNPublishHandler::handleAggregatePublish(Client* client, MQTTSNPacket* packet)
{
    int msgId = 0;
    MQTTGWPacket* publish = handlePublish(client, packet);
    if (publish != nullptr)
    {
        if (publish->getMsgId() > 0)
        {
            if (packet->isDuplicate())
            {
                msgId = _gateway->getAdapterManager()->getAggregater()->getMsgId(client, packet->getMsgId());
            }
            else
            {
                msgId = _gateway->getAdapterManager()->getAggregater()->addMessageIdTable(client, packet->getMsgId());
            }
            publish->setMsgId(msgId);
        }
        Event* ev1 = new Event();
        ev1->setBrokerSendEvent(client, publish);
        _gateway->getBrokerSendQue()->post(ev1);
    }
}

void MQTTSNPublishHandler::handleAggregateAck(Client* client, MQTTSNPacket* packet, int type)
{
    if (type == MQTTSN_PUBREC)
    {
        uint16_t msgId;
        uint8_t rc;

        if (packet->getACK(&msgId, &rc) == 0)
        {
            return;
        }
        MQTTSNPacket* ackPacket = new MQTTSNPacket();
        ackPacket->setPUBREL(msgId, MQTT_SN_RC_SUCCESS);
        Event* ev = new Event();
        ev->setClientSendEvent(client, ackPacket);
        _gateway->getClientSendQue()->post(ev);
    }
}
