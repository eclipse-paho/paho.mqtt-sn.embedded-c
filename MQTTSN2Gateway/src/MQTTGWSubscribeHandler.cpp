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
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#include "MQTTGWSubscribeHandler.h"
#include "MQTTGWPacket.h"

using namespace std;
using namespace MQTTSNGW;

MQTTGWSubscribeHandler::MQTTGWSubscribeHandler(Gateway* gateway)
{
    _gateway = gateway;
}

MQTTGWSubscribeHandler::~MQTTGWSubscribeHandler()
{

}

void MQTTGWSubscribeHandler::handleSuback(Client* client, MQTTGWPacket* packet)
{
    uint16_t msgId;
    uint8_t rc;
    uint8_t returnCode;

    packet->getSUBACK(&msgId, &rc);
    TopicIdMapElement* topicId = client->getWaitedSubTopicId(msgId);

    if (topicId)
    {
        MQTTSNPacket* snPacket = new MQTTSNPacket();

        if (rc == 0x80)
        {
            returnCode = MQTT_SN_RC_TOPIC_ALIAS_INVALID;
        }
        else
        {
            returnCode = rc; /* granted QoS 0/1/2 maps directly to MQTT_SN_RC_GRANTED_QOS_0/1/2 */
        }
        snPacket->setSUBACK(topicId->getTopicType(), 1, topicId->getTopicId(), msgId, returnCode);
        Event* evt = new Event();
        evt->setClientSendEvent(client, snPacket);
        _gateway->getClientSendQue()->post(evt);
        client->eraseWaitedSubTopicId(msgId);
    }
}

void MQTTGWSubscribeHandler::handleUnsuback(Client* client, MQTTGWPacket* packet)
{
    Ack ack;
    packet->getAck(&ack);
    MQTTSNPacket* snPacket = new MQTTSNPacket();
    snPacket->setUNSUBACK(ack.msgId, MQTT_SN_RC_SUCCESS);
    Event* evt = new Event();
    evt->setClientSendEvent(client, snPacket);
    _gateway->getClientSendQue()->post(evt);
}

void MQTTGWSubscribeHandler::handleAggregateSuback(Client* client, MQTTGWPacket* packet)
{
    uint16_t msgId = packet->getMsgId();
    uint16_t clientMsgId = 0;
    Client* newClient = _gateway->getAdapterManager()->getAggregater()->convertClient(msgId, &clientMsgId);
    if (newClient != nullptr)
    {
        packet->setMsgId((int) clientMsgId);
        handleSuback(newClient, packet);
    }
}

void MQTTGWSubscribeHandler::handleAggregateUnsuback(Client* client, MQTTGWPacket* packet)
{
    uint16_t msgId = packet->getMsgId();
    uint16_t clientMsgId = 0;
    Client* newClient = _gateway->getAdapterManager()->getAggregater()->convertClient(msgId, &clientMsgId);
    if (newClient != nullptr)
    {
        packet->setMsgId((int) clientMsgId);
        handleUnsuback(newClient, packet);
    }
}

