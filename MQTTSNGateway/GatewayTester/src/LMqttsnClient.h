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

#ifndef MQTTSNCLIENT_H_
#define MQTTSNCLIENT_H_

#include <stdio.h>
#include <string.h>

#include "LGwProxy.h"
#include "LMqttsnClientApp.h"
#include "LTimer.h"
#include "Payload.h"
#include "LPublishManager.h"
#include "LSubscribeManager.h"
#include "LTaskManager.h"

using namespace std;

namespace linuxAsyncClient {

struct OnPublishList
{
    MQTTSN_topicTypes type;
	const char* topic;
	uint16_t id;
	int (*pubCallback)(uint8_t* payload, uint16_t payloadlen);
	uint8_t qos;
};

/*========================================
       Class LMqttsnClient
 =======================================*/
class LMqttsnClient{
public:
    LMqttsnClient();
    ~LMqttsnClient();
    void onConnect(void);
    void publish(const char* topicName, Payload* payload, uint8_t qos, bool retain = false);
    void publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false);
    void publish(uint16_t topicId, Payload* payload, uint8_t qos, bool retain = false);
    void publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false);
    void subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos);
    void subscribePredefinedId(uint16_t topicId, TopicCallback onPublish, uint8_t qos);
    void unsubscribe(const char* topicName);
    void unsubscribe(const uint16_t topicId);
    void disconnect(uint16_t sleepInSecs);
    void initialize(SENSORNET_CONFIG_t* netconf, LMqttsnConfig* mqconf);
    void run(void);
    void addTask(bool test);
    void setSleepDuration(uint32_t duration);
    void setSleepMode(uint32_t duration);
	void setAutoConnectMode(uint8_t flg);
    void sleep(void);
	const char* getClientId(void);
	uint16_t getTopicId(const char* topicName);
    LGwProxy*          getGwProxy(void);
    LPublishManager*   getPublishManager(void);
    LSubscribeManager* getSubscribeManager(void);
    LRegisterManager*  getRegisterManager(void);
    LTaskManager*      getTaskManager(void);
    LTopicTable*       getTopicTable(void);
private:
    LTaskManager      _taskMgr;
    LPublishManager   _pubMgr;
    LSubscribeManager _subMgr;
    LGwProxy          _gwProxy;
    uint32_t          _sleepDuration;
	uint8_t _isAutoConnect;
};


} /* end of namespace */
#endif /* MQTTSNCLIENT_H_ */
