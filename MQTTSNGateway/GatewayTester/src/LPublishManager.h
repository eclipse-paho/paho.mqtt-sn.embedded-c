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
#ifndef PUBLISHMANAGER_H_
#define PUBLISHMANAGER_H_

#include <time.h>
#include "LMqttsnClientApp.h"
#include "LTimer.h"
#include "LTopicTable.h"
#include "Payload.h"

using namespace std;

namespace linuxAsyncClient {

#define TOPICID_IS_SUSPEND  0
#define TOPICID_IS_READY    1
#define WAIT_PUBACK         2
#define WAIT_PUBREC         3
#define WAIT_PUBREL         4
#define WAIT_PUBCOMP        5

#define SAVE_TASK_INDEX     1
#define NEG_TASK_INDEX      0


typedef struct PubElement{
    uint16_t  msgId;
    uint16_t  topicId;
    const char* topicName;
    uint8_t*  payload;
    uint16_t  payloadlen;
    time_t    sendUTC;
    int       (*callback)(void);
    int       retryCount;
    int       taskIndex;
    PubElement* prev;
    PubElement* next;
    uint8_t   flag;
    uint8_t   status;  // 0:SUSPEND, 1:READY
} PubElement;

/*========================================
       Class LPublishManager
 =======================================*/
class LPublishManager{
public:
	LPublishManager();
    ~LPublishManager();
    void publish(const char* topicName, Payload* payload, uint8_t qos, bool retain = false);
    void publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false);
    void publish(uint16_t topicId, Payload* payload, uint8_t qos, bool retain = false);
    void publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false);
    void responce(const uint8_t* msg, uint16_t msglen);
    void published(uint8_t* msg, uint16_t msglen);
    void checkTimeout(void);
    void sendSuspend(const char* topicName, uint16_t topicId, uint8_t topicType);
    bool isDone(void);
    bool isMaxFlight(void);
	void setAutoConnectMode(bool);
private:
    PubElement* getElement(uint16_t msgId);
    PubElement* getElement(const char* topicName);
    PubElement* add(const char* topicName, uint16_t topicId, uint8_t* payload, uint16_t len,
    		        uint8_t qos, uint8_t retain, uint16_t msgId, uint8_t topicType);
	void remove(PubElement* elm);
	void sendPublish(PubElement* elm);
	void sendPubAck(uint16_t topicId, uint16_t msgId, uint8_t rc);
    void sendPubRel(PubElement* elm);
    void delElement(PubElement* elm);
	PubElement* _first;
	PubElement* _last;
	uint8_t     _elmCnt;
	uint8_t     _publishedFlg;
	uint8_t _autoConnectFlg;
};

} /* tomyAsyncClient */
#endif /* PUBLISHMANAGER_H_ */
