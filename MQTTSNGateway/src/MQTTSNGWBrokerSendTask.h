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
#ifndef MQTTSNGWBROKERSENDTASK_H_
#define MQTTSNGWBROKERSENDTASK_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWClient.h"

namespace MQTTSNGW
{
class Adapter;

/*=====================================
 Class BrokerSendTask
 =====================================*/
class BrokerSendTask: public Thread
{
MAGIC_WORD_FOR_THREAD;
    friend AdapterManager;
public:
    BrokerSendTask(Gateway* gateway);
    ~BrokerSendTask();
    void initialize(int argc, char** argv);
    void run();
private:
    void log(Client*, MQTTGWPacket*);
    Gateway* _gateway;
    GatewayParams* _gwparams;
    LightIndicator* _light;
};

}
#endif /* MQTTSNGWBROKERSENDTASK_H_ */
