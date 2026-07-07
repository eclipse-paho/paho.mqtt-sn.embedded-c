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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Tieto Poland Sp. z o.o. - Gateway improvements
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
    uint16_t tid;
    uint8_t* payload;
    MQTTSN_topicid topicid;
    int payloadlen;
    Publish pub = MQTTPacket_Publish_Initializer;

    char shortTopic[2];

    if (!_gateway->getAdapterManager()->getQoSm1Proxy()->isActive())
    {
        if (client->isQoSm1())
        {
            _gateway->getAdapterManager()->getQoSm1Proxy()->savePacket(client, packet);

            return nullptr;
        }
    }

    if (packet->getPUBLISH(&dup, &qos, &retained, &msgId, &topicid, &payload, &payloadlen) == 0)
    {
        return nullptr;
    }
    pub.msgId = msgId;
    pub.header.bits.dup = dup;
    pub.header.bits.qos = (qos == 3 ? 0 : qos);
    pub.header.bits.retain = retained;
    tid = topicid.data.id;

    Topic* topic = nullptr;

    if (topicid.type == MQTTSN_TOPIC_TYPE_SHORT)
    {
        shortTopic[0] = topicid.data.short_name[0];
        shortTopic[1] = topicid.data.short_name[1];
        pub.topic = shortTopic;
        pub.topiclen = 2;
    }
    else
    {
        topic = client->getTopics()->getTopicById(&topicid);
        if (!topic)
        {
            topic = _gateway->getTopics()->getTopicById(&topicid);
            if (topic)
            {
                topic = client->getTopics()->add(topic->getTopicName()->c_str(), topic->getTopicId());
            }
        }

        if (!topic && qos == 3)
        {
            WRITELOG("%s Invalid TopicId.%s %s\n", ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
            return nullptr;
        }

        if ((qos == 0 || qos == 3) && msgId > 0)
        {
            WRITELOG("%s Invalid MsgId.%s %s\n", ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
            return nullptr;
        }

        if (!topic && msgId && qos > 0 && qos < 3)
        {
            /* Reply PubAck with INVALID_TOPIC_ID to the client */
            MQTTSNPacket* pubAck = new MQTTSNPacket();
            pubAck->setPUBACK(topicid.data.id, msgId, MQTTSN_RC_REJECTED_INVALID_TOPIC_ID);
            Event* ev1 = new Event();
            ev1->setClientSendEvent(client, pubAck);
            _gateway->getClientSendQue()->post(ev1);
            return nullptr;
        }
        if (topic)
        {
            pub.topic = (char*) topic->getTopicName()->data();
            pub.topiclen = topic->getTopicName()->length();
            topicid.data.long_.name = pub.topic;
            topicid.data.long_.len = pub.topiclen;
        }
    }
    /* Save a msgId & a TopicId pare for PUBACK */
    if (msgId && qos > 0 && qos < 3)
    {
        client->setWaitedPubTopicId(msgId, tid, &topicid);
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
    uint16_t topicId;
    uint16_t msgId;
    uint8_t rc;

    if (client->isActive())
    {
        if (packet->getPUBACK(&topicId, &msgId, &rc) == 0)
        {
            return;
        }

        if (rc == MQTTSN_RC_ACCEPTED)
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
        else if (rc == MQTTSN_RC_REJECTED_INVALID_TOPIC_ID)
        {
            WRITELOG("  PUBACK   %d : Invalid Topic ID\n", msgId);
        }
    }
}

void MQTTSNPublishHandler::handleAck(Client* client, MQTTSNPacket* packet, uint8_t packetType)
{
    uint16_t msgId;

    if (client->isActive())
    {
        if (packet->getACK(&msgId) == 0)
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
    uint16_t id;
    uint16_t msgId;
    MQTTSNString topicName = MQTTSNString_initializer;
    ;
    MQTTSN_topicid topicid;

    if (client->isActive() || client->isAwake())
    {
        if (packet->getREGISTER(&id, &msgId, &topicName) == 0)
        {
            return;
        }

        topicid.type = MQTTSN_TOPIC_TYPE_NORMAL;
        topicid.data.long_.len = topicName.lenstring.len;
        topicid.data.long_.name = topicName.lenstring.data;

        id = client->getTopics()->add(&topicid)->getTopicId();

        MQTTSNPacket* regAck = new MQTTSNPacket();
        regAck->setREGACK(id, msgId, MQTTSN_RC_ACCEPTED);
        Event* ev = new Event();
        ev->setClientSendEvent(client, regAck);
        _gateway->getClientSendQue()->post(ev);
    }
}

void MQTTSNPublishHandler::handleRegAck(Client* client, MQTTSNPacket* packet)
{
    uint16_t id;
    uint16_t msgId;
    uint8_t rc;
    if (client->isActive() || client->isAwake())
    {
        if (packet->getREGACK(&id, &msgId, &rc) == 0)
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
            /* send PINGREQ to the broker */
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

        if (packet->getACK(&msgId) == 0)
        {
            return;
        }
        MQTTSNPacket* ackPacket = new MQTTSNPacket();
        ackPacket->setPUBREL(msgId);
        Event* ev = new Event();
        ev->setClientSendEvent(client, ackPacket);
        _gateway->getClientSendQue()->post(ev);
    }
}
