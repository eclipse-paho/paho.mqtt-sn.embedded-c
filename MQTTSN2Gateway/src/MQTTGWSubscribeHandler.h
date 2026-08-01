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
#ifndef MQTTGWSUBSCRIBEHANDLER_H_
#define MQTTGWSUBSCRIBEHANDLER_H_

#include "MQTTSNGWDefines.h"
#include "MQTTGWPacket.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"

namespace MQTTSNGW
{

class MQTTGWSubscribeHandler
{
public:
    MQTTGWSubscribeHandler(Gateway* gateway);
    ~MQTTGWSubscribeHandler();
    void handleSuback(Client* clnode, MQTTGWPacket* packet);
    void handleUnsuback(Client* clnode, MQTTGWPacket* packet);
    void handleAggregateSuback(Client* client, MQTTGWPacket* packet);
    void handleAggregateUnsuback(Client* client, MQTTGWPacket* packet);

private:
    Gateway* _gateway;
};

}

#endif /* MQTTGWSUBSCRIBEHANDLER_H_ */
