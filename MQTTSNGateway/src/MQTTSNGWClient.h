/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Tieto Poland Sp. z o.o. - Gateway improvements
 **************************************************************************************/

#ifndef MQTTSNGWCLIENT_H_
#define MQTTSNGWCLIENT_H_

#include <Timer.h>    // Timer class
#include "MQTTSNGWProcess.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNPacket.h"
#include "Network.h"
#include "SensorNetwork.h"
#include "MQTTSNPacket.h"
#include "MQTTSNGWEncapsulatedPacket.h"
#include "MQTTSNGWForwarder.h"
#include "MQTTSNGWTopic.h"
#include "MQTTSNGWClientList.h"
#include "MQTTSNGWAdapter.h"

namespace MQTTSNGW
{

#define MQTTSN_TOPIC_MULTI_WILDCARD   '#'
#define MQTTSN_TOPIC_SINGLE_WILDCARD  '+'

/*=====================================
 Class PacketQue
 =====================================*/
template<class T> class PacketQue
{
public:
    PacketQue()
    {
        _que = new Que<T>;
    }

    ~PacketQue()
    {
        clear();
        delete _que;
    }

    T* getPacket()
    {
        T* packet;
        if (_que->size() > 0)
        {
            _mutex.lock();
            packet = _que->front();
            _mutex.unlock();
            return packet;
        }
        else
        {
            return 0;
        }
    }

    int post(T* packet)
    {
        int rc;
        _mutex.lock();
        rc = _que->post(packet);
        _mutex.unlock();
        return rc;
    }

    void pop()
    {
        if (_que->size() > 0)
        {
            _mutex.lock();
            _que->pop();
            _mutex.unlock();
        }
    }

    void clear()
    {
        _mutex.lock();
        while (_que->size() > 0)
        {
            delete _que->front();
            _que->pop();
        }
        _mutex.unlock();
    }

    void setMaxSize(int size)
    {
        _que->setMaxSize(size);
    }

private:
    Que<T>* _que;
    Mutex _mutex;
};

/*=====================================
 Class WaitREGACKPacket
 =====================================*/
class waitREGACKPacket
{
    friend class WaitREGACKPacketList;
public:
    waitREGACKPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId);
    ~waitREGACKPacket();

private:
    uint16_t _msgId;
    MQTTSNPacket* _packet;
    waitREGACKPacket* _next;
    waitREGACKPacket* _prev;
};

/*=====================================
 Class WaitREGACKPacketList
 =====================================*/
class WaitREGACKPacketList
{
public:
    WaitREGACKPacketList();
    ~WaitREGACKPacketList();
    int setPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId);
    MQTTSNPacket* getPacket(uint16_t REGACKMsgId);
    void erase(uint16_t REGACKMsgId);
    uint8_t getCount(void);

private:
    uint8_t _cnt;
    waitREGACKPacket* _first;
    waitREGACKPacket* _end;
};

/*=====================================
 Class Client
 =====================================*/
typedef enum
{
	Cstat_Free = 0,
	Cstat_Disconnected,
    Cstat_TryConnecting,
    Cstat_Connecting,
    Cstat_Active,
    Cstat_Asleep,
    Cstat_Awake,
    Cstat_Lost
} ClientStatus;

typedef enum
{
    Ctype_Normal = 0,
    Ctype_Forwarded,
    Ctype_QoS_1,
    Ctype_Aggregated,
    Ctype_Proxy,
    Ctype_Aggregater
} ClientType;

class Forwarder;

class Client
{
    friend class ClientList;
    friend class ClientsPool;
public:
    Client();
    ~Client();

    Connect* getConnectData(void);
    TopicIdMapElement* getWaitedPubTopicId(uint16_t msgId);
    TopicIdMapElement* getWaitedSubTopicId(uint16_t msgId);
    MQTTGWPacket* getClientSleepPacket(void);
    void deleteFirstClientSleepPacket(void);

    MQTTSNPacket* getProxyPacket(void);
    void deleteFirstProxyPacket(void);
    WaitREGACKPacketList* getWaitREGACKPacketList(void);

    void eraseWaitedPubTopicId(uint16_t msgId);
    void eraseWaitedSubTopicId(uint16_t msgId);
    void clearWaitedPubTopicId(void);
    void clearWaitedSubTopicId(void);

    int setClientSleepPacket(MQTTGWPacket*);
    int setProxyPacket(MQTTSNPacket* packet);
    void setWaitedPubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicid* topic);
    void setWaitedSubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicid* topic);

    bool checkTimeover(void);
    void updateStatus(MQTTSNPacket*);
    void updateStatus(ClientStatus);
    void connectSended(void);
    void connackSended(int rc);
    void disconnected(void);
    bool isConnectSendable(void);
    void tryConnect(void);
    ClientStatus getClientStatus(void);

    uint16_t getNextPacketId(void);
    uint8_t getNextSnMsgId(void);
    Topics* getTopics(void);
    void setTopics(Topics* topics);
    void setKeepAlive(MQTTSNPacket* packet);

    SensorNetAddress* getSensorNetAddress(void);
    Network* getNetwork(void);
    void setClientAddress(SensorNetAddress* sensorNetAddr);
    void setSensorNetType(bool stable);

    Forwarder* getForwarder(void);
    void setForwarder(Forwarder* forwader);

    void setAdapterType(AdapterType type);
    void setQoSm1(void);
    void setAggregated(void);
    bool isQoSm1Proxy(void);
    bool isForwarded(void);
    bool isAggregated(void);
    bool isAggregater(void);
    bool isQoSm1(void);
    bool isAdapter(void);

    void setClientId(MQTTSNString id);
    void setWillTopic(MQTTSNString willTopic);
    void setWillMsg(MQTTSNString willmsg);
    char* getClientId(void);
    char* getWillTopic(void);
    char* getWillMsg(void);
    const char* getStatus(void);
    void setWaitWillMsgFlg(bool);
    void setSessionStatus(bool);  // true: clean session
    bool erasable(void);

    bool isDisconnect(void);
    bool isConnecting(void);
    bool isActive(void);
    bool isSleep(void);
    bool isAwake(void);
    bool isSecureNetwork(void);
    bool isSensorNetStable(void);
    bool isWaitWillMsg(void);
	bool isCleanSession(void);

    void holdPingRequest(void);
    void resetPingRequest(void);
    bool isHoldPingReqest(void);

    Client* getNextClient(void);

private:
    PacketQue<MQTTGWPacket> _clientSleepPacketQue;
    PacketQue<MQTTSNPacket> _proxyPacketQue;

    WaitREGACKPacketList _waitREGACKList;

    Topics* _topics;
    TopicIdMap _waitedPubTopicIdMap;
    TopicIdMap _waitedSubTopicIdMap;

    Connect _connectData;
    MQTTSNPacket* _connAck;

    char* _clientId;
    char* _willTopic;
    char* _willMsg;

    bool _holdPingRequest;

    Timer _keepAliveTimer;
    uint32_t _keepAliveMsec;

    ClientStatus _status;
    bool _waitWillMsgFlg;

    uint16_t _packetId;
    uint8_t _snMsgId;

    Network* _network;      // Broker
    bool _secureNetwork;    // SSL
    bool _sensorNetype;     // false: unstable network like a G3
    SensorNetAddress _sensorNetAddr;

    Forwarder* _forwarder;
    ClientType _clientType;

    bool _sessionStatus;
    bool _hasPredefTopic;

    Client* _nextClient;
    Client* _prevClient;
};

}
#endif /* MQTTSNGWCLIENT_H_ */
