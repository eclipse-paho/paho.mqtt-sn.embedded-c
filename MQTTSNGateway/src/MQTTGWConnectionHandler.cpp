/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi and others
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

#include "MQTTGWConnectionHandler.h"
#include "MQTTGWPacket.h"

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
    uint8_t rc = MQTT_SERVER_UNAVAILABLE;
    Connack resp;
    packet->getCONNACK(&resp);

    /* convert MQTT ReturnCode to MQTT-SN one */
    if (resp.rc == MQTT_CONNECTION_ACCEPTED)
    {
        rc = MQTTSN_RC_ACCEPTED;
    }
    else if (resp.rc == MQTT_UNACCEPTABLE_PROTOCOL_VERSION)
    {
        rc = MQTTSN_RC_NOT_SUPPORTED;
        WRITELOG(" ClientID : %s Requested Protocol version is not supported.\n", client->getClientId());
    }
    else if (resp.rc == MQTT_IDENTIFIER_REJECTED)
    {
        rc = MQTTSN_RC_NOT_SUPPORTED;
        WRITELOG(" ClientID : %s ClientID is collect UTF-8 but not allowed by the Server.\n", client->getClientId());
    }
    else if (resp.rc == MQTT_SERVER_UNAVAILABLE)
    {
        rc = MQTTSN_RC_REJECTED_CONGESTED;
        WRITELOG(" ClientID : %s The Network Connection has been made but the MQTT service is unavailable.\n",
                client->getClientId());
    }
    else if (resp.rc == MQTT_BAD_USERNAME_OR_PASSWORD)
    {
        rc = MQTTSN_RC_NOT_SUPPORTED;
        WRITELOG(" Gateway Configuration Error: The data in the user name or password is malformed.\n");
    }
    else if (resp.rc == MQTT_NOT_AUTHORIZED)
    {
        rc = MQTTSN_RC_NOT_SUPPORTED;
        WRITELOG(" Gateway Configuration Error: The Client is not authorized to connect.\n");
    }

    MQTTSNPacket* snPacket = new MQTTSNPacket();
    snPacket->setCONNACK(rc);

    Event* ev1 = new Event();
    ev1->setClientSendEvent(client, snPacket);
    client->connackSended(rc);  // update the client's status
    _gateway->getClientSendQue()->post(ev1);
}

void MQTTGWConnectionHandler::handlePingresp(Client* client, MQTTGWPacket* packet)
{
    MQTTSNPacket* snPacket = new MQTTSNPacket();
    snPacket->setPINGRESP();
    Event* ev1 = new Event();
    ev1->setClientSendEvent(client, snPacket);
    client->updateStatus(snPacket);
    _gateway->getClientSendQue()->post(ev1);
}

void MQTTGWConnectionHandler::handleDisconnect(Client* client, MQTTGWPacket* packet)
{
    MQTTSNPacket* snPacket = new MQTTSNPacket();
    snPacket->setDISCONNECT(0);
    client->disconnected();
    client->getNetwork()->close();
    Event* ev1 = new Event();
    ev1->setClientSendEvent(client, snPacket);
}
