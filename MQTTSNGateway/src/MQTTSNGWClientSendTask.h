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
#ifndef MQTTSNGWCLIENTSENDTASK_H_
#define MQTTSNGWCLIENTSENDTASK_H_

#include "MQTTSNGateway.h"
#include "SensorNetwork.h"

namespace MQTTSNGW
{
class AdapterManager;

/*=====================================
 Class ClientSendTask
 =====================================*/
class ClientSendTask: public Thread
{
MAGIC_WORD_FOR_THREAD;
    friend AdapterManager;
public:
    ClientSendTask(Gateway* gateway);
    ~ClientSendTask(void);
    void run(void);

private:
    void log(Client* client, MQTTSNPacket* packet);

    Gateway* _gateway;
    SensorNetwork* _sensorNetwork;
};

}

#endif /* MQTTSNGWCLIENTSENDTASK_H_ */
