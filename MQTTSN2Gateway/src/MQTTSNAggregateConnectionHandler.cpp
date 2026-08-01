/**************************************************************************************
 * Copyright (c) 2018, 2026 Tomoaki Yamaguchi, Ian Craggs
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

#include "MQTTSNAggregateConnectionHandler.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"
#include <string.h>

using namespace std;
using namespace MQTTSNGW;

/*=====================================
 Class MQTTSNAggregateConnectionHandler
 =====================================*/
MQTTSNAggregateConnectionHandler::MQTTSNAggregateConnectionHandler(Gateway* gateway)
{
    _gateway = gateway;
}

MQTTSNAggregateConnectionHandler::~MQTTSNAggregateConnectionHandler()
{

}

/*
 *  CONNECT
 */
void MQTTSNAggregateConnectionHandler::handleConnect(Client* client, MQTTSNPacket* packet)
{
    MQTTSNPacket_connectData data;
    if (packet->getCONNECT(&data) == 0)
    {
        return;
    }

    /* return CONNACK when the client is sleeping */
    if (client->isSleep() || client->isAwake())
    {
        MQTTSNPacket* connack = new MQTTSNPacket();
        MQTTSNPacket_connackData connackData;
        memset(&connackData, 0, sizeof(connackData));
        connackData.reasonCode = MQTT_SN_RC_SUCCESS;
        connack->setCONNACK(&connackData);
        Event* ev = new Event();
        ev->setClientSendEvent(client, connack);
        _gateway->getClientSendQue()->post(ev);
        sendStoredPublish(client);
        return;
    }

    /* clear ConnectData of Client */
    Connect* connectData = client->getConnectData();
    memset(connectData, 0, sizeof(Connect));
    client->disconnected();

    Topics* topics = client->getTopics();

    client->setSessionStatus(false);
    if (data.flags.bits.cleanStart)
    {
        client->clearWaitedPubTopicId();
        client->clearWaitedSubTopicId();

        if (topics)
        {
            Topic* tp = topics->getFirstTopic();
            while (tp != nullptr)
            {
                if (tp->getType() == MQTTSN_TOPIC_TYPE_NAME)
                {
                    _gateway->getAdapterManager()->getAggregater()->removeAggregateTopic(tp, client);
                }
                tp = topics->getNextTopic(tp);
            }
            topics->eraseNormal();
        }
        client->setSessionStatus(true);
    }

    /* In the aggregating gateway, just send CONNACK directly (no broker CONNECT) */
    MQTTSNPacket* connack = new MQTTSNPacket();
    MQTTSNPacket_connackData connackData;
    memset(&connackData, 0, sizeof(connackData));
    connackData.reasonCode = MQTT_SN_RC_SUCCESS;
    connack->setCONNACK(&connackData);
    Event* ev = new Event();
    ev->setClientSendEvent(client, connack);
    _gateway->getClientSendQue()->post(ev);
    client->connackSended(MQTT_SN_RC_SUCCESS);
    sendStoredPublish(client);
}

/*
 *  DISCONNECT
 */
void MQTTSNAggregateConnectionHandler::handleDisconnect(Client* client, MQTTSNPacket* packet)
{
    MQTTSNPacket* snMsg = new MQTTSNPacket();
    snMsg->setDISCONNECT(MQTT_SN_RC_NORMAL_DISCONNECT);
    Event* evt = new Event();
    evt->setClientSendEvent(client, snMsg);
    _gateway->getClientSendQue()->post(evt);
}

/*
 *  PINGREQ
 */
void MQTTSNAggregateConnectionHandler::handlePingreq(Client* client, MQTTSNPacket* packet)
{
    if ((client->isSleep() || client->isAwake()) && client->getClientSleepPacket())
    {
        sendStoredPublish(client);
        client->holdPingRequest();
    }

    /* create and send PINGRESP to the PacketHandler */
    client->resetPingRequest();

    MQTTGWPacket* pingresp = new MQTTGWPacket();
    pingresp->setHeader(PINGRESP);

    Event* evt = new Event();
    evt->setBrokerRecvEvent(client, pingresp);
    _gateway->getPacketEventQue()->post(evt);
}

void MQTTSNAggregateConnectionHandler::sendStoredPublish(Client* client)
{
    MQTTGWPacket* msg = nullptr;

    while ((msg = client->getClientSleepPacket()) != nullptr)
    {
        client->deleteFirstClientSleepPacket(); // pop the que to delete element.

        Event* ev = new Event();
        ev->setBrokerRecvEvent(client, msg);
        _gateway->getPacketEventQue()->post(ev);
    }
}
