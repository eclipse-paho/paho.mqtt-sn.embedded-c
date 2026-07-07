/**************************************************************************************
 * Copyright (c) 2018, 2026 Tomoaki Yamaguchi
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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWQOSM1PROXY_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWQOSM1PROXY_H_

#include "MQTTSNGWAdapter.h"
namespace MQTTSNGW
{
class Gateway;
class Adapter;
class Client;
class SensorNetAddress;
class MQTTSNPacket;

/*=====================================
 Class QoSm1Proxy
 =====================================*/
class QoSm1Proxy: public Adapter
{
public:
    QoSm1Proxy(Gateway* gw);
    ~QoSm1Proxy(void);

    void initialize(char* GWnAME);
    bool isActive(void);

private:
    Gateway* _gateway;

    bool _isActive { false };
    bool _isSecure { false };
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWQOSM1PROXY_H_ */
