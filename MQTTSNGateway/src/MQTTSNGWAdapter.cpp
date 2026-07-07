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
#include "Timer.h"
#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWAdapter.h"
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"
#include "MQTTSNGWClient.h"

#include <string.h>
using namespace MQTTSNGW;

/*=====================================
 Class Adapter
 =====================================*/
Adapter::Adapter(Gateway* gw)
{
    _gateway = gw;
    _proxy = new Proxy(gw);
    _proxySecure = new Proxy(gw);
}

Adapter::~Adapter(void)
{
    if (_proxy)
    {
        delete _proxy;
    }

    if (_proxySecure)
    {
        delete _proxySecure;
    }
}

void Adapter::setup(const char* adpterName, AdapterType adapterType)
{
    _isSecure = false;
    if (_gateway->hasSecureConnection())
    {
        _isSecure = true;
    }

    MQTTSNString id = MQTTSNString_initializer;
    MQTTSNString idSecure = MQTTSNString_initializer;

    string name = string(adpterName);
    id.cstring = const_cast<char*>(name.c_str());
    string nameSecure = string(adpterName) + "-S";
    idSecure.cstring = const_cast<char*>(nameSecure.c_str());

    Client* client = _gateway->getClientList()->createClient(0, &id, true, false, TRANSPEARENT_TYPE);
    setClient(client, false);
    client->setAdapterType(adapterType);

    client = _gateway->getClientList()->createClient(0, &idSecure, true, true,
    TRANSPEARENT_TYPE);
    setClient(client, true);
    client->setAdapterType(adapterType);
}

Client* Adapter::getClient(SensorNetAddress* addr)
{
    Client* client = _gateway->getClientList()->getClient(addr);
    if (!client)
    {
        return nullptr;
    }
    else if (client->isQoSm1())
    {
        return client;
    }
    else
    {
        return nullptr;
    }
}

const char* Adapter::getClientId(SensorNetAddress* addr)
{
    Client* client = getClient(addr);
    if (!client)
    {
        return nullptr;
    }
    else if (client->isQoSm1())
    {
        return client->getClientId();
    }
    else
    {
        return nullptr;
    }
}

bool Adapter::isSecure(SensorNetAddress* addr)
{
    Client* client = getClient(addr);
    if (!client)
    {
        return false;
    }
    else if (client->isSecureNetwork())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Adapter::isSecure(void)
{
    return _isSecure;
}

void Adapter::setClient(Client* client, bool secure)
{
    if (secure)
    {
        _clientSecure = client;
    }
    else
    {
        _client = client;
    }
}

Client* Adapter::getClient(void)
{
    return _client;
}

Client* Adapter::getSecureClient(void)
{
    return _clientSecure;
}

void Adapter::checkConnection(void)
{
    _proxy->checkConnection(_client);

    if (_isSecure)
    {
        _proxySecure->checkConnection(_clientSecure);
    }
}

void Adapter::send(MQTTSNPacket* packet, Client* client)
{
    Proxy* proxy = _proxy;
    if (client->isSecureNetwork() && !_isSecure)
    {
        if (_isSecure)
        {
            proxy = _proxySecure;
        }
        else
        {
            WRITELOG("%s %s  No Secure connections %s 's packet is discarded.%s\n",
            ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);
            return;
        }
    }

    proxy->recv(packet, client);

}

void Adapter::resetPingTimer(bool secure)
{
    if (secure)
    {
        _proxySecure->resetPingTimer();
    }
    else
    {
        _proxy->resetPingTimer();
    }
}

bool Adapter::isActive(void)
{
    return _isActive;
}

void Adapter::savePacket(Client* client, MQTTSNPacket* packet)
{
    if (client->isSecureNetwork())
    {
        _proxySecure->savePacket(client, packet);
    }
    else
    {
        _proxy->savePacket(client, packet);
    }
}

Client* Adapter::getAdapterClient(Client* client)
{
    if (client->isSecureNetwork())
    {
        return _clientSecure;
    }
    else
    {
        return _client;
    }
}

/*=====================================
 Class Proxy
 =====================================*/
Proxy::Proxy(Gateway* gw)
{
    _gateway = gw;
    _suspendedPacketEventQue = new EventQue();
}
Proxy::~Proxy(void)
{
    if (_suspendedPacketEventQue)
    {
        delete _suspendedPacketEventQue;
    }
}

void Proxy::checkConnection(Client* client)
{
    if (client->isDisconnect() || (client->isConnecting() && _responseTimer.isTimeup()))
    {
        client->connectSended();
        _responseTimer.start(PROXY_RESPONSE_DURATION * 1000UL);
        MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;
        options.clientID.cstring = client->getClientId();
        options.duration = PROXY_KEEPALIVE_DURATION;

        MQTTSNPacket* packet = new MQTTSNPacket();
        packet->setCONNECT(&options);
        Event* ev = new Event();
        ev->setClientRecvEvent(client, packet);
        _gateway->getPacketEventQue()->post(ev);
    }
    else if ((client->isActive() && _keepAliveTimer.isTimeup()) || (_isWaitingResp && _responseTimer.isTimeup()))
    {
        MQTTSNPacket* packet = new MQTTSNPacket();
        MQTTSNString clientId = MQTTSNString_initializer;
        packet->setPINGREQ(&clientId);
        Event* ev = new Event();
        ev->setClientRecvEvent(client, packet);
        _gateway->getPacketEventQue()->post(ev);
        _responseTimer.start(PROXY_RESPONSE_DURATION * 1000UL);
        _isWaitingResp = true;

        if (++_retryCnt > PROXY_MAX_RETRY_CNT)
        {
            client->disconnected();
        }
        resetPingTimer();
    }
}

void Proxy::resetPingTimer(void)
{
    _keepAliveTimer.start(PROXY_KEEPALIVE_DURATION * 1000UL);
}

void Proxy::recv(MQTTSNPacket* packet, Client* client)
{
    if (packet->getType() == MQTTSN_CONNACK)
    {
        if (packet->isAccepted())
        {
            _responseTimer.stop();
            _retryCnt = 0;
            resetPingTimer();
            sendSuspendedPacket();
        }
    }
    else if (packet->getType() == MQTTSN_PINGRESP)
    {
        _isWaitingResp = false;
        _responseTimer.stop();
        _retryCnt = 0;
        resetPingTimer();
    }
    else if (packet->getType() == MQTTSN_DISCONNECT)
    {
        // blank
    }
}

void Proxy::savePacket(Client* client, MQTTSNPacket* packet)
{
    MQTTSNPacket* pk = new MQTTSNPacket(*packet);
    Event* ev = new Event();
    ev->setClientRecvEvent(client, pk);
    _suspendedPacketEventQue->post(ev);
}

void Proxy::sendSuspendedPacket(void)
{
    while (_suspendedPacketEventQue->size())
    {
        Event* ev = _suspendedPacketEventQue->wait();
        _gateway->getPacketEventQue()->post(ev);
    }
}

