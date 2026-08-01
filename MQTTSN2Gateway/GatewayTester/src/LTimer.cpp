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

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "LMqttsnClientApp.h"
#include "LTimer.h"

//using namespace std;
using namespace linuxAsyncClient;

/*=====================================
        Class Timer
 =====================================*/

LTimer::LTimer(){
	_startTime.tv_sec = 0;
	_millis = 0;
}

LTimer::~LTimer(){

}

void LTimer::start(uint32_t msec){
  gettimeofday(&_startTime, 0);
  _millis = msec;
}

bool LTimer::isTimeUp(void){
  return isTimeUp(_millis);
}

bool LTimer::isTimeUp(uint32_t msec){
    struct timeval curTime;
    uint32_t secs, usecs;
    if (_startTime.tv_sec == 0){
        return false;
    }else{
        gettimeofday(&curTime, 0);
        secs  = (curTime.tv_sec  - _startTime.tv_sec) * 1000;
        usecs = (curTime.tv_usec - _startTime.tv_usec) / 1000.0;
        return ((secs + usecs) > (uint32_t)msec);
    }
}

void LTimer::stop(){
  _startTime.tv_sec = 0;
  _millis = 0;
}

uint32_t LTimer::getRemain(void)
{
    struct timeval curTime;
    uint32_t secs, usecs;
    if (_millis <= 0){
        return 0;
    }else{
        gettimeofday(&curTime, 0);
        secs  = (curTime.tv_sec  - _startTime.tv_sec) * 1000;
        usecs = (curTime.tv_usec - _startTime.tv_usec) / 1000.0;
        secs = _millis - (secs + usecs);
        return secs;
    }
}
