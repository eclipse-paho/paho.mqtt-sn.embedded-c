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

#include <MQTTSNGWAdapterManager.h>
#include "MQTTSNGWBrokerSendTask.h"
#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"
#include "MQTTGWPacket.h"
#include <string.h>

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime();
#define ERRMSG_FORMAT "\n%s   \x1b[0m\x1b[31merror:\x1b[0m\x1b[37m Can't Xmit to the Broker. errno=%d\n"

/*=====================================
 Class BrokerSendTask
 =====================================*/
BrokerSendTask::BrokerSendTask(Gateway* gateway)
{
    _gateway = gateway;
    _gateway->attach((Thread*) this);
    _gwparams = nullptr;
    _light = nullptr;
    setTaskName("BrokerSendTask");
}

BrokerSendTask::~BrokerSendTask()
{
}

/**
 *  Initialize attributs of this class
 */
void BrokerSendTask::initialize(int argc, char** argv)
{
    _gwparams = _gateway->getGWParams();
    _light = _gateway->getLightIndicator();
}

/**
 *  connect to the broker and send MQTT messges
 */
void BrokerSendTask::run()
{
    Event* ev = nullptr;
    MQTTGWPacket* packet = nullptr;
    Client* client = nullptr;
    AdapterManager* adpMgr = _gateway->getAdapterManager();
    int rc = 0;

    while (true)
    {
        ev = _gateway->getBrokerSendQue()->wait();

        if (ev->getEventType() == EtStop)
        {
            WRITELOG("%s %s stopped.\n", currentDateTime(), getTaskName());
            delete ev;
            return;
        }

        if (ev->getEventType() == EtBrokerSend)
        {
            client = ev->getClient();
            packet = ev->getMQTTGWPacket();

            /* Check Client is managed by Adapters */
            client = adpMgr->getClient(client);

            if (packet->getType() == CONNECT && client->getNetwork()->isValid())
            {
                client->getNetwork()->close();
            }

            if (!client->getNetwork()->isValid())
            {
                /* connect to the broker and send a packet */

                if (client->isSecureNetwork())
                {
                    rc = client->getNetwork()->connect((const char*) _gwparams->brokerName, (const char*) _gwparams->portSecure,
                            (const char*) _gwparams->rootCApath, (const char*) _gwparams->rootCAfile,
                            (const char*) _gwparams->certKey, (const char*) _gwparams->privateKey);
                }
                else
                {
                    rc = client->getNetwork()->connect((const char*) _gwparams->brokerName, (const char*) _gwparams->port);
                }

                if (!rc)
                {
                    /* disconnect the broker and the client */
                    WRITELOG("%s BrokerSendTask: %s can't connect to the broker. errno=%d %s %s\n",
                    ERRMSG_HEADER, client->getClientId(), errno, strerror(errno), ERRMSG_FOOTER);
                    delete ev;
                    client->getNetwork()->close();
                    continue;
                }
            }

            /* send a packet */
            _light->blueLight(true);
            if ((rc = packet->send(client->getNetwork())) > 0)
            {
                if (packet->getType() == CONNECT)
                {
                    client->connectSended();
                }
                else if (packet->getType() == DISCONNECT)
                {
                    client->getNetwork()->close();
                    client->disconnected();
                }
                log(client, packet);
            }
            else
            {
                WRITELOG("%s BrokerSendTask: %s can't send a packet to the broker. errno=%d %s %s\n",
                ERRMSG_HEADER, client->getClientId(), rc == -1 ? errno : 0, strerror(errno), ERRMSG_FOOTER);
                if ( errno != EBADF)
                {
                    client->getNetwork()->close();
                }

                /* Disconnect the client */
                packet = new MQTTGWPacket();
                packet->setHeader(DISCONNECT);
                Event* ev1 = new Event();
                ev1->setBrokerRecvEvent(client, packet);
                _gateway->getPacketEventQue()->post(ev1);
            }

            _light->blueLight(false);
        }
        delete ev;
    }
}

/**
 *  write message content into stdout or Ringbuffer
 */
void BrokerSendTask::log(Client* client, MQTTGWPacket* packet)
{
    char pbuf[(SIZE_OF_LOG_PACKET + 5) * 3];
    char msgId[6];

    switch (packet->getType())
    {
    case CONNECT:
        WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(),
        RIGHTARROWB, client->getClientId(), packet->print(pbuf));
        break;
    case PUBLISH:
        WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROWB,
                client->getClientId(), packet->print(pbuf));
        break;
    case SUBSCRIBE:
    case UNSUBSCRIBE:
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP:
        WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), RIGHTARROWB,
                client->getClientId(), packet->print(pbuf));
        break;
    case PINGREQ:
        WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(),
        RIGHTARROWB, client->getClientId(), packet->print(pbuf));
        break;
    case DISCONNECT:
        WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(),
        RIGHTARROWB, client->getClientId(), packet->print(pbuf));
        break;
    default:
        break;
    }
}

