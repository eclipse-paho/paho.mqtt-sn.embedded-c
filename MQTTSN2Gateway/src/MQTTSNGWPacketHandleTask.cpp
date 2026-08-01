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

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWPacketHandleTask.h"
#include "MQTTSNGWProcess.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWProcess.h"
#include "MQTTSNGWAdapterManager.h"
#include "MQTTGWConnectionHandler.h"
#include "MQTTGWPublishHandler.h"
#include "MQTTGWSubscribeHandler.h"
#include "MQTTSNGWConnectionHandler.h"
#include "MQTTSNGWPublishHandler.h"
#include "MQTTSNGWSubscribeHandler.h"
#include "Timer.h"
#include "MQTTSNAggregateConnectionHandler.h"

#include <string.h>

using namespace std;
using namespace MQTTSNGW;

#define EVENT_QUE_TIME_OUT  2000      // 2000 msecs
char* currentDateTime(void);

/*=====================================
 Class PacketHandleTask
 =====================================*/

PacketHandleTask::PacketHandleTask(Gateway* gateway)
{
    _gateway = gateway;
    _gateway->attach((Thread*) this);
    _mqttConnection = new MQTTGWConnectionHandler(_gateway);
    _mqttPublish = new MQTTGWPublishHandler(_gateway);
    _mqttSubscribe = new MQTTGWSubscribeHandler(_gateway);
    _mqttsnConnection = new MQTTSNConnectionHandler(_gateway);
    _mqttsnPublish = new MQTTSNPublishHandler(_gateway);
    _mqttsnSubscribe = new MQTTSNSubscribeHandler(_gateway);

    _mqttsnAggrConnection = new MQTTSNAggregateConnectionHandler(_gateway);
    setTaskName("PacketHandleTask");
}

/**
 *  Destructor is called by Gateway's destructor indirectly.
 */
PacketHandleTask::~PacketHandleTask()
{
    if (_mqttConnection)
    {
        delete _mqttConnection;
    }
    if (_mqttPublish)
    {
        delete _mqttPublish;
    }
    if (_mqttSubscribe)
    {
        delete _mqttSubscribe;
    }
    if (_mqttsnConnection)
    {
        delete _mqttsnConnection;
    }
    if (_mqttsnPublish)
    {
        delete _mqttsnPublish;
    }
    if (_mqttsnSubscribe)
    {
        delete _mqttsnSubscribe;
    }

    if (_mqttsnAggrConnection)
    {
        delete _mqttsnAggrConnection;
    }
}

void PacketHandleTask::run()
{
    Event* ev = nullptr;
    EventQue* eventQue = _gateway->getPacketEventQue();
    AdapterManager* adpMgr = _gateway->getAdapterManager();

    Client* client = nullptr;
    MQTTSNPacket* snPacket = nullptr;
    MQTTGWPacket* brPacket = nullptr;
    char msgId[6];
    memset(msgId, 0, 6);

    _advertiseTimer.start(_gateway->getGWParams()->keepAlive * 1000UL);

    while (true)
    {
        /* wait Event */
        ev = eventQue->timedwait(EVENT_QUE_TIME_OUT);

        if (ev->getEventType() == EtStop)
        {
            WRITELOG("%s %s stopped.\n", currentDateTime(), getTaskName());
            delete ev;
            return;
        }

        if (ev->getEventType() == EtTimeout)
        {
            /*------ Check Keep Alive Timer & send Advertise ------*/
            if (_advertiseTimer.isTimeup())
            {
                _mqttsnConnection->sendADVERTISE();
                _advertiseTimer.start(_gateway->getGWParams()->keepAlive * 1000UL);
            }

            /*------ Check Adapters   Connect or PINGREQ ------*/
            adpMgr->checkConnection();
        }

        /*------    Handle SEARCHGW Message     ---------*/
        else if (ev->getEventType() == EtBroadcast)
        {
            snPacket = ev->getMQTTSNPacket();
            _mqttsnConnection->handleSearchgw(snPacket);
        }

        /*------    Handle Messages form Clients      ---------*/
        else if (ev->getEventType() == EtClientRecv)
        {
            client = ev->getClient();
            snPacket = ev->getMQTTSNPacket();

            DEBUGLOG("     PacketHandleTask gets %s %s from the client.\n", snPacket->getName(), snPacket->getMsgId(msgId));

            if (adpMgr->isAggregatedClient(client))
            {
                aggregatePacketHandler(client, snPacket); // client is converted to Aggregater by BrokerSendTask
            }
            else
            {
                transparentPacketHandler(client, snPacket);
            }

            /* Reset the Timer for PINGREQ. */
            client->updateStatus(snPacket);
        }
        /*------  Handle Messages form Broker      ---------*/
        else if (ev->getEventType() == EtBrokerRecv)
        {
            client = ev->getClient();
            brPacket = ev->getMQTTGWPacket();
            DEBUGLOG("     PacketHandleTask gets %s %s from the broker.\n", brPacket->getName(), brPacket->getMsgId(msgId));

            if (client->isAggregater())
            {
                aggregatePacketHandler(client, brPacket);
            }
            else
            {
                transparentPacketHandler(client, brPacket);
            }
        }
        delete ev;
    }
}

void PacketHandleTask::aggregatePacketHandler(Client*client, MQTTSNPacket* packet)
{
    switch (packet->getType())
    {
    case MQTTSN_CONNECT:
        _mqttsnAggrConnection->handleConnect(client, packet);
        break;
    case MQTTSN_DISCONNECT:
        _mqttsnAggrConnection->handleDisconnect(client, packet);
        break;
    case MQTTSN_SLEEPREQ:
        _mqttsnConnection->handleSleepReq(client, packet);
        break;
    case MQTTSN_WAKEUP:
        _mqttsnConnection->handleWakeup(client, packet);
        break;
    case MQTTSN_PINGREQ:
        _mqttsnAggrConnection->handlePingreq(client, packet);
        break;
    case MQTTSN_PUBLISH:
        _mqttsnPublish->handleAggregatePublish(client, packet);
        break;
    case MQTTSN_PUBACK:
        _mqttsnPublish->handleAggregateAck(client, packet, MQTTSN_PUBACK);
        break;
    case MQTTSN_PUBREC:
        _mqttsnPublish->handleAggregateAck(client, packet, MQTTSN_PUBREC);
        break;
    case MQTTSN_PUBREL:
        _mqttsnPublish->handleAggregateAck(client, packet, MQTTSN_PUBREL);
        break;
    case MQTTSN_PUBCOMP:
        _mqttsnPublish->handleAggregateAck(client, packet, MQTTSN_PUBCOMP);
        break;
    case MQTTSN_REGISTER:
        _mqttsnPublish->handleRegister(client, packet);
        break;
    case MQTTSN_REGACK:
        _mqttsnPublish->handleRegAck(client, packet);
        break;
    case MQTTSN_SUBSCRIBE:
        _mqttsnSubscribe->handleAggregateSubscribe(client, packet);
        break;
    case MQTTSN_UNSUBSCRIBE:
        _mqttsnSubscribe->handleAggregateUnsubscribe(client, packet);
        break;
    default:
        break;
    }
}

void PacketHandleTask::aggregatePacketHandler(Client*client, MQTTGWPacket* packet)
{
    switch (packet->getType())
    {
    case CONNACK:
        _mqttConnection->handleConnack(client, packet);
        break;
    case PINGRESP:
        _mqttConnection->handlePingresp(client, packet);
        break;
    case PUBLISH:
        _mqttPublish->handleAggregatePublish(client, packet);
        break;
    case PUBACK:
        _mqttPublish->handleAggregatePuback(client, packet);
        break;
    case PUBREC:
        _mqttPublish->handleAggregateAck(client, packet, PUBREC);
        break;
    case PUBREL:
        _mqttPublish->handleAggregatePubrel(client, packet);
        break;
    case PUBCOMP:
        _mqttPublish->handleAggregateAck(client, packet, PUBCOMP);
        break;
    case SUBACK:
        _mqttSubscribe->handleAggregateSuback(client, packet);
        break;
    case UNSUBACK:
        _mqttSubscribe->handleAggregateUnsuback(client, packet);
        break;
    default:
        break;
    }
}

void PacketHandleTask::transparentPacketHandler(Client*client, MQTTSNPacket* packet)
{
    switch (packet->getType())
    {
    case MQTTSN_CONNECT:
        _mqttsnConnection->handleConnect(client, packet);
        break;
    case MQTTSN_DISCONNECT:
        _mqttsnConnection->handleDisconnect(client, packet);
        break;
    case MQTTSN_SLEEPREQ:
        _mqttsnConnection->handleSleepReq(client, packet);
        break;
    case MQTTSN_WAKEUP:
        _mqttsnConnection->handleWakeup(client, packet);
        break;
    case MQTTSN_PINGREQ:
        _mqttsnConnection->handlePingreq(client, packet);
        break;
    case MQTTSN_PUBLISH:
        _mqttsnPublish->handlePublish(client, packet);
        break;
    case MQTTSN_PUBWOS:
        _mqttsnPublish->handlePublish(client, packet);
        break;
    case MQTTSN_PUBACK:
        _mqttsnPublish->handlePuback(client, packet);
        break;
    case MQTTSN_PUBREC:
        _mqttsnPublish->handleAck(client, packet, PUBREC);
        break;
    case MQTTSN_PUBREL:
        _mqttsnPublish->handleAck(client, packet, PUBREL);
        break;
    case MQTTSN_PUBCOMP:
        _mqttsnPublish->handleAck(client, packet, PUBCOMP);
        break;
    case MQTTSN_REGISTER:
        _mqttsnPublish->handleRegister(client, packet);
        break;
    case MQTTSN_REGACK:
        _mqttsnPublish->handleRegAck(client, packet);
        break;
    case MQTTSN_SUBSCRIBE:
        _mqttsnSubscribe->handleSubscribe(client, packet);
        break;
    case MQTTSN_UNSUBSCRIBE:
        _mqttsnSubscribe->handleUnsubscribe(client, packet);
        break;
    default:
        break;
    }
}

void PacketHandleTask::transparentPacketHandler(Client*client, MQTTGWPacket* packet)
{
    switch (packet->getType())
    {
    case CONNACK:
        _mqttConnection->handleConnack(client, packet);
        break;
    case PINGRESP:
        _mqttConnection->handlePingresp(client, packet);
        break;
    case PUBLISH:
        _mqttPublish->handlePublish(client, packet);
        break;
    case PUBACK:
        _mqttPublish->handlePuback(client, packet);
        break;
    case PUBREC:
        _mqttPublish->handleAck(client, packet, PUBREC);
        break;
    case PUBREL:
        _mqttPublish->handleAck(client, packet, PUBREL);
        break;
    case PUBCOMP:
        _mqttPublish->handleAck(client, packet, PUBCOMP);
        break;
    case SUBACK:
        _mqttSubscribe->handleSuback(client, packet);
        break;
    case UNSUBACK:
        _mqttSubscribe->handleUnsuback(client, packet);
        break;
    case DISCONNECT:
        client->disconnected(); // Just change Client's status to "Disconnected"
        break;
    default:
        break;
    }
}

