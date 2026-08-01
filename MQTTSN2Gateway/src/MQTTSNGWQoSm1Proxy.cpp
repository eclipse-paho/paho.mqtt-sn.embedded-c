/**************************************************************************************
 * Copyright (c) 2018, 2026 Tomoaki Yamaguchi, Ian Craggs
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

