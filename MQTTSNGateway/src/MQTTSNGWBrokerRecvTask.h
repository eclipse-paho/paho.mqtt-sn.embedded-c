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
#ifndef MQTTSNGWBROKERRECVTASK_H_
#define MQTTSNGWBROKERRECVTASK_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGateway.h"

namespace MQTTSNGW
{

/*=====================================
 Class BrokerRecvTask
 =====================================*/
class BrokerRecvTask: public Thread
{
MAGIC_WORD_FOR_THREAD;

public:
    BrokerRecvTask(Gateway* gateway);
    ~BrokerRecvTask();
    void initialize(int argc, char** argv);
    void run(void);

private:
    int log(Client*, MQTTGWPacket*);

    Gateway* _gateway;
    LightIndicator* _light;
};

}

#endif /* MQTTSNGWBROKERRECVTASK_H_ */
