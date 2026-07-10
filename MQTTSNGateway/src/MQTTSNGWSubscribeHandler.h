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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/
#ifndef MQTTSNGWSUBSCRIBEHANDLER_H_
#define MQTTSNGWSUBSCRIBEHANDLER_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNGWClient.h"

namespace MQTTSNGW
{
/*=====================================
 Class MQTTSNSubscribeHandler
 =====================================*/
class MQTTSNSubscribeHandler
{
public:
    MQTTSNSubscribeHandler(Gateway* gateway);
    ~MQTTSNSubscribeHandler();
    MQTTGWPacket* handleSubscribe(Client* client, MQTTSNPacket* packet);
    MQTTGWPacket* handleUnsubscribe(Client* client, MQTTSNPacket* packet);
    void handleAggregateSubscribe(Client* client, MQTTSNPacket* packet);
    void handleAggregateUnsubscribe(Client* client, MQTTSNPacket* packet);

private:
    Gateway* _gateway;
};

}

#endif /* MQTTSNGWSUBSCRIBEHANDLER_H_ */
