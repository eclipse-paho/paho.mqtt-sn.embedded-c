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
#ifndef MQTTSNGATEWAY_SRC_TESTS_TESTTOPICS_H_
#define MQTTSNGATEWAY_SRC_TESTS_TESTTOPICS_H_

#include "MQTTSNGWClient.h"

class TestTopics
{
public:
	TestTopics();
	~TestTopics();
	void test(void);

private:
	Topics* _topics;
};

#endif /* MQTTSNGATEWAY_SRC_TESTS_TESTTOPICS_H_ */
