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

#include <stdio.h>
#include <unistd.h>

#include "LMqttsnClientApp.h"
using namespace std;
/*=====================================
        Global functions
 ======================================*/
#ifndef CPU_BIGENDIANN

/*--- For Little endianness ---*/

uint16_t getUint16(const uint8_t* pos){
	uint16_t val = ((uint16_t)*pos++ << 8);
	return val += *pos;
}

void setUint16(uint8_t* pos, uint16_t val){
    *pos++ = (val >> 8) & 0xff;
	*pos   = val & 0xff;
}

uint32_t getUint32(const uint8_t* pos){
    uint32_t val = uint32_t(*pos++) <<  24;
	val += uint32_t(*pos++) << 16;
	val += uint32_t(*pos++) <<  8;
	return val += *pos++;
}

void setUint32(uint8_t* pos, uint32_t val){
	*pos++ = (val >> 24) & 0xff;
	*pos++ = (val >> 16) & 0xff;
	*pos++ = (val >>  8) & 0xff;
	*pos   =  val & 0xff;
}

float getFloat32(const uint8_t* pos){
	union{
		float flt;
		uint8_t d[4];
	}val;
    val.d[3] = *pos++;
	val.d[2] = *pos++;
	val.d[1] = *pos++;
	val.d[0] = *pos;
	return val.flt;
}

void setFloat32(uint8_t* pos, float flt){
	union{
		float flt;
		uint8_t d[4];
	}val;
	val.flt = flt;
    *pos++ = val.d[3];
    *pos++ = val.d[2];
    *pos++ = val.d[1];
    *pos   = val.d[0];
}

#else

/*--- For Big endianness ---*/

uint16_t getUint16(const uint8_t* pos){
  uint16_t val = *pos++;
  return val += ((uint16_t)*pos++ << 8);
}

void setUint16(uint8_t* pos, uint16_t val){
	*pos++ =  val & 0xff;
	*pos   = (val >>  8) & 0xff;
}

uint32_t getUint32(const uint8_t* pos){
    long val = uint32_t(*(pos + 3)) << 24;
    val += uint32_t(*(pos + 2)) << 16;
	val += uint32_t(*(pos + 1)) <<  8;
	return val += *pos;
}

void setUint32(uint8_t* pos, uint32_t val){
    *pos++ =  val & 0xff;
    *pos++ = (val >>  8) & 0xff;
    *pos++ = (val >> 16) & 0xff;
    *pos   = (val >> 24) & 0xff;
}

float getFloat32(const uint8_t* pos){
	union{
		float flt;
		uint8_t d[4];
	}val;

    val.d[0] = *pos++;
	val.d[1] = *pos++;
	val.d[2] = *pos++;
	val.d[3] = *pos;
	return val.flt;
}

void setFloat32(uint8_t* pos, float flt){
	union{
		float flt;
		uint8_t d[4];
	}val;
	val.flt = flt;
    *pos++ = val.d[0];
    *pos++ = val.d[1];
    *pos++ = val.d[2];
    *pos   = val.d[3];
}

#endif  // CPU_LITTLEENDIANN




