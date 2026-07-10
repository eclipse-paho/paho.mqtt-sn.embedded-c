/**************************************************************************************
 * Copyright (c) 2018, Tomoaki Yamaguchi
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

#include "MQTTSNGWQoSm1Proxy.h"
#include "MQTTSNGateway.h"
#include "SensorNetwork.h"
#include "MQTTSNGWClientList.h"
#include <string>
#include <string.h>

using namespace MQTTSNGW;

/*=====================================
 Class QoSm1Proxy
 =====================================*/
QoSm1Proxy::QoSm1Proxy(Gateway* gw) :
        Adapter(gw)
{
    _gateway = gw;
}

QoSm1Proxy::~QoSm1Proxy(void)
{

}

void QoSm1Proxy::initialize(char* gwName)
{
    if (_gateway->hasSecureConnection())
    {
        _isSecure = true;
    }

    /*  Create QoS-1 Clients from clients.conf */
    _gateway->getClientList()->setClientList(QOSM1PROXY_TYPE);

    /* Create a client for  QoS-1 proxy */
    string name = string(gwName) + string("_QoS-1");
    setup(name.c_str(), Atype_QoSm1Proxy);
    _isActive = true;
}

bool QoSm1Proxy::isActive(void)
{
    return _isActive;
}

