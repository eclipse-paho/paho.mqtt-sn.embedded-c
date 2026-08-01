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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWCLIENTLIST_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWCLIENTLIST_H_

#include "MQTTSNGWClient.h"
#include "MQTTSNGateway.h"

namespace MQTTSNGW
{
#define TRANSPEARENT_TYPE 0
#define QOSM1PROXY_TYPE 1
#define AGGREGATER_TYPE 2
#define FORWARDER_TYPE  3

class Client;

/*=====================================
 Class ClientsPool
 =====================================*/
class ClientsPool
{
public:
	ClientsPool();
	~ClientsPool();
	void allocate(int maxClients);
	Client* getClient(void);
	void setClient(Client* client);

private:
	Client* _firstClient;
	Client* _endClient;
	int _clientCnt;
};

/*=====================================
 Class ClientList
 =====================================*/
class ClientList
{
public:
	ClientList(Gateway* gw);
    ~ClientList();

    void initialize(bool aggregate);
    void setClientList(int type);
    void setPredefinedTopics(bool aggregate);
    void erase(Client*&);
    Client* createClient(SensorNetAddress* addr, MQTTSN_string* clientId,
            int type);
    Client* createClient(SensorNetAddress* addr, MQTTSN_string* clientId,
            bool unstableLine, bool secure, int type);
    bool createList(const char* fileName, int type);
    Client* getClient(SensorNetAddress* addr);
    Client* getClient(MQTTSN_string* clientId);
    Client* getClient(int index);
    uint16_t getClientCount(void);
    Client* getClient(void);
    bool isAuthorized();

private:
    bool readPredefinedList(const char* fileName, bool _aggregate);
	ClientsPool* _clientsPool;
	Gateway* _gateway;
    Client* createPredefinedTopic(MQTTSN_string* clientId, string topicName,
            uint16_t toipcId, bool _aggregate);
    Client* _firstClient;
    Client* _endClient;
    Mutex _mutex;
    uint16_t _clientCnt;
    uint16_t _maxClients;
    bool _authorize { false };
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWCLIENTLIST_H_ */
