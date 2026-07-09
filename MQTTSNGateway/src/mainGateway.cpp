/**************************************************************************************
 * Copyright (c) 2016, 2020 Tomoaki Yamaguchi and others
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/
#include "MQTTSNGateway.h"
#include "MQTTSNGWBrokerRecvTask.h"
#include "MQTTSNGWBrokerSendTask.h"
#include "MQTTSNGWClientRecvTask.h"
#include "MQTTSNGWClientSendTask.h"
#include "MQTTSNGWPacketHandleTask.h"

using namespace MQTTSNGW;
/*
 *  Gateway Application
 */
Gateway gateway;
PacketHandleTask task1(&gateway);
ClientRecvTask task2(&gateway);
ClientSendTask task3(&gateway);
BrokerRecvTask task4(&gateway);
BrokerSendTask task5(&gateway);

int main(int argc, char** argv)
{
    // Prevent buffering of log messages
    setbuf(stdout, NULL);

    try
    {
        gateway.initialize(argc, argv);
        gateway.run();
    }
    catch (Exception &ex)
    {
        ex.writeMessage();
        WRITELOG("ABORT Gateway!!!\n\n\n");
    }
}
