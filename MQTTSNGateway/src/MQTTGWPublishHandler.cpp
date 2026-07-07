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

#include "MQTTGWPublishHandler.h"
#include "MQTTSNGateway.h"
#include "MQTTSNPacket.h"
#include <string>

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime(void);

MQTTGWPublishHandler::MQTTGWPublishHandler(Gateway* gateway)
{
    _gateway = gateway;
}

MQTTGWPublishHandler::~MQTTGWPublishHandler()
{

}

void MQTTGWPublishHandler::handlePublish(Client* client, MQTTGWPacket* packet)
{
    if (!client->isActive() && !client->isSleep() && !client->isAwake())
    {
        WRITELOG("%s     The client is neither active nor sleep %s%s\n",
        ERRMSG_HEADER, client->getStatus(), ERRMSG_FOOTER);
        return;
    }

    /* client is sleeping. save PUBLISH */
    if (client->isSleep())
    {
        Publish pub;
        packet->getPUBLISH(&pub);

        WRITELOG(FORMAT_Y_G_G, currentDateTime(), packet->getName(),
        RIGHTARROW, client->getClientId(), "is sleeping. a message was saved.");

        if (pub.header.bits.qos == 1)
        {
            replyACK(client, &pub, PUBACK);
        }
        else if (pub.header.bits.qos == 2)
        {
            replyACK(client, &pub, PUBREC);
        }

        MQTTGWPacket* msg = new MQTTGWPacket();
        *msg = *packet;
        if (msg->getType() == 0)
        {
            WRITELOG("%s MQTTGWPublishHandler::handlePublish can't allocate memories for Packet.%s\n",
            ERRMSG_HEADER, ERRMSG_FOOTER);
            delete msg;
            return;
        }
        client->setClientSleepPacket(msg);
        return;
    }

    Publish pub;
    packet->getPUBLISH(&pub);

    MQTTSNPacket* snPacket = new MQTTSNPacket();

    /* create MQTTSN_topicid */
    MQTTSN_topicid topicId;
    uint16_t id = 0;

    if (pub.topiclen <= 2)
    {
        topicId.type = MQTTSN_TOPIC_TYPE_SHORT;
        *(topicId.data.short_name) = *pub.topic;
        *(topicId.data.short_name + 1) = *(pub.topic + 1);
    }
    else
    {
        topicId.data.long_.len = pub.topiclen;
        topicId.data.long_.name = pub.topic;
        Topic* tp = client->getTopics()->getTopicByName(&topicId);

        if (tp)
        {
            topicId.type = tp->getType();
            topicId.data.long_.len = pub.topiclen;
            topicId.data.long_.name = pub.topic;
            topicId.data.id = tp->getTopicId();
        }
        else
        {
            /* This message might be subscribed with wild card or not cleanSession*/
            topicId.type = MQTTSN_TOPIC_TYPE_NORMAL;
            Topic* topic = client->getTopics()->match(&topicId);

            if (topic == nullptr && client->isCleanSession())
            {
                WRITELOG("%sMQTTGWPublishHandler Invalid Topic. PUBLISH message is discarded.%s\n",
                ERRMSG_HEADER, ERRMSG_FOOTER);
                if (pub.header.bits.qos == 1)
                {
                    replyACK(client, &pub, PUBACK);
                }
                else if (pub.header.bits.qos == 2)
                {
                    replyACK(client, &pub, PUBREC);
                }

                delete snPacket;
                return;
            }

            if (topic == nullptr)
            {
                topicId.type = MQTTSN_TOPIC_TYPE_NORMAL;
                topicId.data.long_.len = pub.topiclen;
                topicId.data.long_.name = pub.topic;
                topicId.data.id = 0;
            }

            /* add the Topic and get a TopicId */
            topic = client->getTopics()->add(&topicId);
            if (topic == nullptr)
            {
                WRITELOG(
                        "%sMQTTGWPublishHandler Can't Add a Topic. MAX_TOPIC_PAR_CLIENT is exceeded. PUBLISH message is discarded.%s\n",
                        ERRMSG_HEADER, ERRMSG_FOOTER);
                delete snPacket;
                return;
            }
            id = topic->getTopicId();
            if (id > 0)
            {
                /* create REGISTER */
                MQTTSNPacket* regPacket = new MQTTSNPacket();

                MQTTSNString topicName = MQTTSNString_initializer;
                topicName.lenstring.len = topicId.data.long_.len;
                topicName.lenstring.data = topicId.data.long_.name;

                uint16_t regackMsgId = client->getNextSnMsgId();
                regPacket->setREGISTER(id, regackMsgId, &topicName);

                /* send REGISTER */
                Event* evrg = new Event();
                evrg->setClientSendEvent(client, regPacket);
                _gateway->getClientSendQue()->post(evrg);

                /* send PUBLISH */
                topicId.data.id = id;
                snPacket->setPUBLISH((uint8_t) pub.header.bits.dup, (int) pub.header.bits.qos, (uint8_t) pub.header.bits.retain,
                        (uint16_t) pub.msgId, topicId, (uint8_t*) pub.payload, pub.payloadlen);
                client->getWaitREGACKPacketList()->setPacket(snPacket, regackMsgId);
                return;
            }
            else
            {
                WRITELOG("%sMQTTGWPublishHandler Can't create a Topic. PUBLISH message is discarded.%s\n",
                ERRMSG_HEADER, ERRMSG_FOOTER);
                delete snPacket;
                return;
            }
        }
    }

    snPacket->setPUBLISH((uint8_t) pub.header.bits.dup, (int) pub.header.bits.qos, (uint8_t) pub.header.bits.retain,
            (uint16_t) pub.msgId, topicId, (uint8_t*) pub.payload, pub.payloadlen);
    Event* ev1 = new Event();
    ev1->setClientSendEvent(client, snPacket);
    _gateway->getClientSendQue()->post(ev1);

}

void MQTTGWPublishHandler::replyACK(Client* client, Publish* pub, int type)
{
    MQTTGWPacket* pubAck = new MQTTGWPacket();
    pubAck->setAck(type, (uint16_t) pub->msgId);
    Event* ev1 = new Event();
    ev1->setBrokerSendEvent(client, pubAck);
    _gateway->getBrokerSendQue()->post(ev1);
}

void MQTTGWPublishHandler::handlePuback(Client* client, MQTTGWPacket* packet)
{
    Ack ack;
    packet->getAck(&ack);
    TopicIdMapElement* topicId = client->getWaitedPubTopicId((uint16_t) ack.msgId);
    if (topicId)
    {
        MQTTSNPacket* mqttsnPacket = new MQTTSNPacket();
        mqttsnPacket->setPUBACK(topicId->getTopicId(), (uint16_t) ack.msgId, 0);

        client->eraseWaitedPubTopicId((uint16_t) ack.msgId);
        Event* ev1 = new Event();
        ev1->setClientSendEvent(client, mqttsnPacket);
        _gateway->getClientSendQue()->post(ev1);
        return;
    }
    WRITELOG(" PUBACK from the Broker is invalid. PacketID : %04X  ClientID : %s \n", (uint16_t) ack.msgId,
            client->getClientId());
}

void MQTTGWPublishHandler::handleAck(Client* client, MQTTGWPacket* packet, int type)
{
    Ack ack;
    packet->getAck(&ack);

    if (client->isActive() || client->isAwake())
    {
        MQTTSNPacket* mqttsnPacket = new MQTTSNPacket();
        if (type == PUBREC)
        {
            mqttsnPacket->setPUBREC((uint16_t) ack.msgId);
        }
        else if (type == PUBREL)
        {
            mqttsnPacket->setPUBREL((uint16_t) ack.msgId);
        }
        else if (type == PUBCOMP)
        {
            mqttsnPacket->setPUBCOMP((uint16_t) ack.msgId);
        }

        Event* ev1 = new Event();
        ev1->setClientSendEvent(client, mqttsnPacket);
        _gateway->getClientSendQue()->post(ev1);
    }
    else if (client->isSleep())
    {
        if (type == PUBREL)
        {
            MQTTGWPacket* pubComp = new MQTTGWPacket();
            pubComp->setAck(PUBCOMP, (uint16_t) ack.msgId);
            Event* ev1 = new Event();
            ev1->setBrokerSendEvent(client, pubComp);
            _gateway->getBrokerSendQue()->post(ev1);
        }
    }
}

void MQTTGWPublishHandler::handleAggregatePuback(Client* client, MQTTGWPacket* packet)
{
    uint16_t msgId = packet->getMsgId();
    uint16_t clientMsgId = 0;
    Client* newClient = _gateway->getAdapterManager()->convertClient(msgId, &clientMsgId);
    if (newClient != nullptr)
    {
        packet->setMsgId((int) clientMsgId);
        handlePuback(newClient, packet);
    }
}

void MQTTGWPublishHandler::handleAggregateAck(Client* client, MQTTGWPacket* packet, int type)
{
    uint16_t msgId = packet->getMsgId();
    uint16_t clientMsgId = 0;
    Client* newClient = _gateway->getAdapterManager()->convertClient(msgId, &clientMsgId);
    if (newClient != nullptr)
    {
        packet->setMsgId((int) clientMsgId);
        handleAck(newClient, packet, type);
    }
}

void MQTTGWPublishHandler::handleAggregatePubrel(Client* client, MQTTGWPacket* packet)
{
    Publish pub;
    packet->getPUBLISH(&pub);
    replyACK(client, &pub, PUBCOMP);
}

void MQTTGWPublishHandler::handleAggregatePublish(Client* client, MQTTGWPacket* packet)
{
    Publish pub;
    packet->getPUBLISH(&pub);

    string* topicName = new string(pub.topic, pub.topiclen); // topic deletes topicName when the topic is deleted
    Topic topic = Topic(topicName, MQTTSN_TOPIC_TYPE_NORMAL);

    // ToDo: need to refactor
    ClientTopicElement* elm = _gateway->getAdapterManager()->getAggregater()->getClientElement(&topic);

    while (elm != nullptr)
    {
        Client* devClient = elm->getClient();
        MQTTGWPacket* msg = new MQTTGWPacket();
        *msg = *packet;

        if (msg->getType() == 0)
        {
            WRITELOG("%s MQTTGWPublishHandler::handleAggregatePublish can't allocate memories for Packet.%s\n",
            ERRMSG_HEADER, ERRMSG_FOOTER);
            delete msg;
            break;
        }

        Event* ev = new Event();
        ev->setBrokerRecvEvent(devClient, msg);
        _gateway->getPacketEventQue()->post(ev);

        elm = elm->getNextClientElement();
    }
}

