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
#ifndef MQTTGWPUBLISHHANDLER_H_
#define MQTTGWPUBLISHHANDLER_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNGateway.h"

namespace MQTTSNGW
{

class MQTTGWPublishHandler
{
public:
    MQTTGWPublishHandler(Gateway* gateway);
    ~MQTTGWPublishHandler();
    void handlePublish(Client* client, MQTTGWPacket* packet);
    void handlePuback(Client* client, MQTTGWPacket* packet);
    void handleAck(Client* client, MQTTGWPacket* packet, int type);

    void handleAggregatePublish(Client* client, MQTTGWPacket* packet);
    void handleAggregatePuback(Client* client, MQTTGWPacket* packet);
    void handleAggregateAck(Client* client, MQTTGWPacket* packet, int type);
    void handleAggregatePubrel(Client* client, MQTTGWPacket* packet);

private:
    void replyACK(Client* client, Publish* pub, int type);

    Gateway* _gateway;
};

}

#endif /* MQTTGWPUBLISHHANDLER_H_ */
