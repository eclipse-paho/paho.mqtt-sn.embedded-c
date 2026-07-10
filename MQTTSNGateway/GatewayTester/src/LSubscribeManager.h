/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
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

#ifndef SUBSCRIBEMANAGER_H_
#define SUBSCRIBEMANAGER_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "LMqttsnClientApp.h"
#include "LRegisterManager.h"
#include "LTimer.h"
#include "LTopicTable.h"

using namespace std;

namespace linuxAsyncClient {

typedef struct SubElement{
    TopicCallback callback;
    const char* topicName;
    uint16_t  msgId;
    time_t    sendUTC;
    uint16_t  topicId;
    uint8_t   msgType;
    MQTTSN_topicTypes   topicType;
    uint8_t   qos;

    int       retryCount;
    uint8_t   done;

    SubElement* prev;
    SubElement* next;
} SubElement;

/*========================================
       Class LSubscribeManager
 =======================================*/
class LSubscribeManager{
public:
    LSubscribeManager();
    ~LSubscribeManager();
    void onConnect(void);
    void subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos);
	void subscribePredefinedId(uint16_t topicId, TopicCallback onPublish, uint8_t qos);
    void unsubscribe(const char* topicName);
    void unsubscribe(uint16_t topicId);
    void responce(const uint8_t* msg);
    void checkTimeout(void);
    bool isDone(void);
private:
    void send(SubElement* elm);
    SubElement* getFirstElement(void);
    SubElement* getElement(uint16_t msgId);
    SubElement* getElement(uint16_t topicId, MQTTSN_topicTypes topicType);
    SubElement* getElement(const char* topicName, uint8_t msgType);
	SubElement* add(uint8_t msgType, const char* topicName, MQTTSN_topicTypes topicType, uint16_t topicId, uint8_t qos, TopicCallback callback);
	void remove(SubElement* elm);
	SubElement* _first;
	SubElement* _last;
};
 
} /* end of namespace */
#endif /* SUBSCRIBEMANAGER_H_ */
