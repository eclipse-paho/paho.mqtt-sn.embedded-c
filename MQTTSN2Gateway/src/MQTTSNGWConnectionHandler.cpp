/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi, Ian Craggs and others
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

#include "MQTTSNGWConnectionHandler.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"
#include <string.h>

using namespace std;
using namespace MQTTSNGW;

/*=====================================
 Class MQTTSNConnectionHandler
 =====================================*/
MQTTSNConnectionHandler::MQTTSNConnectionHandler(Gateway* gateway)
{
    _gateway = gateway;
}

MQTTSNConnectionHandler::~MQTTSNConnectionHandler()
{

}

/*
 *  ADVERTISE
 */
void MQTTSNConnectionHandler::sendADVERTISE()
{
    MQTTSNPacket* adv = new MQTTSNPacket();
    adv->setADVERTISE(_gateway->getGWParams()->gatewayId, _gateway->getGWParams()->keepAlive);
    Event* ev1 = new Event();
    ev1->setBrodcastEvent(adv);  //broadcast
    _gateway->getClientSendQue()->post(ev1);
}

/*
 *  SEARCHGW
 */
void MQTTSNConnectionHandler::handleSearchgw(MQTTSNPacket* packet)
{
    if (packet->getType() == MQTTSN_SEARCHGW)
    {
        MQTTSNPacket* gwinfo = new MQTTSNPacket();
        gwinfo->setGWINFO(_gateway->getGWParams()->gatewayId);
        Event* ev1 = new Event();
        ev1->setBrodcastEvent(gwinfo);
        _gateway->getClientSendQue()->post(ev1);
    }
}

/*
 *  CONNECT (v2.0 — will is embedded in the CONNECT packet)
 */
void MQTTSNConnectionHandler::handleConnect(Client* client, MQTTSNPacket* packet)
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
    if (!client->isAdapter())
    {
        client->disconnected();
    }

    Topics* topics = client->getTopics();

    /* store the client ID */
    client->setClientId(data.clientID);

    /* prepare MQTT CONNECT data */
    connectData->header.bits.type = CONNECT;
    connectData->clientID = client->getClientId();
    connectData->version = _gateway->getGWParams()->mqttVersion;
    connectData->keepAliveTimer = data.keepAlive;
    connectData->flags.bits.will = data.flags.bits.will;

    if ((const char*) _gateway->getGWParams()->loginId != nullptr)
    {
        connectData->flags.bits.username = 1;
    }
    if ((const char*) _gateway->getGWParams()->password != 0)
    {
        connectData->flags.bits.password = 1;
    }

    client->setSessionStatus(false);
    if (data.flags.bits.cleanStart)
    {
        connectData->flags.bits.cleanstart = 1;
        client->clearWaitedPubTopicId();
        client->clearWaitedSubTopicId();
        if (topics)
        {
            topics->eraseNormal();
        }
        client->setSessionStatus(true);
    }

    /* In v2.0 the will is embedded in CONNECT — extract and forward directly */
    char* willTopicBuf = nullptr;
    char* willMsgBuf = nullptr;

    if (data.flags.bits.will)
    {
        if (data.will.topic.type == MQTTSN_TOPIC_TYPE_NAME && data.will.topic.alt.string.len > 0)
        {
            willTopicBuf = (char*) calloc(data.will.topic.alt.string.len + 1, 1);
            if (willTopicBuf)
            {
                memcpy(willTopicBuf, data.will.topic.alt.string.data, data.will.topic.alt.string.len);
            }
        }
        if (data.will.payload.len > 0 && data.will.payload.data)
        {
            willMsgBuf = (char*) calloc(data.will.payload.len + 1, 1);
            if (willMsgBuf)
            {
                memcpy(willMsgBuf, data.will.payload.data, data.will.payload.len);
            }
        }
        connectData->willTopic = willTopicBuf;
        connectData->willMsg = willMsgBuf;
        connectData->flags.bits.willQoS = data.willFlags.bits.QoS;
        connectData->flags.bits.willRetain = data.willFlags.bits.retain;
    }

    /* create MQTT CONNECT & send it to the broker */
    MQTTGWPacket* mqMsg = new MQTTGWPacket();
    mqMsg->setCONNECT(connectData, (unsigned char*) _gateway->getGWParams()->loginId,
            (unsigned char*) _gateway->getGWParams()->password);
    Event* ev1 = new Event();
    ev1->setBrokerSendEvent(client, mqMsg);
    _gateway->getBrokerSendQue()->post(ev1);

    /* free temporary buffers after setCONNECT has serialised them */
    if (willTopicBuf)
    {
        free(willTopicBuf);
        connectData->willTopic = nullptr;
    }
    if (willMsgBuf)
    {
        free(willMsgBuf);
        connectData->willMsg = nullptr;
    }
}

/*
 *  DISCONNECT
 */
void MQTTSNConnectionHandler::handleDisconnect(Client* client, MQTTSNPacket* packet)
{
    MQTTSNPacket_disconnectData data;
    if (packet->getDISCONNECT(&data) != 0)
    {
        MQTTGWPacket* mqMsg = new MQTTGWPacket();
        mqMsg->setHeader(DISCONNECT);
        Event* ev = new Event();
        ev->setBrokerSendEvent(client, mqMsg);
        _gateway->getBrokerSendQue()->post(ev);
    }

    MQTTSNPacket* snMsg = new MQTTSNPacket();
    snMsg->setDISCONNECT(MQTT_SN_RC_NORMAL_DISCONNECT);
    Event* evt = new Event();
    evt->setClientSendEvent(client, snMsg);
    _gateway->getClientSendQue()->post(evt);
}

/*
 *  SLEEPREQ — client requests to enter sleep mode
 */
void MQTTSNConnectionHandler::handleSleepReq(Client* client, MQTTSNPacket* packet)
{
    uint8_t retainTopicAliases = 0;
    uint16_t packetid = 0;
    uint32_t duration = 0;

    if (packet->getSLEEPREQ(&retainTopicAliases, &packetid, &duration) == 0)
    {
        return;
    }

    MQTTSNPacket* resp = new MQTTSNPacket();
    resp->setSLEEPRESP(0, packetid, 0, MQTT_SN_RC_SUCCESS);
    Event* ev = new Event();
    ev->setClientSendEvent(client, resp);
    _gateway->getClientSendQue()->post(ev);
}

/*
 *  WAKEUP — client wakes from sleep
 */
void MQTTSNConnectionHandler::handleWakeup(Client* client, MQTTSNPacket* packet)
{
    sendStoredPublish(client);

    MQTTSNPacket* resp = new MQTTSNPacket();
    resp->setWAKEUP();
    Event* ev = new Event();
    ev->setClientSendEvent(client, resp);
    _gateway->getClientSendQue()->post(ev);
}

/*
 *  PINGREQ
 */
void MQTTSNConnectionHandler::handlePingreq(Client* client, MQTTSNPacket* packet)
{
    uint16_t packetid = 0;
    packet->getPINGREQ(&packetid);
    client->setPingReqPacketId(packetid);

    if ((client->isSleep() || client->isAwake()) && client->getClientSleepPacket())
    {
        sendStoredPublish(client);
        client->holdPingRequest();
    }
    else
    {
        client->resetPingRequest();
        /* send PINGREQ to the broker to maintain connection keep-alive */
        MQTTGWPacket* pingreq = new MQTTGWPacket();
        pingreq->setHeader(PINGREQ);
        Event* evt = new Event();
        evt->setBrokerSendEvent(client, pingreq);
        _gateway->getBrokerSendQue()->post(evt);
    }
}

void MQTTSNConnectionHandler::sendStoredPublish(Client* client)
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
