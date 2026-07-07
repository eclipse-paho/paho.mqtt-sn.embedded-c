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

#ifndef MQTTSNGWPACKETHANDLETASK_H_
#define MQTTSNGWPACKETHANDLETASK_H_

#include "Timer.h"
#include "MQTTSNGWProcess.h"
namespace MQTTSNGW
{
class Gateway;
class Client;
class MQTTSNPacket;
class MQTTGWPacket;
class Timer;
class MQTTGWConnectionHandler;
class MQTTGWPublishHandler;
class MQTTGWSubscribeHandler;
class MQTTSNConnectionHandler;
class MQTTSNPublishHandler;
class MQTTSNSubscribeHandler;

class MQTTSNAggregateConnectionHandler;

#define ERRNO_APL_01  11    // Task Initialize Error

class Thread;
class Timer;
/*=====================================
 Class PacketHandleTask
 =====================================*/
class PacketHandleTask: public Thread
{
MAGIC_WORD_FOR_THREAD;
    friend class MQTTGWAggregatePublishHandler;
    friend class MQTTGWAggregateSubscribeHandler;
    friend class MQTTSNAggregateConnectionHandler;
    friend class MQTTSNAggregatePublishHandler;
    friend class MQTTSNAggregateSubscribeHandler;
public:
    PacketHandleTask(Gateway* gateway);
    ~PacketHandleTask();
    void run();
private:
    void aggregatePacketHandler(Client*client, MQTTSNPacket* packet);
    void aggregatePacketHandler(Client*client, MQTTGWPacket* packet);
    void transparentPacketHandler(Client*client, MQTTSNPacket* packet);
    void transparentPacketHandler(Client*client, MQTTGWPacket* packet);

    Gateway* _gateway
    { nullptr };
    Timer _advertiseTimer;
    Timer _sendUnixTimer;
    MQTTGWConnectionHandler* _mqttConnection { nullptr };
    MQTTGWPublishHandler* _mqttPublish { nullptr };
    MQTTGWSubscribeHandler* _mqttSubscribe { nullptr };
    MQTTSNConnectionHandler* _mqttsnConnection { nullptr };
    MQTTSNPublishHandler* _mqttsnPublish { nullptr };
    MQTTSNSubscribeHandler* _mqttsnSubscribe { nullptr };
    MQTTSNAggregateConnectionHandler* _mqttsnAggrConnection { nullptr };
};

}

#endif /* MQTTSNGWPACKETHANDLETASK_H_ */
