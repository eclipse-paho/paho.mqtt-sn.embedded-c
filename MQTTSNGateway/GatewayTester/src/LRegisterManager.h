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

#ifndef REGISTERQUE_H_
#define REGISTERQUE_H_

#include <time.h>
#include "LMqttsnClientApp.h"

namespace linuxAsyncClient {
/*======================================
      structure LRegisterQue
 ======================================*/
typedef struct RegQueElement{
	const char* topicName;
	uint16_t msgId;
    int      retryCount;
    time_t   sendUTC;
	RegQueElement* prev;
	RegQueElement* next;
}RegQueElement;

class LRegisterManager{
public:
	LRegisterManager();
	~LRegisterManager();
	void registerTopic(char* topicName);
	void responceRegAck(uint16_t msgId, uint16_t topicId);
	void responceRegister(uint8_t* msg, uint16_t msglen);
	bool isDone(void);
	uint8_t checkTimeout();
	const char* getTopic(uint16_t msgId);
private:
	RegQueElement* getElement(const char* topicName);
	RegQueElement* getElement(uint16_t msgId);
	RegQueElement* add(const char* topicName, uint16_t msgId);
	void remove(RegQueElement* elm);
	void send(RegQueElement* elm);
	RegQueElement* _first;
	RegQueElement* _last;
};
}

#endif /* REGISTERQUE_H_ */
