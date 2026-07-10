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
#ifndef TESTPROCESS_H_
#define TESTPROCESS_H_

#include "MQTTSNGWProcess.h"
#include "MQTTSNGateway.h"
#define EVENT_CNT 10
namespace MQTTSNGW
{
class TestProcess: public MultiTaskProcess{
public:
	TestProcess();
	~TestProcess();
	virtual void initialize(int argc, char** argv);
	void run(void);
	EventQue* getEventQue(void) { return &_evQue; }

private:
	EventQue _evQue;
};

}

#endif /* TESTPROCESS_H_ */
