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

#include "MQTTSNGWBrokerRecvTask.h"
#include "MQTTSNGWClient.h"
#include "MQTTSNGWClientList.h"
#include "MQTTSNGateway.h"
#include <unistd.h>

using namespace std;
using namespace MQTTSNGW;

char* currentDateTime(void);

/*=====================================
 Class BrokerRecvTask
 =====================================*/
BrokerRecvTask::BrokerRecvTask(Gateway* gateway)
{
    _gateway = gateway;
    _gateway->attach((Thread*) this);
    _light = nullptr;
    setTaskName("BrokerRecvTask");
}

BrokerRecvTask::~BrokerRecvTask()
{
}

/**
 *  Initialize attributs of this class
 */
void BrokerRecvTask::initialize(int argc, char** argv)
{
    _light = _gateway->getLightIndicator();
}

/**
 *  receive a MQTT messge from the broker and post a event.
 */
void BrokerRecvTask::run(void)
{
    struct timeval timeout;
    MQTTGWPacket* packet = nullptr;
    int rc;
    Event* ev = nullptr;
    fd_set rset;
    fd_set wset;

    while (true)
    {
        _light->blueLight(false);
        if (CHK_SIGINT)
        {
            WRITELOG("%s %s stopped.\n", currentDateTime(), getTaskName());
            return;
        }
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;    // 500 msec
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        int maxSock = 0;
        int sockfd = 0;

        /* Prepare sockets list to read */
        Client* client = _gateway->getClientList()->getClient(0);

        while (client)
        {
            if (client->getNetwork()->isValid())
            {
                sockfd = client->getNetwork()->getSock();
                FD_SET(sockfd, &rset);
                FD_SET(sockfd, &wset);
                if (sockfd > maxSock)
                {
                    maxSock = sockfd;
                }
            }
            client = client->getNextClient();
        }

        if (maxSock == 0)
        {
            usleep(500 * 1000);
        }
        else
        {
            /* Check sockets is ready to read */
            int activity = select(maxSock + 1, &rset, 0, 0, &timeout);

            if (activity > 0)
            {
                client = _gateway->getClientList()->getClient(0);

                while (client)
                {
                    _light->blueLight(false);
                    if (client->getNetwork()->isValid())
                    {
                        int sockfd = client->getNetwork()->getSock();
                        if (FD_ISSET(sockfd, &rset))
                        {
                            packet = new MQTTGWPacket();
                            rc = 0;
                            /* read sockets */
                            _light->blueLight(true);
                            rc = packet->recv(client->getNetwork());
                            if (rc > 0)
                            {
                                if (log(client, packet) == -1)
                                {
                                    delete packet;
                                    goto nextClient;
                                }

                                /* post a BrokerRecvEvent */
                                ev = new Event();
                                ev->setBrokerRecvEvent(client, packet);
                                _gateway->getPacketEventQue()->post(ev);
                            }
                            else
                            {
                                if (rc == 0)  // Disconnected
                                {
                                    WRITELOG("%s BrokerRecvTask %s is disconnected by the broker.%s\n",
                                    ERRMSG_HEADER, client->getClientId(),
                                    ERRMSG_FOOTER);
                                    client->getNetwork()->close();
                                    client->disconnected();
                                }
                                else if (rc == -1)
                                {
                                    WRITELOG("%s BrokerRecvTask can't receive a packet from the broker errno=%d %s%s\n",
                                    ERRMSG_HEADER, errno, client->getClientId(),
                                    ERRMSG_FOOTER);
                                }
                                else if (rc == -2)
                                {
                                    WRITELOG(
                                            "%s BrokerRecvTask receive invalid length of packet from the broker.  DISCONNECT  %s %s\n",
                                            ERRMSG_HEADER, client->getClientId(),
                                            ERRMSG_FOOTER);
                                }
                                else if (rc == -3)
                                {
                                    WRITELOG("%s BrokerRecvTask can't allocate memories for the packet %s%s\n",
                                    ERRMSG_HEADER, client->getClientId(),
                                    ERRMSG_FOOTER);
                                }

                                delete packet;

                                if ((rc == -1 || rc == -2) && (client->isActive() || client->isSleep() || client->isAwake()))
                                {
                                    client->getNetwork()->close();
                                    client->disconnected();
                                }
                            }
                        }
                    }
                    nextClient: client = client->getNextClient();
                }
            }
        }
    }
}

/**
 *  write message content into stdout or Ringbuffer
 */
int BrokerRecvTask::log(Client* client, MQTTGWPacket* packet)
{
    char pbuf[(SIZE_OF_LOG_PACKET + 5) * 3];
    char msgId[6];
    int rc = 0;

    switch (packet->getType())
    {
    case CONNACK:
        WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(), LEFTARROWB, client->getClientId(), packet->print(pbuf));
        break;
    case PUBLISH:
        WRITELOG(FORMAT_W_MSGID_Y_W_NL, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROWB,
                client->getClientId(), packet->print(pbuf));
        break;
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP:
        WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROWB,
                client->getClientId(), packet->print(pbuf));
        break;
    case SUBACK:
    case UNSUBACK:
        WRITELOG(FORMAT_W_MSGID_Y_W, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROWB,
                client->getClientId(), packet->print(pbuf));
        break;
    case PINGRESP:
        WRITELOG(FORMAT_Y_Y_W, currentDateTime(), packet->getName(), LEFTARROWB, client->getClientId(), packet->print(pbuf));
        break;
    default:
        WRITELOG("Type=%x\n", packet->getType());
        rc = -1;
        break;
    }
    return rc;
}
