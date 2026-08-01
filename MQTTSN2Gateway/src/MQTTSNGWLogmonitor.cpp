/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi, Ian Craggs
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
#include "MQTTSNGWProcess.h"
#include "MQTTSNGWLogmonitor.h"
#include <stdio.h>

using namespace std;
using namespace MQTTSNGW;

Logmonitor::Logmonitor()
{
    theProcess = this;
}

Logmonitor::~Logmonitor()
{

}

void Logmonitor::run()
{
    while (true)
    {
        const char* data = getLog();
        if (*data == 0)
        {
            break;
        }
        else
        {
            printf("%s", data);
        }
    }
}

