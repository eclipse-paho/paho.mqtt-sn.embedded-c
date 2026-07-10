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
#ifndef MQTTGWCONNECTIONHANDLER_H_
#define MQTTGWCONNECTIONHANDLER_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWPacket.h"
#include "MQTTSNGateway.h"

namespace MQTTSNGW
{

class MQTTGWConnectionHandler
{
public:
    MQTTGWConnectionHandler(Gateway* gateway);
    ~MQTTGWConnectionHandler();
    void handleConnack(Client* client, MQTTGWPacket* packet);
    void handlePingresp(Client* client, MQTTGWPacket* packet);
    void handleDisconnect(Client* client, MQTTGWPacket* packet);
private:
    Gateway* _gateway;
};

}
#endif /* MQTTGWCONNECTIONHANDLER_H_ */
