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
 *    Tomoaki Yamaguchi - initial API and implementation 
 **************************************************************************************/
#ifndef MQTTSNGATEWAY_SRC_TESTS_TESTTOPICIDMAP_H_
#define MQTTSNGATEWAY_SRC_TESTS_TESTTOPICIDMAP_H_

#include "MQTTSNGWClient.h"

class TestTopicIdMap
{
public:
	TestTopicIdMap();
	~TestTopicIdMap();
	void test(void);
	bool testGetElement(uint16_t msgid, uint16_t id, MQTTSN_topicid* topic);

private:
	TopicIdMap* _map;
};

#endif /* MQTTSNGATEWAY_SRC_TESTS_TESTTOPICIDMAP_H_ */
