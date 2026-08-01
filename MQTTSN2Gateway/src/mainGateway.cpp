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
