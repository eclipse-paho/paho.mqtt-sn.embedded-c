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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWFORWARDER_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWFORWARDER_H_

#include "MQTTSNGWClient.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWEncapsulatedPacket.h"
#include "SensorNetwork.h"

namespace MQTTSNGW
{
class Gateway;
class Client;
class WirelessNodeId;

/*=====================================
 Class ForwarderElement
 =====================================*/
class ForwarderElement
{
    friend class Forwarder;
public:
    ForwarderElement();
    ~ForwarderElement();

    void setClient(Client* client);
    void setWirelessNodeId(WirelessNodeId* id);
private:
    Client* _client;
    WirelessNodeId* _wirelessNodeId;
    ForwarderElement* _next;
};

/*=====================================
 Class Forwarder
 =====================================*/
class Forwarder
{
    friend class ForwarderList;
public:
    Forwarder(void);
    Forwarder(SensorNetAddress* addr, MQTTSNString* forwarderId);
    ~Forwarder();

    void initialize(void);
    const char* getId(void);
    void addClient(Client* client, WirelessNodeId* id);
    Client* getClient(WirelessNodeId* id);
    WirelessNodeId* getWirelessNodeId(Client* client);
    void eraseClient(Client* client);
    SensorNetAddress* getSensorNetAddr(void);
    const char* getName(void);

private:
    string _forwarderName;
    SensorNetAddress _sensorNetAddr;
    ForwarderElement* _headClient { nullptr };
    Forwarder* _next { nullptr };
    Mutex _mutex;
};

/*=====================================
 Class ForwarderList
 =====================================*/
class ForwarderList
{
public:
    ForwarderList();
    ~ForwarderList();

    void initialize(Gateway* gw);
    Forwarder* getForwarder(SensorNetAddress* addr);
    Forwarder* addForwarder(SensorNetAddress* addr, MQTTSNString* forwarderId);

private:
    Forwarder* _head;
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWFORWARDER_H_ */
