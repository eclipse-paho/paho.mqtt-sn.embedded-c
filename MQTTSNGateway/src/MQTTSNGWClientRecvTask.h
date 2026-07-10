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
#ifndef MQTTSNGWCLIENTRECVTASK_H_
#define MQTTSNGWCLIENTRECVTASK_H_

#include "SensorNetwork.h"
#include "MQTTSNGateway.h"

namespace MQTTSNGW
{
class AdapterManager;

/*=====================================
 Class ClientRecvTask
 =====================================*/
class ClientRecvTask: public Thread
{
MAGIC_WORD_FOR_THREAD;
    friend AdapterManager;
public:
    ClientRecvTask(Gateway*);
    ~ClientRecvTask(void);
    virtual void initialize(int argc, char** argv);
    void run(void);

private:
    void log(Client*, MQTTSNPacket*, MQTTSNString* id);
    void log(const char* clientId, MQTTSNPacket* packet);

    Gateway* _gateway;
    SensorNetwork* _sensorNetwork;
};

}

#endif /* MQTTSNGWCLIENTRECVTASK_H_ */
