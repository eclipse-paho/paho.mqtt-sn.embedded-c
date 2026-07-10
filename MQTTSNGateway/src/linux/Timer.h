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
 *    Tomoaki Yamaguchi - initial API and implementation 
 **************************************************************************************/
#ifndef MQTTSNGATEWAY_SRC_LINUX_TIMER_H_
#define MQTTSNGATEWAY_SRC_LINUX_TIMER_H_

#include <stdint.h>
#include <sys/time.h>
#include "MQTTSNGWDefines.h"

namespace MQTTSNGW
{
/*==========================================================
 *           Light Indicators
 ===========================================================*/
#define MAX_GPIO                27    // GPIO02 - GPIO27
#define LIGHT_INDICATOR_GREEN   23    // RPi connector 16
#define LIGHT_INDICATOR_RED     24    // RPi connector 18
#define LIGHT_INDICATOR_BLUE    25    // RPi connector 22

/*============================================
 Timer
 ============================================*/
class Timer
{
public:
	Timer(void);
	~Timer(void);
	void start(uint32_t msecs = 0);
	bool isTimeup(void);
	bool isTimeup(uint32_t msecs);
	void stop();

private:
	struct timeval _startTime;
	uint32_t _millis;
};

/*=====================================
 Class LightIndicator
 =====================================*/
class LightIndicator
{
public:
	LightIndicator();
	~LightIndicator();
	void greenLight(bool on);
	void blueLight(bool on);
	void redLight(bool on);
	void allLightOff(void);

private:
	void init();
	int  lit(int gpioNo, const char* onoff);
	void pinMode(int gpioNo);
	bool _greenStatus;
	int _gpio[MAX_GPIO + 1];
};

}

#endif /* MQTTSNGATEWAY_SRC_LINUX_TIMER_H_ */
