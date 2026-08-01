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
 *    Tomoaki Yamaguchi - initial API and implementation 
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#include <string.h>
#include <cassert>
#include "TestProcess.h"
#include "TestTopics.h"
#include "TestQue.h"
#include "TestTree23.h"
#include "TestTopicIdMap.h"
#include "MQTTSNGWProcess.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWPacket.h"
#include "Timer.h"

using namespace std;
using namespace MQTTSNGW;

#define CONFDIR "./"
#define CONF    "gateway.conf"

const char* currentDateTime(void);

TestProcess::TestProcess()
{
	theMultiTaskProcess = this;
	theProcess = this;
}

TestProcess::~TestProcess()
{

}

void TestProcess::initialize(int argc, char** argv)
{
	MultiTaskProcess::initialize(argc, argv);
	assert(0 == strcmp(CONFDIR, getConfigDirName()->c_str()));
	assert(0 == strcmp(CONF, getConfigFileName()->c_str()));
	resetRingBuffer();
}

void TestProcess::run(void)
{
	char value[256];
	int i = 0;
	Timer tm;

	/* Test command line parameter */

	assert(1 == getArgc() || 3 == getArgc() );
	getParam("BrokerName", value);
	assert(0 == strcmp("mqtt.eclipseprojects.io", value));

	/* Test RingBuffer */
	for ( i = 0; i < 1000; i++)
	{
		putLog("Test RingBuffer %d ", 1234567890);
	}
	putLog("\n\nRingBuffer Test complieted.\n\n");

	/* Test Timer */
	printf("Timer Test start\n");
	printf("%s Timer start\n",  currentDateTime());
	tm.start(1000);
	while (!tm.isTimeup());
	printf("%s Timer 1sec\n", currentDateTime());

	tm.start();
	while (!tm.isTimeup(1000));
	printf("%s Timer 1sec\n", currentDateTime());
	printf("Timer Test completed\n\n");

	/* Test Que */
    printf("Test  Que            ");
	TestQue* tque = new TestQue();
	tque->test();
	delete tque;

	/* Test Tree23 */
    //printf("Test  Tree23         ");
	//TestTree23* tree23 = new TestTree23();
	//tree23->test();
	//delete tree23;

	/* Test TopicTable */
    printf("Test  Topic          ");
	TestTopics* testTopic = new TestTopics();
	testTopic->test();
	delete testTopic;

	/* Test TopicIdMap */
    printf("Test  TopicIdMap     ");
	TestTopicIdMap* testMap = new TestTopicIdMap();
	testMap->test();
	delete testMap;

	/* Test EventQue */
	/*
	printf("Test  EventQue       ");
	Client* client = new Client();
	_evQue.setMaxSize(EVENT_CNT);
	for ( int i = 0; i < EVENT_CNT + 4; i++ )
	{
		Event* ev = new Event();
		MQTTSNPacket* packet = new MQTTSNPacket();
		packet->setDISCONNECT(i);
		ev->setClientSendEvent(client, packet);
		_evQue.post(ev);
	}
	delete client;
	*/
	//MultiTaskProcess::run();
}
