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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/
#ifndef MQTTSNGWCONNECTIONHANDLER_H_
#define MQTTSNGWCONNECTIONHANDLER_H_

#include "MQTTSNGateway.h"
#include "MQTTSNGWPacket.h"

namespace MQTTSNGW
{

class MQTTSNConnectionHandler
{
public:
    MQTTSNConnectionHandler(Gateway* gateway);
    ~MQTTSNConnectionHandler();
    void sendADVERTISE(void);
    void handleSearchgw(MQTTSNPacket* packet);
    void handleConnect(Client* client, MQTTSNPacket* packet);
    void handleWilltopic(Client* client, MQTTSNPacket* packet);
    void handleWillmsg(Client* client, MQTTSNPacket* packet);
    void handleDisconnect(Client* client, MQTTSNPacket* packet);
    void handleWilltopicupd(Client* client, MQTTSNPacket* packet);
    void handleWillmsgupd(Client* client, MQTTSNPacket* packet);
    void handlePingreq(Client* client, MQTTSNPacket* packet);
private:
    void sendStoredPublish(Client* client);

    Gateway* _gateway;
};

}

#endif /* MQTTSNGWCONNECTIONHANDLER_H_ */
