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
 **************************************************************************************/
#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"
#include "MQTTSNGWVersion.h"
#include "MQTTSNGWClientRecvTask.h"
#include "MQTTSNGWClientSendTask.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWAggregater.h"
#include "MQTTSNGWQoSm1Proxy.h"
#include <string.h>
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Class AdapterManager
 =====================================*/
AdapterManager::AdapterManager(Gateway* gw)
{
    _gateway = gw;
    _forwarders = new ForwarderList();
    _qosm1Proxy = new QoSm1Proxy(gw);
    _aggregater = new Aggregater(gw);
}

void AdapterManager::initialize(char* gwName, bool aggregate, bool forwarder, bool qosM1)
{
    if (aggregate)
    {
        _aggregater->initialize(gwName);
    }

    if (qosM1)
    {
        _qosm1Proxy->initialize(gwName);
    }

    if (forwarder)
    {
        _forwarders->initialize(_gateway);
    }
}

AdapterManager::~AdapterManager(void)
{
    if (_forwarders)
    {
        delete _forwarders;
    }
    if (_qosm1Proxy)
    {
        delete _qosm1Proxy;
    }
    if (_aggregater)
    {
        delete _aggregater;
    }
}

ForwarderList* AdapterManager::getForwarderList(void)
{
    return _forwarders;
}

QoSm1Proxy* AdapterManager::getQoSm1Proxy(void)
{
    return _qosm1Proxy;
}

Aggregater* AdapterManager::getAggregater(void)
{
    return _aggregater;
}

bool AdapterManager::isAggregatedClient(Client* client)
{
    if (!_aggregater->isActive() || client->isQoSm1() || client->isAggregater() || client->isQoSm1Proxy())
    {
        return false;
    }
    else
    {
        return true;
    }
}

Client* AdapterManager::getClient(Client* client)
{
    bool secure = client->isSecureNetwork();
    Client* newClient = client;

    if (client->isQoSm1())
    {
        newClient = _qosm1Proxy->getAdapterClient(client);
        _qosm1Proxy->resetPingTimer(secure);
    }
    else if (client->isAggregated())
    {
        newClient = _aggregater->getAdapterClient(client);
        _aggregater->resetPingTimer(secure);
    }
    else if (client->isQoSm1Proxy())
    {
        _qosm1Proxy->resetPingTimer(secure);
    }
    else if (client->isAggregater())
    {
        _aggregater->resetPingTimer(secure);
    }
    return newClient;
}

int AdapterManager::unicastToClient(Client* client, MQTTSNPacket* packet, ClientSendTask* task)
{
    char pbuf[SIZE_OF_LOG_PACKET * 3];
    Forwarder* fwd = client->getForwarder();
    int rc = 0;

    if (fwd)
    {
        MQTTSNGWEncapsulatedPacket encap(packet);
        WirelessNodeId* wnId = fwd->getWirelessNodeId(client);
        encap.setWirelessNodeId(wnId);
        task->log(client, packet);
        WRITELOG(FORMAT_Y_W_G, currentDateTime(), encap.getName(), RIGHTARROW, fwd->getId(), encap.print(pbuf));
        rc = encap.unicast(_gateway->getSensorNetwork(), fwd->getSensorNetAddr());
    }
    else
    {
        task->log(client, packet);
        if (client->isQoSm1Proxy())
        {
            _qosm1Proxy->send(packet, client);
        }
        else if (client->isAggregater())
        {
            _aggregater->send(packet, client);
        }
        else
        {
            rc = packet->unicast(_gateway->getSensorNetwork(), client->getSensorNetAddress());
        }
    }
    return rc;
}

void AdapterManager::checkConnection(void)
{
    if (_aggregater->isActive())
    {
        _aggregater->checkConnection();
    }

    if (_qosm1Proxy->isActive())
    {
        _qosm1Proxy->checkConnection();
    }
}

Client* AdapterManager::convertClient(uint16_t msgId, uint16_t* clientMsgId)
{
    return _aggregater->convertClient(msgId, clientMsgId);
}

bool AdapterManager::isAggregaterActive(void)
{
    return _aggregater->isActive();
}

/*
 AggregateTopicElement* AdapterManager::findTopic(Topic* topic)
 {
 return _aggregater->findTopic(topic);
 }

 AggregateTopicElement* AdapterManager::addAggregateTopic(Topic* topic, Client* client)
 {
 return _aggregater->addAggregateTopic(topic, client);
 }


 void AdapterManager::removeAggregateTopic(Topic* topic, Client* client)
 {
 //_aggregater->removeAggregateTopic(topic, client);
 }

 void AdapterManager::removeAggregateTopicList(Topics* topics, Client* client)
 {

 }
 */

