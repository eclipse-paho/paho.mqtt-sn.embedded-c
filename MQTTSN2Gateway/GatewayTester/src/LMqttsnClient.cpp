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

#include "LGwProxy.h"
#include "LMqttsnClientApp.h"
#include "LMqttsnClient.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;

extern TaskList      theTaskList[];
extern TestList      theTestList[];
extern OnPublishList theOnPublishList[];
extern MQTTSNCONF;
extern SENSORNET_CONFIG_t theNetcon;
extern void setup(void);

/*=====================================
          LMqttsnClient
 ======================================*/
LMqttsnClient* theClient = new LMqttsnClient();
LScreen* theScreen = new LScreen();
bool theOTAflag = false;
bool theClientMode = true;


/*-------------------------------------
 *    main
 *------------------------------------*/

int main(int argc, char** argv)
{
#ifndef CLIENT_MODE
	char c = 0;
	printf("\n%s", PAHO_COPYRIGHT4);
    printf("\n%s", PAHO_COPYRIGHT0);
#if defined(UDP)
    printf("UDP ClientId:%s PortNo:%d\n", theNetcon.clientId, theNetcon.uPortNo);
#elif defined(UDP6)
    printf("UDP6 ClientId:%s PortNo:%d\n", theNetcon.clientId, theNetcon.uPortNo);
#elif defined(DTLS)
    printf("DTLS ClientId:%s PortNo:%d\n", theNetcon.clientId, theNetcon.uPortNo);
#elif defined(DTLS6)
    printf("DTLS6 ClientId:%s PortNo:%d\n", theNetcon.clientId, theNetcon.uPortNo);
#elif defined(RFCOMM)
    printf("RFCOMM ClientId:%s channel:%d\n", theNetcon.clientId, theNetcon.channel);
#else
    printf("\n");
#endif
	printf("%s\n", PAHO_COPYRIGHT1);
	printf("%s\n", PAHO_COPYRIGHT2);
	printf(" *\n%s\n", PAHO_COPYRIGHT3);
	printf("%s\n", TESTER_VERSION);
	printf("%s\n", PAHO_COPYRIGHT4);

	theClientMode = false;
	PROMPT(" Do you like Tomoaki ? ( y/n ) : ");
	while (true)
	{
		if (CHECKKEYIN(&c))
		{
			if ( toupper(c) == 'N' )
			{
				DISPLAY("\033[0;31m\n**** Sorry ****\033[0;37m\n\n");
				PROMPT("");
				return 0;
			}
		}
		else if ( toupper(c) == 'Y' )
		{
			DISPLAY("\033[0m\033[0;32mAttempting to Connect the Broker.....\033[0m\033[0;37m\n");
			PROMPT("");
			break;
		}
	}
	theClient->setAutoConnectMode(false);
	theClient->getPublishManager()->setAutoConnectMode(false);
#endif

	setup();
	theClient->addTask(theClientMode);
	theClient->initialize( &theNetcon, &theMqcon);
	do
	{
		theClient->run();
	}
	while (theClientMode);

	delete theScreen;
	delete theClient;
	return 0;
}

/*=====================================
        Class LMqttsnClient
 ======================================*/
LMqttsnClient::LMqttsnClient()
{
	_isAutoConnect = true;
}

LMqttsnClient::~LMqttsnClient()
{

}

void LMqttsnClient::initialize(SENSORNET_CONFIG_t* netconf, LMqttsnConfig* mqconf)
{
	_gwProxy.initialize(netconf, mqconf);
	setSleepDuration(mqconf->sleepDuration);
}

void LMqttsnClient::addTask(bool clientMode)
{
	if ( clientMode )
	{
		_taskMgr.add(theTaskList);
	}
	else
	{
		_taskMgr.add(theTestList);
	}
}


LGwProxy* LMqttsnClient::getGwProxy(void)
{
	return &_gwProxy;
}

LPublishManager* LMqttsnClient::getPublishManager(void)
{
	return &_pubMgr;
}
;

LSubscribeManager* LMqttsnClient::getSubscribeManager(void)
{
	return &_subMgr;
}
;

LRegisterManager* LMqttsnClient::getRegisterManager(void)
{
	return _gwProxy.getRegisterManager();
}

LTaskManager* LMqttsnClient::getTaskManager(void)
{
	return &_taskMgr;
}
;

LTopicTable* LMqttsnClient::getTopicTable(void)
{
	return _gwProxy.getTopicTable();
}

void LMqttsnClient::publish(const char* topicName, Payload* payload, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicName, payload, qos, retain);
}

void LMqttsnClient::publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicName, payload, len, qos, retain);
}

void LMqttsnClient::publish(uint16_t topicId, Payload* payload, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicId, payload, qos, retain);
}

void LMqttsnClient::publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicId, payload, len, qos, retain);
}

void LMqttsnClient::subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos)
{
	_subMgr.subscribe(topicName, onPublish, qos);
}

void LMqttsnClient::subscribePredefinedId(uint16_t topicId, TopicCallback onPublish,
		uint8_t qos)
{
	_subMgr.subscribePredefinedId(topicId, onPublish, qos);
}

void LMqttsnClient::unsubscribe(const char* topicName)
{
	_subMgr.unsubscribe(topicName);
}

void LMqttsnClient::unsubscribe(const uint16_t topicId)
{
    _subMgr.unsubscribe(topicId);
}

void LMqttsnClient::disconnect(uint16_t sleepInSecs)
{
	_gwProxy.disconnect(sleepInSecs);
}

void LMqttsnClient::run()
{
	if (_isAutoConnect)
	{
		_gwProxy.connect();
	}
	_taskMgr.run();
}

void LMqttsnClient::setAutoConnectMode(uint8_t flg)
{
	_isAutoConnect = flg;
	_pubMgr.setAutoConnectMode(flg);
	_gwProxy.setAutoConnectMode(flg);
}

void LMqttsnClient::setSleepMode(uint32_t duration)
{
	// ToDo:  set WDT and sleep mode
	DISPLAY("\033[0m\033[0;32m\n\n Get into SLEEP mode %u [msec].\033[0m\033[0;37m\n\n", duration);
}

void LMqttsnClient::sleep(void)
{
	disconnect(_sleepDuration);
}

void LMqttsnClient::setSleepDuration(uint32_t duration)
{
	_sleepDuration = duration;
}

void LMqttsnClient::onConnect(void)
{
	if (_isAutoConnect)
	{
        _subMgr.onConnect();
	}
}

const char* LMqttsnClient::getClientId(void)
{
	return _gwProxy.getClientId();
}

uint16_t LMqttsnClient::getTopicId(const char* topicName)
{
    return _gwProxy.getTopicTable()->getTopicId(topicName);
}

