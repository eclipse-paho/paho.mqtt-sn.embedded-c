/**************************************************************************************
 * Copyright (c) 2018, Tomoaki Yamaguchi
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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNAGGREGATECONNECTIONHANDLER_H_
#define MQTTSNGATEWAY_SRC_MQTTSNAGGREGATECONNECTIONHANDLER_H_

#include "MQTTSNGWDefines.h"

namespace MQTTSNGW
{
class Gateway;
class Client;
class MQTTSNPacket;

class MQTTSNAggregateConnectionHandler
{
public:
    MQTTSNAggregateConnectionHandler(Gateway* gateway);
    ~MQTTSNAggregateConnectionHandler(void);

    void handleConnect(Client* client, MQTTSNPacket* packet);
    void handleWillmsg(Client* client, MQTTSNPacket* packet);
    void handleDisconnect(Client* client, MQTTSNPacket* packet);
    void handlePingreq(Client* client, MQTTSNPacket* packet);

private:
    void sendStoredPublish(Client* client);

    Gateway* _gateway;
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNAGGREGATECONNECTIONHANDLER_H_ */
