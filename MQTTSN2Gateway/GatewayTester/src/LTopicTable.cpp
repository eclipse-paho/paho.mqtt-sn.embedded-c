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
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "LMqttsnClientApp.h"
#include "LTopicTable.h"

using namespace std;
using namespace linuxAsyncClient;
/*=====================================
        Class Topic
 ======================================*/
LTopic::LTopic(){
    _topicStr = 0;
    _callback = 0;
    _topicId = 0;
    _topicType = MQTTSN_TOPIC_TYPE_NORMAL;
    _next = 0;
    _malocFlg = 0;
}

LTopic::~LTopic(){
	if (_malocFlg){
		free(_topicStr);
	}
}

TopicCallback LTopic::getCallback(void){
	return _callback;
}

int LTopic::execCallback(uint8_t* payload, uint16_t payloadlen){
    if(_callback != 0){
        return _callback(payload, payloadlen);
    }
    return 0;
}


uint8_t LTopic::hasWildCard(uint8_t* pos){
	*pos = strlen(_topicStr) - 1;
    if (*(_topicStr + *pos) == '#'){
        return MQTTSN_TOPIC_MULTI_WILDCARD;
    }else{
    	for(uint8_t p = 0; p < strlen(_topicStr); p++){
    		if (*(_topicStr + p) == '+'){
    			*pos = p;
    			return MQTTSN_TOPIC_SINGLE_WILDCARD;
    		}
    	}
    }
    return 0;
}

bool LTopic::isMatch(const char* topic){
    uint8_t pos;

	if ( strlen(topic) < strlen(_topicStr)){
		return false;
	}

	uint8_t wc = hasWildCard(&pos);

	if (wc == MQTTSN_TOPIC_SINGLE_WILDCARD){
		if ( strncmp(_topicStr, topic, pos - 1) == 0){
			if (*(_topicStr + pos + 1) == '/'){
				for(uint8_t p = pos; p < strlen(topic); p++){
					if (*(topic + p) == '/'){
						if (strcmp(_topicStr + pos + 1, topic + p ) == 0){
							return true;
						}
					}
				}
			}else{
				for(uint8_t p = pos + 1;p < strlen(topic); p++){
					if (*(topic + p) == '/'){
						return false;
					}
				}
			}
			return true;
		}
	}else if (wc == MQTTSN_TOPIC_MULTI_WILDCARD){
		if (strncmp(_topicStr, topic, pos) == 0){
			return true;
		}
	}else if (strcmp(_topicStr, topic) == 0){
		return true;
	}
	return false;
}


/*=====================================
        Class TopicTable
 ======================================*/
LTopicTable::LTopicTable(){
	_first = 0;
	_last = 0;
}

LTopicTable::~LTopicTable(){
	clearTopic();
}


LTopic* LTopicTable::getTopic(const char* topic){
	LTopic* p = _first;
	while(p){
		if (p->_topicStr != 0 && strcmp(p->_topicStr, topic) == 0){
			return p;
		}
		p = p->_next;
	}
	return 0;
}

LTopic* LTopicTable::getTopic(uint16_t topicId, MQTTSN_topicTypes topicType){
	LTopic* p = _first;
	while(p){
		if (p->_topicId == topicId && p->_topicType == topicType){
			return p;
		}
		p = p->_next;
	}
	return 0;
}

uint16_t LTopicTable::getTopicId(const char* topic){
	LTopic* p = getTopic(topic);
	if (p){
		return p->_topicId;
	}
	return 0;
}


char* LTopicTable::getTopicName(LTopic* topic){
	return topic->_topicStr;
}


void LTopicTable::setTopicId(const char* topic, uint16_t id, MQTTSN_topicTypes type){
    LTopic* tp = getTopic(topic);
    if (tp){
        tp->_topicId = id;
    }else{
    	add(topic, type, id, 0);
    }
}


bool LTopicTable::setCallback(const char* topic, TopicCallback callback){
	LTopic* p = getTopic(topic);
	if (p){
		p->_callback = callback;
		return true;
	}
	return false;
}


bool LTopicTable::setCallback(uint16_t topicId, MQTTSN_topicTypes topicType, TopicCallback callback){
	LTopic* p = getTopic(topicId, topicType);
	if (p){
		p->_callback = callback;
		return true;
	}
	return false;
}


int LTopicTable::execCallback(uint16_t  topicId, uint8_t* payload, uint16_t payloadlen, MQTTSN_topicTypes topicType){
	LTopic* p = getTopic(topicId, topicType);
	if (p){;
		return p->execCallback(payload, payloadlen);
	}
	return 0;
}


LTopic* LTopicTable::add(const char* topicName, MQTTSN_topicTypes type, uint16_t id, TopicCallback callback, uint8_t alocFlg)
{
	LTopic* elm;

    if (topicName){
	    elm = getTopic(topicName);
    }else{
        elm = getTopic(id, type);
    }
    
	if (elm == 0){
		elm = new LTopic();
		if(elm == 0){
			goto exit;
		}
		if ( _last == 0){
			_first = elm;
			_last = elm;
		}
		else
		{
			elm->_prev = _last;
			_last->_next = elm;
			_last = elm;
		}

		elm->_topicStr =  const_cast <char*>(topicName);
		elm->_topicId = id;
		elm->_topicType = type;
		elm->_callback = callback;
		elm->_malocFlg = alocFlg;
		elm->_prev = 0;
	}else{
		elm->_callback = callback;
	}
exit:
	return elm;
}

void LTopicTable::remove(uint16_t topicId, MQTTSN_topicTypes type)
{
	LTopic* elm = getTopic(topicId, type);

	if (elm){
		if (elm->_prev == 0)
		{
			_first = elm->_next;
			if (elm->_next == 0)
			{
				_last = 0;
			}
			else
			{
				elm->_next->_prev = 0;
				_last = elm->_next;
			}
		}
		else
		{
			if ( elm->_next == 0 )
			{
				_last = elm->_prev;
			}
			elm->_prev->_next = elm->_next;
		}
		delete elm;
	}
}

LTopic* LTopicTable::match(const char* topicName){
	LTopic* elm = _first;
	while(elm){
		if (elm->isMatch(topicName)){
			break;
		}
		elm = elm->_next;
	}
	return elm;
}


void LTopicTable::clearTopic(void){
	LTopic* p = _first;
	while(p){
		_first = p->_next;
		delete p;
		p = _first;
	}
	_last = 0;
}
