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

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWClientList.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include <string>
#include <string.h>
#include <stdio.h>

#include "MQTTSNGWForwarder.h"

using namespace MQTTSNGW;
char* currentDateTime(void);

/*=====================================
 Class Client
 =====================================*/
static const char* theClientStatus[] = { "InPool", "Disconnected", "TryConnecting", "Connecting", "Active", "Asleep", "Awake",
        "Lost" };

Client::Client()
{
    _packetId = 0;
    _snMsgId = 0;
    _status = Cstat_Free;
    _keepAliveMsec = 0;
    _topics = new Topics();
    _clientId = nullptr;
    _willTopic = nullptr;
    _willMsg = nullptr;
    _connectData = MQTTPacket_Connect_Initializer;
    _network = new Network();
    _sensorNetype = true;
    _connAck = nullptr;
    _waitWillMsgFlg = false;
    _sessionStatus = false;
    _prevClient = nullptr;
    _nextClient = nullptr;
    _clientSleepPacketQue.setMaxSize(MAX_SAVED_PUBLISH);
    _proxyPacketQue.setMaxSize(MAX_SAVED_PUBLISH);
    _hasPredefTopic = false;
    _holdPingRequest = false;
    _forwarder = nullptr;
    _clientType = Ctype_Normal;
}

Client::~Client()
{
    if (_topics)
    {
        delete _topics;
    }

    if (_clientId)
    {
        free(_clientId);
    }

    if (_willTopic)
    {
        free(_willTopic);
    }

    if (_willMsg)
    {
        free(_willMsg);
    }

    if (_connAck)
    {
        delete _connAck;
    }

    if (_network)
    {
        delete _network;
    }
}

TopicIdMapElement* Client::getWaitedPubTopicId(uint16_t msgId)
{
    return _waitedPubTopicIdMap.getElement(msgId);
}

TopicIdMapElement* Client::getWaitedSubTopicId(uint16_t msgId)
{
    return _waitedSubTopicIdMap.getElement(msgId);
}

MQTTGWPacket* Client::getClientSleepPacket()
{
    return _clientSleepPacketQue.getPacket();
}

void Client::deleteFirstClientSleepPacket()
{
    _clientSleepPacketQue.pop();
}

int Client::setClientSleepPacket(MQTTGWPacket* packet)
{
    int rc = _clientSleepPacketQue.post(packet);
    if (rc)
    {
        WRITELOG("%s    %s is sleeping. the packet was saved.\n", currentDateTime(), _clientId);
    }
    else
    {
        WRITELOG("%s    %s is sleeping　but discard the packet.\n", currentDateTime(), _clientId);
    }
    return rc;
}

MQTTSNPacket* Client::getProxyPacket(void)
{
    return _proxyPacketQue.getPacket();
}

void Client::deleteFirstProxyPacket()
{
    _proxyPacketQue.pop();
}

int Client::setProxyPacket(MQTTSNPacket* packet)
{
    int rc = _proxyPacketQue.post(packet);
    if (rc)
    {
        WRITELOG("%s    %s is Disconnected. the packet was saved.\n", currentDateTime(), _clientId);
    }
    else
    {
        WRITELOG("%s    %s is Disconnected and discard the packet.\n", currentDateTime(), _clientId);
    }
    return rc;
}

Connect* Client::getConnectData(void)
{
    return &_connectData;
}

void Client::eraseWaitedPubTopicId(uint16_t msgId)
{
    _waitedPubTopicIdMap.erase(msgId);
}

void Client::eraseWaitedSubTopicId(uint16_t msgId)
{
    _waitedSubTopicIdMap.erase(msgId);
}

void Client::clearWaitedPubTopicId(void)
{
    _waitedPubTopicIdMap.clear();
}

void Client::clearWaitedSubTopicId(void)
{
    _waitedSubTopicIdMap.clear();
}

void Client::setWaitedPubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicid* topic)
{
    _waitedPubTopicIdMap.add(msgId, topicId, topic);
}

void Client::setWaitedSubTopicId(uint16_t msgId, uint16_t topicId, MQTTSN_topicid* topic)
{
    _waitedSubTopicIdMap.add(msgId, topicId, topic);
}

bool Client::checkTimeover(void)
{
    return (_status == Cstat_Active && _keepAliveTimer.isTimeup());
}

void Client::setKeepAlive(MQTTSNPacket* packet)
{
    MQTTSNPacket_connectData param;
    if (packet->getCONNECT(&param))
    {
        _keepAliveMsec = param.duration * 1000UL;
        _keepAliveTimer.start(_keepAliveMsec * 1.5);
    }
}

void Client::setForwarder(Forwarder* forwarder)
{
    _forwarder = forwarder;
    _clientType = Ctype_Forwarded;
}

Forwarder* Client::getForwarder(void)
{
    return _forwarder;
}

void Client::setSessionStatus(bool status)
{
    _sessionStatus = status;
}

bool Client::erasable(void)
{
    return _sessionStatus && !_hasPredefTopic && _forwarder == nullptr;
}

void Client::updateStatus(MQTTSNPacket* packet)
{
    if (((_status == Cstat_Disconnected) || (_status == Cstat_Lost)) && packet->getType() == MQTTSN_CONNECT)
    {
        setKeepAlive(packet);
    }
    else if (_status == Cstat_Active)
    {
        switch (packet->getType())
        {
        case MQTTSN_PINGREQ:
        case MQTTSN_PUBLISH:
        case MQTTSN_SUBSCRIBE:
        case MQTTSN_UNSUBSCRIBE:
        case MQTTSN_PUBACK:
        case MQTTSN_PUBCOMP:
        case MQTTSN_PUBREL:
        case MQTTSN_PUBREC:
            if (_clientType != Ctype_Proxy)
            {
                _keepAliveTimer.start(_keepAliveMsec * 1.5);
            }
            break;
        case MQTTSN_DISCONNECT:
            uint16_t duration;
            packet->getDISCONNECT(&duration);
            if (duration)
            {
                _status = Cstat_Asleep;
            }
            else
            {
                disconnected();
            }
            break;
        default:
            break;
        }
    }
    else if (_status == Cstat_Awake || _status == Cstat_Asleep)
    {
        switch (packet->getType())
        {
        case MQTTSN_CONNECT:
            _status = Cstat_Active;
            break;
        case MQTTSN_DISCONNECT:
            disconnected();
            break;
        case MQTTSN_PINGREQ:
            _status = Cstat_Awake;
            break;
        case MQTTSN_PINGRESP:
            _status = Cstat_Asleep;
            break;
        default:
            break;
        }
    }DEBUGLOG("Client Status = %s\n", theClientStatus[_status]);
}

void Client::updateStatus(ClientStatus stat)
{
    _status = stat;
}

void Client::connectSended()
{
    _status = Cstat_Connecting;
}

void Client::connackSended(int rc)
{
    if (rc == MQTTSN_RC_ACCEPTED)
    {
        _status = Cstat_Active;
    }
    else
    {
        disconnected();
    }
}

void Client::disconnected(void)
{
    _status = Cstat_Disconnected;
    _waitWillMsgFlg = false;
}

void Client::tryConnect(void)
{
    _status = Cstat_TryConnecting;
}

bool Client::isCleanSession(void)
{
    return _sessionStatus;
}

bool Client::isConnectSendable(void)
{
    if (_status == Cstat_Lost || _status == Cstat_TryConnecting)
    {
        return false;
    }
    else
    {
        return true;
    }
}

uint16_t Client::getNextPacketId(void)
{
    _packetId++;
    if (_packetId == 0xffff)
    {
        _packetId = 1;
    }
    return _packetId;
}

uint8_t Client::getNextSnMsgId(void)
{
    _snMsgId++;
    if (_snMsgId == 0)
    {
        _snMsgId++;
    }
    return _snMsgId;
}

Topics* Client::getTopics(void)
{
    return _topics;
}

Network* Client::getNetwork(void)
{
    return _network;
}

void Client::setClientAddress(SensorNetAddress* sensorNetAddr)
{
    _sensorNetAddr = *sensorNetAddr;
}

SensorNetAddress* Client::getSensorNetAddress(void)
{
    return &_sensorNetAddr;
}

void Client::setSensorNetType(bool stable)
{
    _sensorNetype = stable;
}

void Client::setTopics(Topics* topics)
{
    _topics = topics;
}

ClientStatus Client::getClientStatus(void)
{
    return _status;
}

void Client::setWaitWillMsgFlg(bool flg)
{
    _waitWillMsgFlg = flg;
}

bool Client::isWaitWillMsg(void)
{
    return _waitWillMsgFlg;
}

bool Client::isDisconnect(void)
{
    return (_status == Cstat_Disconnected);
}

bool Client::isActive(void)
{
    return (_status == Cstat_Active);
}

bool Client::isSleep(void)
{
    return (_status == Cstat_Asleep);
}

bool Client::isAwake(void)
{
    return (_status == Cstat_Awake);
}

bool Client::isConnecting(void)
{
    return (_status == Cstat_Connecting);
}

bool Client::isSecureNetwork(void)
{
    return _network->isSecure();
}

bool Client::isSensorNetStable(void)
{
    return _sensorNetype;
}

WaitREGACKPacketList* Client::getWaitREGACKPacketList()
{
    return &_waitREGACKList;
}

Client* Client::getNextClient(void)
{
    return _nextClient;
}

void Client::setClientId(MQTTSNString id)
{
    if (_clientId)
    {
        free(_clientId);
    }

    if (id.cstring)
    {
        _clientId = (char*) calloc(strlen(id.cstring) + 1, 1);
        memcpy(_clientId, id.cstring, strlen(id.cstring));
    }
    else
    {
        /* save clientId into (char*)_clientId NULL terminated */
        _clientId = (char*) calloc(MQTTSNstrlen(id) + 1, 1);
        unsigned char* ptr = (unsigned char*) _clientId;
        writeMQTTSNString((unsigned char**) &ptr, id);
    }
}

void Client::setWillTopic(MQTTSNString willTopic)
{
    if (_willTopic)
    {
        free(_willTopic);
    }

    _willTopic = (char*) calloc(MQTTSNstrlen(willTopic) + 1, 1);
    /* save willTopic into (char*)_willTopic  with NULL termination */
    unsigned char* ptr = (unsigned char*) _willTopic;
    writeMQTTSNString((unsigned char**) &ptr, willTopic);
}

void Client::setWillMsg(MQTTSNString willMsg)
{
    if (_willMsg)
    {
        free(_willMsg);
    }

    _willMsg = (char*) calloc(MQTTSNstrlen(willMsg) + 1, 1);
    /* save willMsg into (char*)_willMsg  with NULL termination */
    unsigned char* ptr = (unsigned char*) _willMsg;
    writeMQTTSNString((unsigned char**) &ptr, willMsg);
}

char* Client::getClientId(void)
{
    return _clientId;
}

char* Client::getWillTopic(void)
{
    return _willTopic;
}

char* Client::getWillMsg(void)
{
    return _willMsg;
}

const char* Client::getStatus(void)
{
    return theClientStatus[_status];
}

bool Client::isQoSm1Proxy(void)
{
    return _clientType == Ctype_Proxy;
}

bool Client::isForwarded(void)
{
    return _clientType == Ctype_Forwarded;
}

bool Client::isAggregated(void)
{
    return _clientType == Ctype_Aggregated;
}

bool Client::isAggregater(void)
{
    return _clientType == Ctype_Aggregater;
}

void Client::setAdapterType(AdapterType type)
{
    switch (type)
    {
    case Atype_QoSm1Proxy:
        _clientType = Ctype_Proxy;
        break;
    case Atype_Aggregater:
        _clientType = Ctype_Aggregater;
        break;
    default:
        throw EXCEPTION("Client::setAdapterType(): Invalid Type.", 0);
        break;
    }
}

bool Client::isAdapter(void)
{
    return _clientType == Ctype_Proxy || _clientType == Ctype_Aggregater;
}

bool Client::isQoSm1(void)
{
    return _clientType == Ctype_QoS_1;
}

void Client::setQoSm1(void)
{
    _clientType = Ctype_QoS_1;
}

void Client::setAggregated(void)
{
    _clientType = Ctype_Aggregated;
}

void Client::holdPingRequest(void)
{
    _holdPingRequest = true;
}

void Client::resetPingRequest(void)
{
    _holdPingRequest = false;
}

bool Client::isHoldPingReqest(void)
{
    return _holdPingRequest;
}

/*=====================================
 Class WaitREGACKPacket
 =====================================*/
waitREGACKPacket::waitREGACKPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId)
{
    _packet = packet;
    _msgId = REGACKMsgId;
    _next = nullptr;
    _prev = nullptr;
}

waitREGACKPacket::~waitREGACKPacket()
{
    delete _packet;
}

/*=====================================
 Class WaitREGACKPacketList
 =====================================*/

WaitREGACKPacketList::WaitREGACKPacketList()
{
    _first = nullptr;
    _end = nullptr;
    _cnt = 0;
}

WaitREGACKPacketList::~WaitREGACKPacketList()
{
    waitREGACKPacket* p = _first;
    while (p)
    {
        waitREGACKPacket* q = p->_next;
        delete p;
        p = q;
    }
}

int WaitREGACKPacketList::setPacket(MQTTSNPacket* packet, uint16_t REGACKMsgId)
{
    waitREGACKPacket* elm = new waitREGACKPacket(packet, REGACKMsgId);
    if (elm == nullptr)
    {
        return 0;
    }

    if (_first == nullptr)
    {
        _first = elm;
        _end = elm;
    }
    else
    {
        _end->_next = elm;
        elm->_prev = _end;
        _end = elm;
    }
    _cnt++;
    return 1;
}

MQTTSNPacket* WaitREGACKPacketList::getPacket(uint16_t REGACKMsgId)
{
    waitREGACKPacket* p = _first;
    while (p)
    {
        if (p->_msgId == REGACKMsgId)
        {
            return p->_packet;
        }
        p = p->_next;
    }
    return nullptr;
}

void WaitREGACKPacketList::erase(uint16_t REGACKMsgId)
{
    waitREGACKPacket* p = _first;
    while (p)
    {
        if (p->_msgId == REGACKMsgId)
        {
            if (p->_prev == nullptr)
            {
                _first = p->_next;

            }
            else
            {
                p->_prev->_next = p->_next;
            }
            if (p->_next == nullptr)
            {
                _end = p->_prev;
            }
            else
            {
                p->_next->_prev = p->_prev;
            }
            _cnt--;
            break;
            // Do not delete element. Element is deleted after sending to Client.
        }
        p = p->_next;
    }
}

uint8_t WaitREGACKPacketList::getCount(void)
{
    return _cnt;
}

