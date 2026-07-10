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

#include "MQTTSNGWClientRecvTask.h"
#include "MQTTSNPacket.h"
#include "MQTTSNGWQoSm1Proxy.h"
#include "MQTTSNGWEncapsulatedPacket.h"
#include <cstring>

//#include "MQTTSNGWForwarder.h"

using namespace MQTTSNGW;
char* currentDateTime(void);
/*=====================================
 Class ClientRecvTask
 =====================================*/
ClientRecvTask::ClientRecvTask(Gateway* gateway)
{
    _gateway = gateway;
    _gateway->attach((Thread*) this);
    _sensorNetwork = _gateway->getSensorNetwork();
    setTaskName("ClientRecvTask");
}

ClientRecvTask::~ClientRecvTask()
{
}

void ClientRecvTask::initialize(int argc, char** argv)
{
}

/*
 * Receive a packet from clients via sensor netwwork
 * and generate a event to execute the packet handling  procedure
 * of MQTTSNPacketHandlingTask.
 */
void ClientRecvTask::run()
{
    Event* ev = nullptr;
    AdapterManager* adpMgr = _gateway->getAdapterManager();
    QoSm1Proxy* qosm1Proxy = adpMgr->getQoSm1Proxy();
    int clientType = adpMgr->isAggregaterActive() ? AGGREGATER_TYPE : TRANSPEARENT_TYPE;
    ClientList* clientList = _gateway->getClientList();
    EventQue* packetEventQue = _gateway->getPacketEventQue();
    EventQue* clientsendQue = _gateway->getClientSendQue();

    char buf[128];

    while (true)
    {
        Client* client = nullptr;
        Forwarder* fwd = nullptr;
        WirelessNodeId nodeId;

        MQTTSNPacket* packet = new MQTTSNPacket();
        int packetLen = packet->recv(_sensorNetwork);

        if (CHK_SIGINT)
        {
            WRITELOG("%s %s stopped.\n", currentDateTime(), getTaskName());
            delete packet;
            return;
        }

        if (packetLen < 2)
        {
            delete packet;
            continue;
        }

        if (packet->getType() <= MQTTSN_ADVERTISE || packet->getType() == MQTTSN_GWINFO)
        {
            delete packet;
            continue;
        }

        if (packet->getType() == MQTTSN_SEARCHGW)
        {
            /* write log and post Event */
            log(0, packet, 0);
            ev = new Event();
            ev->setBrodcastEvent(packet);
            packetEventQue->post(ev);
            continue;
        }

        SensorNetAddress* senderAddr = _gateway->getSensorNetwork()->getSenderAddress();

        if (packet->getType() == MQTTSN_ENCAPSULATED)
        {
            fwd = _gateway->getAdapterManager()->getForwarderList()->getForwarder(senderAddr);

            if (fwd != nullptr)
            {
                MQTTSNString fwdName = MQTTSNString_initializer;
                fwdName.cstring = const_cast<char *>(fwd->getName());
                log(0, packet, &fwdName);

                /* get the packet from the encapsulation message */
                MQTTSNGWEncapsulatedPacket encap;
                encap.desirialize(packet->getPacketData(), packet->getPacketLength());
                nodeId.setId(encap.getWirelessNodeId());
                client = fwd->getClient(&nodeId);
                packet = encap.getMQTTSNPacket();
            }
        }
        else
        {
            /*   Check the client belonging to QoS-1Proxy  ?    */

            if (qosm1Proxy->isActive())
            {
                const char* clientName = qosm1Proxy->getClientId(senderAddr);

                if (clientName != nullptr)
                {
                    client = qosm1Proxy->getClient();

                    if (!packet->isQoSMinusPUBLISH())
                    {
                        log(clientName, packet);
                        WRITELOG("%s %s  %s can send only PUBLISH with QoS-1.%s\n",
                        ERRMSG_HEADER, clientName, senderAddr->sprint(buf), ERRMSG_FOOTER);
                        delete packet;
                        continue;
                    }
                }
            }

            if (client == nullptr)
            {
                client = _gateway->getClientList()->getClient(senderAddr);
            }
        }

        if (client != nullptr)
        {
            log(client, packet, 0);

            if (client->isDisconnect() && packet->getType() != MQTTSN_CONNECT)
            {
                WRITELOG("%s MQTTSNGWClientRecvTask %s is not connecting.%s\n",
                ERRMSG_HEADER, client->getClientId(), ERRMSG_FOOTER);

                /* send DISCONNECT to the client, if it is not connected */
                MQTTSNPacket* snPacket = new MQTTSNPacket();
                snPacket->setDISCONNECT(0);
                ev = new Event();
                ev->setClientSendEvent(client, snPacket);
                clientsendQue->post(ev);
                delete packet;
                continue;
            }
            else
            {
                ev = new Event();
                ev->setClientRecvEvent(client, packet);
                packetEventQue->post(ev);
            }
        }
        else
        {
            /* new client */
            if (packet->getType() == MQTTSN_CONNECT)
            {
                MQTTSNPacket_connectData data;
                memset(&data, 0, sizeof(MQTTSNPacket_connectData));
                if (!packet->getCONNECT(&data))
                {
                    log(0, packet, &data.clientID);
                    WRITELOG("%s CONNECT message form %s is incorrect.%s\n",
                    ERRMSG_HEADER, senderAddr->sprint(buf),
                    ERRMSG_FOOTER);
                    delete packet;
                    continue;
                }

                client = clientList->getClient(&data.clientID);

                if (fwd != nullptr)
                {
                    if (client == nullptr)
                    {
                        /* create a new client */
                        client = clientList->createClient(0, &data.clientID, clientType);
                    }
                    /* Add to a forwarded client list of forwarder. */
                    fwd->addClient(client, &nodeId);
                }
                else
                {
                    if (client)
                    {
                        /* Authentication is not required */
                        if (_gateway->getGWParams()->clientAuthentication == false)
                        {
                            client->setClientAddress(senderAddr);
                        }
                    }
                    else
                    {
                        /* create a new client */
                        client = clientList->createClient(senderAddr, &data.clientID, clientType);
                    }
                }

                log(client, packet, &data.clientID);

                if (client == nullptr)
                {
                    WRITELOG("%s Client(%s) was rejected. CONNECT message has been discarded.%s\n",
                    ERRMSG_HEADER, senderAddr->sprint(buf),
                    ERRMSG_FOOTER);
                    delete packet;
                    continue;
                }

                /* post Client RecvEvent */
                ev = new Event();
                ev->setClientRecvEvent(client, packet);
                packetEventQue->post(ev);
            }
            else
            {
                log(client, packet, 0);
                if (packet->getType() == MQTTSN_ENCAPSULATED)
                {
                    WRITELOG(
                            "%s MQTTSNGWClientRecvTask  Forwarder(%s) is not declared by ClientList file. message has been discarded.%s\n",
                            ERRMSG_HEADER, _sensorNetwork->getSenderAddress()->sprint(buf),
                            ERRMSG_FOOTER);
                }
                else
                {
                    WRITELOG("%s MQTTSNGWClientRecvTask  Client(%s) is not connecting. message has been discarded.%s\n",
                    ERRMSG_HEADER, senderAddr->sprint(buf),
                    ERRMSG_FOOTER);
                }
                delete packet;
            }
        }
    }
}

void ClientRecvTask::log(Client* client, MQTTSNPacket* packet, MQTTSNString* id)
{
    const char* clientId;
    char cstr[MAX_CLIENTID_LENGTH + 1];

    if (id)
    {
        if (id->cstring)
        {
            strncpy(cstr, id->cstring, strlen(id->cstring));
            clientId = cstr;
        }
        else
        {
            memset((void*) cstr, 0, id->lenstring.len + 1);
            strncpy(cstr, id->lenstring.data, id->lenstring.len);
            clientId = cstr;
        }
    }
    else if (client)
    {
        clientId = client->getClientId();
    }
    else
    {
        clientId = UNKNOWNCL;
    }

    log(clientId, packet);
}

void ClientRecvTask::log(const char* clientId, MQTTSNPacket* packet)
{
    char pbuf[ SIZE_OF_LOG_PACKET * 3 + 1];
    char msgId[6];

    switch (packet->getType())
    {
    case MQTTSN_SEARCHGW:
        WRITELOG(FORMAT_Y_G_G_NL, currentDateTime(), packet->getName(),
        LEFTARROW, CLIENT, packet->print(pbuf));
        break;
    case MQTTSN_CONNECT:
    case MQTTSN_PINGREQ:
        WRITELOG(FORMAT_Y_G_G_NL, currentDateTime(), packet->getName(),
        LEFTARROW, clientId, packet->print(pbuf));
        break;
    case MQTTSN_DISCONNECT:
    case MQTTSN_WILLTOPICUPD:
    case MQTTSN_WILLMSGUPD:
    case MQTTSN_WILLTOPIC:
    case MQTTSN_WILLMSG:
        WRITELOG(FORMAT_Y_G_G, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
        break;
    case MQTTSN_PUBLISH:
    case MQTTSN_REGISTER:
    case MQTTSN_SUBSCRIBE:
    case MQTTSN_UNSUBSCRIBE:
        WRITELOG(FORMAT_G_MSGID_G_G_NL, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId,
                packet->print(pbuf));
        break;
    case MQTTSN_REGACK:
    case MQTTSN_PUBACK:
    case MQTTSN_PUBREC:
    case MQTTSN_PUBREL:
    case MQTTSN_PUBCOMP:
        WRITELOG(FORMAT_G_MSGID_G_G, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId,
                packet->print(pbuf));
        break;
    case MQTTSN_ENCAPSULATED:
        WRITELOG(FORMAT_Y_G_G, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
        break;
    default:
        WRITELOG(FORMAT_W_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
        break;
    }
}
