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

#include <stdlib.h>
#include <string.h>

#include "LMqttsnClientApp.h"
#include "LTimer.h"
#include "LScreen.h"
#include "LGwProxy.h"
#include "LMqttsnClient.h"
#include "LSubscribeManager.h"

using namespace std;
using namespace linuxAsyncClient;

extern void setUint16(uint8_t* pos, uint16_t val);
extern uint16_t getUint16(const uint8_t* pos);
extern LMqttsnClient* theClient;
extern SUBSCRIBE_LIST;
extern LScreen* theScreen;
#define SUB_DONE   1
#define SUB_READY  0
/*========================================
 Class SubscribeManager
 =======================================*/
LSubscribeManager::LSubscribeManager()
{
    _first = 0;
    _last = 0;
}

LSubscribeManager::~LSubscribeManager()
{
    SubElement* elm = _first;
    SubElement* sav = 0;
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

void LSubscribeManager::onConnect(void)
{
    DISPLAY("\033[0m\033[0;32m Attempting OnConnect.....\033[0m\033[0;37m\n");
    if (_first == 0)
    {
        for (uint8_t i = 0; theOnPublishList[i].topic != 0; i++)
        {
            if ( theOnPublishList[i].type == MQTTSN_TOPIC_TYPE_PREDEFINED)
            {
				subscribePredefinedId(theOnPublishList[i].id,
						theOnPublishList[i].pubCallback,
						theOnPublishList[i].qos);
            }
            else
            {
                subscribe(theOnPublishList[i].topic, theOnPublishList[i].pubCallback, theOnPublishList[i].qos);
            }
        }
    }
    else
    {
        SubElement* elm = _first;
        SubElement* pelm;
        do
        {
            pelm = elm;
            if (elm->msgType == MQTTSN_TYPE_SUBSCRIBE)
            {
                elm->done = SUB_READY;
                elm->retryCount = MQTTSN_RETRY_COUNT;
                subscribe(elm->topicName, elm->callback, elm->qos);
            }
            elm = pelm->next;
        } while (pelm->next);
    }

    while (!theClient->getSubscribeManager()->isDone())
    {
        theClient->getGwProxy()->getMessage();
    }
    DISPLAY("\033[0m\033[0;32m OnConnect complete\033[0m\033[0;37m\n");
    DISPLAY("\033[0m\033[0;32m Test is Ready.\033[0m\033[0;37m\n");
}

bool LSubscribeManager::isDone(void)
{
    SubElement* elm = _first;
    SubElement* prevelm;
    while (elm)
    {
        prevelm = elm;
        if (elm->done == SUB_READY)
        {
            return false;
        }
        elm = prevelm->next;
    }
    return true;
}

void LSubscribeManager::send(SubElement* elm)
{
    if (elm->done == SUB_DONE)
    {
        return;
    }

    uint8_t msg[MQTTSN_MAX_MSG_LENGTH + 1];
    if (elm->topicType == MQTTSN_TOPIC_TYPE_PREDEFINED)
    {
        msg[0] = 7;
        setUint16(msg + 5, elm->topicId);
    }
    else
    {
        msg[0] = 5 + strlen(elm->topicName);
        strcpy((char*) msg + 5, elm->topicName);
    }
    msg[1] = elm->msgType;
    msg[2] = elm->qos | elm->topicType;
    if (elm->retryCount == MQTTSN_RETRY_COUNT)
    {
        elm->msgId = theClient->getGwProxy()->getNextMsgId();
    }

    if ((elm->retryCount < MQTTSN_RETRY_COUNT) && elm->msgType == MQTTSN_TYPE_SUBSCRIBE)
    {
        msg[2] = msg[2] | MQTTSN_FLAG_DUP;
    }

    setUint16(msg + 3, elm->msgId);

    theClient->getGwProxy()->connect();
    theClient->getGwProxy()->writeMsg(msg);
    theClient->getGwProxy()->setPingReqTimer();
    elm->sendUTC = time(NULL);
    elm->retryCount--;
}

void LSubscribeManager::subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos)
{
    MQTTSN_topicTypes topicType;
    if ( strlen(topicName) > 2 )
    {
        topicType = MQTTSN_TOPIC_TYPE_NORMAL;
    }
    else
    {
        topicType = MQTTSN_TOPIC_TYPE_SHORT;
    }
    SubElement* elm = add(MQTTSN_TYPE_SUBSCRIBE, topicName, topicType, 0,  qos, onPublish);
    send(elm);
}

void LSubscribeManager::subscribePredefinedId(uint16_t topicId, TopicCallback onPublish, uint8_t qos)
{
    SubElement* elm = add(MQTTSN_TYPE_SUBSCRIBE, 0, MQTTSN_TOPIC_TYPE_PREDEFINED, topicId,  qos, onPublish);
    send(elm);
}

void LSubscribeManager::unsubscribe(const char* topicName)
{
    MQTTSN_topicTypes topicType;
    if ( strlen(topicName) > 2 )
    {
        topicType = MQTTSN_TOPIC_TYPE_NORMAL;
    }
    else
    {
        topicType = MQTTSN_TOPIC_TYPE_SHORT;
    }
    SubElement* elm = add(MQTTSN_TYPE_UNSUBSCRIBE, topicName, topicType, 0, 0, 0);
    send(elm);
}

void LSubscribeManager::unsubscribe( uint16_t topicId)
{
    SubElement* elm = add(MQTTSN_TYPE_UNSUBSCRIBE, 0, MQTTSN_TOPIC_TYPE_PREDEFINED, topicId, 0, 0);
    send(elm);
}

void LSubscribeManager::checkTimeout(void)
{
    SubElement* elm = _first;

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
                if ( elm->done == SUB_READY )
                {
                    if (elm->msgType == MQTTSN_TYPE_SUBSCRIBE)
                    {
                        DISPLAY("\033[0m\033[0;31m\n!!!!!! SUBSCRIBE  Error !!!!! Topic : %s\033[0m\033[0;37m\n\n", (char*)elm->topicName);
                    }else{
                        DISPLAY("\033[0m\033[0;31m\n!!!!!! UNSUBSCRIBE  Error !!!!! Topic : %s\033[0m\033[0;37m\n\n", (char*)elm->topicName);
                    }
                    elm->done = SUB_DONE;
                }
            }
        }
        elm = elm->next;
    }
}

void LSubscribeManager::responce(const uint8_t* msg)
{
    if (msg[0] == MQTTSN_TYPE_SUBACK)
    {
        uint16_t topicId = getUint16(msg + 2);
        uint16_t msgId = getUint16(msg + 4);
        uint8_t rc = msg[6];

        SubElement* elm = getElement(msgId);
        if (elm)
        {
            if ( rc == MQTTSN_RC_ACCEPTED )
            {
                theClient->getGwProxy()->getTopicTable()->add((char*) elm->topicName, elm->topicType, topicId, elm->callback);
                getElement(msgId)->done = SUB_DONE;
                DISPLAY("\033[0m\033[0;32m Topic \"%s\" Id : %d was Subscribed. \033[0m\033[0;37m\n\n", getElement(msgId)->topicName, topicId);
            }
            else
            {
                DISPLAY("\033[0m\033[0;31m SUBACK Invalid messageId. %s\033[0m\033[0;37m\n\n", getElement(msgId)->topicName);
                remove(elm);
            }
        }
    }
    else if (msg[0] == MQTTSN_TYPE_UNSUBACK)
    {
        uint16_t msgId = getUint16(msg + 1);
        SubElement* elm = getElement(msgId);
        if (elm)
        {
            //theClient->getGwProxy()->getTopicTable()->setCallback(elm->topicName, 0);
            DISPLAY("\033[0m\033[0;32m Topic  \"%s\"  was Unsubscribed. \033[0m\033[0;37m\n\n", getElement(msgId)->topicName);
            remove(elm);
        }
        else
        {
			DISPLAY(
					"\033[0m\033[0;31m UNSUBACK Invalid messageId=%04x. \033[0m\033[0;37m\n\n",
					msgId);
        }
    }
}

/* SubElement operations */

SubElement* LSubscribeManager::add(uint8_t msgType, const char* topicName, MQTTSN_topicTypes topicType, uint16_t topicId,
        uint8_t qos, TopicCallback callback)
{
    SubElement* elm = 0;
    if (topicName )
    {
        elm = getElement(topicName, msgType);
    }
    else
    {
        elm = getElement(topicId, topicType);
    }

    if ( elm  == 0 )
    {
        elm = (SubElement*) calloc(1, sizeof(SubElement));
        if (elm == 0)
        {
            return 0;
        }
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
    }

    elm->msgType = msgType;
    elm->callback = callback;
    elm->topicName = topicName;
    elm->topicId = topicId;
    elm->topicType = topicType;

    if (qos == 1)
    {
        elm->qos = MQTTSN_FLAG_QOS_1;
    }
    else if (qos == 2)
    {
        elm->qos = MQTTSN_FLAG_QOS_2;
    }
    else
    {
        elm->qos = MQTTSN_FLAG_QOS_0;
    }
    elm->msgId = 0;
    elm->retryCount = MQTTSN_RETRY_COUNT;
    elm->done = SUB_READY;
    elm->sendUTC = 0;

    return elm;
}

void LSubscribeManager::remove(SubElement* elm)
{
    if (elm)
    {
        if (elm->prev == 0)
        {
            _first = elm->next;
            if (elm->next != 0)
            {
                elm->next->prev = 0;
                _last = elm->next;
            }
            free(elm);
        }
        else
        {
            if ( elm->next == 0 )
            {
                _last = elm->prev;
            }
            elm->prev->next = elm->next;
            free(elm);
        }
    }
}

SubElement* LSubscribeManager::getElement(uint16_t msgId)
{
    SubElement* elm = _first;
    while (elm)
    {
        if (elm->msgId == msgId)
        {
            return elm;
        }
        else
        {
            elm = elm->next;
        }
    }
    return 0;
}

SubElement* LSubscribeManager::getElement(const char* topicName, uint8_t msgType)
{
    SubElement* elm = _first;
    while (elm)
    {
        if ( elm->msgType == msgType &&  strncmp(elm->topicName, topicName, strlen(topicName)) == 0 )
        {
            return elm;
        }
        else
        {
            elm = elm->next;
        }
    }
    return 0;
}

SubElement* LSubscribeManager::getElement(uint16_t topicId, MQTTSN_topicTypes topicType)
{
    SubElement* elm = _first;
    while (elm)
    {
        if (elm->topicId == topicId && elm->topicType == topicType)
        {
            return elm;
        }
        else
        {
            elm = elm->next;
        }
    }
    return 0;
}
