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
#include <unistd.h>
#include <cassert>
#include <cstring>
#include "TestTask.h"
#include "Threading.h"
#include "TestProcess.h"
using namespace std;
using namespace MQTTSNGW;

TestTask::TestTask(TestProcess* proc)
{
	proc->attach((Thread*)this);
	_proc = proc;
}

TestTask::~TestTask()
{

}

void TestTask::initialize(int argc, char** argv)
{
	printf("Task initialize complite.\n");
}

void TestTask::run(void)
{
	int evcnt = 0;
	EventQue* evQue = _proc->getEventQue();
	MQTTSNPacket_disconnectData disconnectData;
	memset(&disconnectData, 0, sizeof(disconnectData));


	while (true)
	{
		Event* ev = evQue->timedwait(5000);
		evcnt++;
		if ( ev->getEventType() == EtTimeout )
		{
			assert(EVENT_CNT + 1 == evcnt);
			delete ev;
			printf("[ OK ]\n");
			break;
		}
		MQTTSNPacket* packet = ev->getMQTTSNPacket();
		packet->getDISCONNECT(&disconnectData);
		delete ev;
	}

	while(true)
	{
		if ( CHK_SIGINT)
		{
			printf("\nTest  Task           [ OK ]\n");
			return;
		}
		printf("Enter CTRL+C\n");
		sleep(1);
	}
}
