/**************************************************************************************
 * Copyright (c) 2018, 2026 Tomoaki Yamaguchi
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
        MQTTSNPacket* packet = new MQTTSNPacket();
        packet->setCONNACK(MQTTSN_RC_ACCEPTED);
        Event* ev = new Event();
        ev->setClientSendEvent(client, packet);
        _gateway->getClientSendQue()->post(ev);
        sendStoredPublish(client);
        return;
    }

    //* clear ConnectData of Client */
    Connect* connectData = client->getConnectData();
    memset(connectData, 0, sizeof(Connect));

    client->disconnected();

    Topics* topics = client->getTopics();

    /* CONNECT was not sent yet. prepare Connect data */

    client->setSessionStatus(false);
    if (data.cleansession)
    {
        /* reset the table of msgNo and TopicId pare */
        client->clearWaitedPubTopicId();
        client->clearWaitedSubTopicId();

        /* renew the TopicList */
        if (topics)
        {
            Topic* tp = topics->getFirstTopic();
            while (tp != nullptr)
            {
                if (tp->getType() == MQTTSN_TOPIC_TYPE_NORMAL)
                {
                    _gateway->getAdapterManager()->getAggregater()->removeAggregateTopic(tp, client);
                }
                tp = topics->getNextTopic(tp);
            }
            topics->eraseNormal();
        }
        client->setSessionStatus(true);
    }

    if (data.willFlag)
    {
        /* create & send WILLTOPICREQ message to the client */
        MQTTSNPacket* reqTopic = new MQTTSNPacket();
        reqTopic->setWILLTOPICREQ();
        Event* evwr = new Event();
        evwr->setClientSendEvent(client, reqTopic);

        /* Send WILLTOPICREQ to the client */
        _gateway->getClientSendQue()->post(evwr);
    }
    else
    {
        /* create CONNACK & send it to the client */
        MQTTSNPacket* packet = new MQTTSNPacket();
        packet->setCONNACK(MQTTSN_RC_ACCEPTED);
        Event* ev = new Event();
        ev->setClientSendEvent(client, packet);
        _gateway->getClientSendQue()->post(ev);
        client->connackSended(MQTTSN_RC_ACCEPTED);
        sendStoredPublish(client);
        return;
    }
}

/*
 *  WILLMSG
 */
void MQTTSNAggregateConnectionHandler::handleWillmsg(Client* client, MQTTSNPacket* packet)
{
    if (!client->isWaitWillMsg())
    {
        DEBUGLOG("     MQTTSNConnectionHandler::handleWillmsg  WaitWillMsgFlg is off.\n");
        return;
    }

    MQTTSNString willmsg = MQTTSNString_initializer;
    //Connect* connectData = client->getConnectData();

    if (client->isConnectSendable())
    {
        /* save WillMsg in the client */
        if (packet->getWILLMSG(&willmsg) == 0)
        {
            return;
        }
        client->setWillMsg(willmsg);

        /* Send CONNACK to the client */
        MQTTSNPacket* packet = new MQTTSNPacket();
        packet->setCONNACK(MQTTSN_RC_ACCEPTED);
        Event* ev = new Event();
        ev->setClientSendEvent(client, packet);
        _gateway->getClientSendQue()->post(ev);

        sendStoredPublish(client);
        return;
    }
}

/*
 *  DISCONNECT
 */
void MQTTSNAggregateConnectionHandler::handleDisconnect(Client* client, MQTTSNPacket* packet)
{
    MQTTSNPacket* snMsg = new MQTTSNPacket();
    snMsg->setDISCONNECT(0);
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

