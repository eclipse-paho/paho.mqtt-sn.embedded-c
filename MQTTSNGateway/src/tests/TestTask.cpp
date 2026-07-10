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
 *    Tomoaki Yamaguchi - initial API and implementation 
 **************************************************************************************/
#include <unistd.h>
#include <cassert>
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
	uint16_t duration = 0;


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
		packet->getDISCONNECT(&duration);
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
