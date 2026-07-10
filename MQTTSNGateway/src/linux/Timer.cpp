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

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "Timer.h"
#include "MQTTSNGWDefines.h"

using namespace std;
using namespace MQTTSNGW;

/*=====================================
 Print Current Date & Time
 =====================================*/
char theCurrentTime[32];

const char* currentDateTime()
{
	struct timeval now;
	struct tm tstruct;
	gettimeofday(&now, 0);
	tstruct = *localtime(&now.tv_sec);
	strftime(theCurrentTime, sizeof(theCurrentTime), "%Y%m%d %H%M%S", &tstruct);
	sprintf(theCurrentTime + 15, ".%03d", (int)now.tv_usec / 1000 );
	return theCurrentTime;
}

/*============================================
 Timer
 ============================================*/
Timer::Timer(void)
{
	stop();
}

Timer::~Timer(void)
{

}

void Timer::start(uint32_t msecs)
{
	gettimeofday(&_startTime, 0);
	_millis = msecs;
}

bool Timer::isTimeup(void)
{
	return isTimeup(_millis);
}

bool Timer::isTimeup(uint32_t msecs)
{
	struct timeval curTime;
	long secs, usecs;
	if (_startTime.tv_sec == 0)
	{
		return false;
	}
	else
	{
		gettimeofday(&curTime, 0);
		secs = (curTime.tv_sec - _startTime.tv_sec) * 1000;
		usecs = (curTime.tv_usec - _startTime.tv_usec) / 1000.0;
		return ((secs + usecs) > (long) msecs);
	}
}

void Timer::stop()
{
	_startTime.tv_sec = 0;
	_millis = 0;
}

/*=====================================
Class LightIndicator
=====================================*/

LightIndicator::LightIndicator()
{
	_greenStatus = false;
	for ( int i = 0; i <= MAX_GPIO; i++)
	{
		_gpio[i] = 0;
	}
	init();
}

LightIndicator::~LightIndicator()
{
	for ( int i = 0; i <= MAX_GPIO; i++)
	{
		if ( _gpio[i] )
		{
			close( _gpio[i]);
		}
	}
}

void LightIndicator::greenLight(bool on)
{
	if (on)
	{
		if (!_greenStatus)
		{
			_greenStatus = true;
			//Turn Green on & turn Red off
			lit(LIGHT_INDICATOR_GREEN, "1");
			lit(LIGHT_INDICATOR_RED, "0");
		}
	}
	else
	{
		if (_greenStatus)
		{
			_greenStatus = false;
			//Turn Green off & turn Red on
			lit(LIGHT_INDICATOR_GREEN, "0");
			lit(LIGHT_INDICATOR_RED, "1");
		}
	}
}
void LightIndicator::blueLight(bool on)
{
	if (on)
	{
		lit(LIGHT_INDICATOR_BLUE, "1");
		if ( !_greenStatus )
		{
			greenLight(true);
		}
	}
	else
	{
		lit(LIGHT_INDICATOR_BLUE, "0");
	}
}

void LightIndicator::redLight(bool on)
{
	if (on)
	{
		lit(LIGHT_INDICATOR_RED, "1");
	}
	else
	{
		lit(LIGHT_INDICATOR_RED, "0");
	}
}

void LightIndicator::allLightOff(void)
{
	lit(LIGHT_INDICATOR_RED, "0");
	lit(LIGHT_INDICATOR_BLUE, "0");
	lit(LIGHT_INDICATOR_GREEN, "0");
	_greenStatus = false;
}

void LightIndicator::init()
{
	pinMode(LIGHT_INDICATOR_GREEN);
	pinMode(LIGHT_INDICATOR_RED);
	pinMode(LIGHT_INDICATOR_BLUE);
}

int LightIndicator::lit(int gpioNo, const char* onoff)
{
	int rc = 0;
	if( _gpio[gpioNo] )
	{
		rc = write(_gpio[gpioNo], onoff, 1);
	}
	return rc;
}

void LightIndicator::pinMode(int gpioNo)
{
	int rc = 0;
	int fd = rc; // eliminate unused warnning of compiler

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if ( fd < 0 )
	{
		return;
	}
	char no[4];

	sprintf(no,"%d", gpioNo);
	rc = write(fd, no, strlen(no));
	close(fd);

	char fileName[64];
	sprintf( fileName, "/sys/class/gpio/gpio%d/direction", gpioNo);

	fd = open(fileName, O_WRONLY);
	if ( fd < 0 )
	{
		return;
	}
	rc = write(fd,"out", 3);
	close(fd);
	sprintf( fileName, "/sys/class/gpio/gpio%d/value", gpioNo);
	fd = open(fileName, O_WRONLY);
	if ( fd > 0 )
	{
		_gpio[gpioNo] = fd;
	}
}


