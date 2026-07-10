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

#include <stdlib.h>
#include <string.h>

#include "LMqttsnClientApp.h"
#include "LTimer.h"
#include "LGwProxy.h"
#include "LMqttsnClient.h"
#include "LRegisterManager.h"

using namespace std;
using namespace linuxAsyncClient;

extern void setUint16(uint8_t* pos, uint16_t val);
extern LMqttsnClient* theClient;
/*=====================================
 Class RegisterQue
 =====================================*/
LRegisterManager::LRegisterManager()
{
	_first = 0;
	_last = 0;
}

LRegisterManager::~LRegisterManager()
{
	RegQueElement* elm = _first;
	RegQueElement* sav = 0;
	while (elm)
	{
		sav = elm->next;
		if (elm != 0)
		{
			free(elm);
		}
		elm = sav;
	}
}

RegQueElement* LRegisterManager::add(const char* topic, uint16_t msgId)
{
	RegQueElement* elm = (RegQueElement*) calloc(1, sizeof(RegQueElement));

	if (elm)
	{
		if (_last == 0)
		{
			_first = elm;
			_last = elm;
		}
		else
		{
			elm->prev = _last;
			_last->next = elm;
			_last = elm;
		}
		elm->topicName = topic;
		elm->msgId = msgId;
		elm->retryCount = MQTTSN_RETRY_COUNT;
		elm->sendUTC = 0;
	}
	return elm;
}

void LRegisterManager::remove(RegQueElement* elm)
{
	if (elm)
		{
			if (elm->prev == 0)
			{
				_first = elm->next;
				if (elm->next == 0)
				{
					_last = 0;
				}
				else
				{
					elm->next->prev = 0;
					_last = elm->next;
				}
			}
			else
			{
				if ( elm->next == 0 )
				{
					_last = elm->prev;
				}
				elm->prev->next = elm->next;
			}
			free(elm);
		}
}

bool LRegisterManager::isDone(void)
{
	return _first == 0;
}

const char* LRegisterManager::getTopic(uint16_t msgId)
{
	RegQueElement* elm = _first;
	while (elm)
	{
		if (elm->msgId == msgId)
		{
			return elm->topicName;
		}
		else
		{
			elm = elm->next;
		}
	}
	return 0;
}

void LRegisterManager::send(RegQueElement* elm)
{
	uint8_t msg[MQTTSN_MAX_MSG_LENGTH + 1];
	msg[0] = 6 + strlen(elm->topicName);
	msg[1] = MQTTSN_TYPE_REGISTER;
	msg[2] = msg[3] = 0;
	setUint16(msg + 4, elm->msgId);
	strcpy((char*) msg + 6, elm->topicName);
	theClient->getGwProxy()->connect();
	theClient->getGwProxy()->writeMsg(msg);
	elm->sendUTC = time(NULL);
	elm->retryCount--;
}

RegQueElement* LRegisterManager::getElement(const char* topicName)
{
	RegQueElement* elm = _first;
	while (elm)
	{
		if (strcmp(elm->topicName, topicName))
		{
			elm = elm->next;
		}
		else
		{
			return elm;
		}
	}
	return 0;
}

RegQueElement* LRegisterManager::getElement(uint16_t msgId)
{
	RegQueElement* elm = _first;
	while (elm)
	{
		if (elm->msgId == msgId)
		{
			break;
		}
		else
		{
			elm = elm->next;
		}
	}
	return elm;
}

void LRegisterManager::registerTopic(char* topicName)
{
	RegQueElement* elm = getElement(topicName);
	if (elm == 0)
	{
		uint16_t msgId = theClient->getGwProxy()->getNextMsgId();
		elm = add(topicName, msgId);
		send(elm);
	}
}

void LRegisterManager::responceRegAck(uint16_t msgId, uint16_t topicId)
{
	const char* topicName = getTopic(msgId);
	MQTTSN_topicTypes type = MQTTSN_TOPIC_TYPE_NORMAL;
	if (topicName)
	{
		theClient->getGwProxy()->getTopicTable()->setTopicId((char*) topicName, topicId,  type); // Add Topic to TopicTable
		RegQueElement* elm = getElement(msgId);
		remove(elm);
		theClient->getPublishManager()->sendSuspend((char*) topicName, topicId, type);
	}
}

void LRegisterManager::responceRegister(uint8_t* msg, uint16_t msglen)
{
	// *msg is terminated with 0x00 by Network::getMessage()
	uint8_t regack[7];
	regack[0] = 7;
	regack[1] = MQTTSN_TYPE_REGACK;
	memcpy(regack + 2, msg + 1, 4);

	LTopic* tp = theClient->getGwProxy()->getTopicTable()->match((char*) msg + 5);
	if (tp)
	{
		TopicCallback callback = tp->getCallback();
		void* topicName = calloc(strlen((char*) msg + 5) + 1, sizeof(char));
		theClient->getGwProxy()->getTopicTable()->add((char*) topicName, MQTTSN_TOPIC_TYPE_NORMAL, 0, callback, 1);
		regack[6] = MQTTSN_RC_ACCEPTED;
	}
	else
	{
		regack[6] = MQTTSN_RC_REJECTED_INVALID_TOPIC_ID;
	}
	theClient->getGwProxy()->writeMsg(regack);
}

uint8_t LRegisterManager::checkTimeout(void)
{
	RegQueElement* elm = _first;
	RegQueElement* sav;
	while (elm)
	{
		if (elm->sendUTC + MQTTSN_TIME_RETRY < time(NULL))
		{
			if (elm->retryCount >= 0)
			{
				send(elm);
			}
			else
			{
				if (elm->next)
				{
					sav = elm->prev;
					remove(elm);
					if (sav)
					{
						elm = sav;
					}
					else
					{
						break;
					}
				}
				else
				{
					remove(elm);
					break;
				}
			}
		}
		elm = elm->next;
	}
	return 0;
}
