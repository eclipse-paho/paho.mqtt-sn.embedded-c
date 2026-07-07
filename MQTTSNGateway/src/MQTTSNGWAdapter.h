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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWADAPTER_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWADAPTER_H_

#include <stdint.h>
#include "Timer.h"
namespace MQTTSNGW
{
class Gateway;
class Client;
class Proxy;
class SensorNetAddress;
class MQTTSNPacket;
class MQTTSNGWPacket;
class EventQue;
class Timer;

/* When you add a new type, Client::setAdapterType() and Client::isAdapter() functions must be modified. */
typedef enum
{
    Atype_QoSm1Proxy, Atype_Aggregater
} AdapterType;

/*=====================================
 Class Adapter
 =====================================*/
class Adapter
{
public:
    Adapter(Gateway* gw);
    ~Adapter(void);

    void setup(const char* adpterName, AdapterType adapterType);
    const char* getClientId(SensorNetAddress* addr);
    void setClient(Client* client, bool secure);
    Client* getClient(SensorNetAddress* addr);
    Client* getClient(void);
    Client* getSecureClient(void);
    Client* getAdapterClient(Client* client);
    void resetPingTimer(bool secure);
    void checkConnection(void);
    void send(MQTTSNPacket* packet, Client* client);
    bool isActive(void);
    bool isSecure(SensorNetAddress* addr);
    bool isSecure(void);
    void savePacket(Client* client, MQTTSNPacket* packet);

private:
    Gateway* _gateway { nullptr };
    Proxy* _proxy { nullptr };
    Proxy* _proxySecure { nullptr };
    Client* _client { nullptr };
    Client* _clientSecure { nullptr };
    bool _isActive { false };
    bool _isSecure { false };
};

/*=====================================
 Class Proxy
 =====================================*/
class Proxy
{
public:
    Proxy(Gateway* gw);
    ~Proxy(void);

    void setKeepAlive(uint16_t secs);
    void checkConnection(Client* client);
    void resetPingTimer(void);
    void recv(MQTTSNPacket* packet, Client* client);
    void savePacket(Client* client, MQTTSNPacket* packet);

private:
    void sendSuspendedPacket(void);
    Gateway* _gateway;
    EventQue* _suspendedPacketEventQue
    { nullptr };
    Timer _keepAliveTimer;
    Timer _responseTimer;
    bool _isWaitingResp { false };
    int _retryCnt { 0 };
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWADAPTER_H_ */
