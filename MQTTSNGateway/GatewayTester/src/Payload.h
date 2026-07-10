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

#ifndef PAYLOAD_H_
#define PAYLOAD_H_

#include "LMqttsnClientApp.h"

#define MSGPACK_FALSE    0xc2
#define MSGPACK_TRUE     0xc3
#define MSGPACK_POSINT   0x80
#define MSGPACK_NEGINT   0xe0
#define MSGPACK_UINT8    0xcc
#define MSGPACK_UINT16   0xcd
#define MSGPACK_UINT32   0xce
#define MSGPACK_INT8     0xd0
#define MSGPACK_INT16    0xd1
#define MSGPACK_INT32    0xd2
#define MSGPACK_FLOAT32  0xca
#define MSGPACK_FIXSTR   0xa0
#define MSGPACK_STR8     0xd9
#define MSGPACK_STR16    0xda
#define MSGPACK_ARRAY15  0x90
#define MSGPACK_ARRAY16  0xdc
#define MSGPACK_MAX_ELEMENTS   50   // Less than 256

namespace linuxAsyncClient {
/*=====================================
        Class Payload
  =====================================*/
class Payload{
public:
	Payload();
	Payload(uint16_t len);
	~Payload();

/*---------------------------------------------
  getLen() and getRowData() are
  minimum required functions of Payload class.
----------------------------------------------*/
	uint16_t getLen();       // get data length
	uint8_t* getRowData();   // get data pointer

/*--- Functions for MessagePack ---*/
	void init(void);
	int8_t set_bool(bool val);
	int8_t set_uint32(uint32_t val);
	int8_t set_int32(int32_t val);
	int8_t set_float(float val);
	int8_t set_str(char* val);
	int8_t set_str(const char* val);
	int8_t set_array(uint8_t val);

	bool    get_bool(uint8_t index);
	uint8_t getArray(uint8_t index);
	uint32_t get_uint32(uint8_t index);
	int32_t  get_int32(uint8_t index);
    float    get_float(uint8_t index);
    const char* get_str(uint8_t index, uint16_t* len);

	void 	 setRowData(uint8_t* payload, uint16_t payloadLen);
	uint16_t getAvailableLength();
private:
	uint8_t* getBufferPos(uint8_t index);
	uint8_t* _buff;
	uint16_t _len;
	uint8_t  _elmCnt;
	uint8_t* _pos;
	uint8_t  _memDlt;
};

} /* linuxAsyncClient */

#endif /* PAYLOAD_H_ */
