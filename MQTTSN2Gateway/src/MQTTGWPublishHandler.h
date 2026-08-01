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
