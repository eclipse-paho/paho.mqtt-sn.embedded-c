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

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>

#include "LMqttsnClientApp.h"

namespace linuxAsyncClient {

/*============================================
                LTimer
 ============================================*/
class LTimer{
public:
    LTimer();
    ~LTimer();
    void start(uint32_t msec = 0);
    bool isTimeUp(uint32_t msec);
    bool isTimeUp(void);
    void stop(void);
    void changeUTC(void){};
    uint32_t getRemain(void);
    static void setUnixTime(uint32_t utc){};
private:
    struct timeval _startTime;
    uint32_t _millis;
};

} /* end of namespace */

#endif /* TIMER_H_ */
