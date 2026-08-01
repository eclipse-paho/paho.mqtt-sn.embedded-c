/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi, Ian Craggs and others
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

#include "MQTTGWConnectionHandler.h"
#include "MQTTGWPacket.h"
#include <string.h>

using namespace std;
using namespace MQTTSNGW;

MQTTGWConnectionHandler::MQTTGWConnectionHandler(Gateway* gateway)
{
    _gateway = gateway;
}

MQTTGWConnectionHandler::~MQTTGWConnectionHandler()
{

}

void MQTTGWConnectionHandler::handleConnack(Client* client, MQTTGWPacket* packet)
{
    uint8_t rc = MQTT_SN_RC_SERVER_UNAVAILABLE;
    Connack resp;
    packet->getCONNACK(&resp);

    /* convert MQTT return code to MQTT-SN v2.0 reason code */
    if (resp.rc == MQTT_CONNECTION_ACCEPTED)
    {
        rc = MQTT_SN_RC_SUCCESS;
    }
    else if (resp.rc == MQTT_UNACCEPTABLE_PROTOCOL_VERSION)
    {
        rc = MQTT_SN_RC_UNSUPPORTED_PROTOCOL;
        WRITELOG(" ClientID : %s Requested Protocol version is not supported.\n", client->getClientId());
    }
    else if (resp.rc == MQTT_IDENTIFIER_REJECTED)
    {
        rc = MQTT_SN_RC_CLIENT_ID_INVALID;
        WRITELOG(" ClientID : %s ClientID is collect UTF-8 but not allowed by the Server.\n", client->getClientId());
    }
    else if (resp.rc == MQTT_SERVER_UNAVAILABLE)
    {
        rc = MQTT_SN_RC_SERVER_UNAVAILABLE;
        WRITELOG(" ClientID : %s The Network Connection has been made but the MQTT service is unavailable.\n",
                client->getClientId());
    }
    else if (resp.rc == MQTT_BAD_USERNAME_OR_PASSWORD)
    {
        rc = MQTT_SN_RC_BAD_USER_PASSWORD;
        WRITELOG(" Gateway Configuration Error: The data in the user name or password is malformed.\n");
    }
    else if (resp.rc == MQTT_NOT_AUTHORIZED)
    {
        rc = MQTT_SN_RC_NOT_AUTHORIZED;
        WRITELOG(" Gateway Configuration Error: The Client is not authorized to connect.\n");
    }

    MQTTSNPacket* snPacket = new MQTTSNPacket();
    MQTTSNPacket_connackData connackData;
    memset(&connackData, 0, sizeof(connackData));
    connackData.reasonCode = rc;
    snPacket->setCONNACK(&connackData);

    Event* ev1 = new Event();
    ev1->setClientSendEvent(client, snPacket);
    client->connackSended(rc);
    _gateway->getClientSendQue()->post(ev1);
}

void MQTTGWConnectionHandler::handlePingresp(Client* client, MQTTGWPacket* packet)
{
    MQTTSNPacket* snPacket = new MQTTSNPacket();
    snPacket->setPINGRESP(client->getPingReqPacketId(), 0);
    Event* ev1 = new Event();
    ev1->setClientSendEvent(client, snPacket);
    client->updateStatus(snPacket);
    _gateway->getClientSendQue()->post(ev1);
}

void MQTTGWConnectionHandler::handleDisconnect(Client* client, MQTTGWPacket* packet)
{
    MQTTSNPacket* snPacket = new MQTTSNPacket();
    snPacket->setDISCONNECT(MQTT_SN_RC_NORMAL_DISCONNECT);
    client->disconnected();
    client->getNetwork()->close();
    Event* ev1 = new Event();
    ev1->setClientSendEvent(client, snPacket);
}
