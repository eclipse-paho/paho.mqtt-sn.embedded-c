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
#ifndef MQTTSNGWLOGMONITOR_H_
#define MQTTSNGWLOGMONITOR_H_

#include "MQTTSNGWProcess.h"

namespace MQTTSNGW
{
class Logmonitor: public Process
{
public:
    Logmonitor();
    ~Logmonitor();
    void run();
};

}

#endif /* MQTTSNGWLOGMONITOR_H_ */
