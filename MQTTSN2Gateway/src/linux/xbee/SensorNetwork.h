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
 *    Tomoaki Yamaguchi - initial API and implementation 
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/
#ifndef SENSORNETWORKX_H_
#define SENSORNETWORKX_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWProcess.h"
#include <string>
#include <termios.h>

using namespace std;

namespace MQTTSNGW
{

#define API_XMITREQUEST          0x10
#define API_RESPONSE             0x90
#define API_MODEMSTATUS          0x8A
#define API_XMITSTATUS           0x8B

#define XMIT_STATUS_TIME_OVER    5000

#define START_BYTE               0x7e
#define ESCAPE                   0x7d
#define XON                      0x11
#define XOFF                     0x13

/*===========================================
  Class  SerialPort
 ============================================*/
class SerialPort{
public:
	SerialPort();
	~SerialPort();
	int open(char* devName, unsigned int baudrate,  bool parity, unsigned int stopbit, unsigned int flg);
	bool send(unsigned char b);
	bool recv(unsigned char* b);
	void flush();

private:
	int _fd;  // file descriptor
	struct termios _tio;
};

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
class SensorNetAddress
{
	friend class XBee;
public:
	SensorNetAddress();
	~SensorNetAddress();
	void setAddress(uint8_t* address64, uint8_t* address16);
	int  setAddress(string* data);
	void setBroadcastAddress(void);
	bool isMatch(SensorNetAddress* addr);
	SensorNetAddress& operator =(SensorNetAddress& addr);
	char* sprint(char*);
private:
	uint8_t _address16[2];
	uint8_t _address64[8];
};

/*========================================
 Class XBee
 =======================================*/
class XBee
{
public:
	XBee();
	~XBee();

	int open(char* device, int boudrate);
	void close(void);
	int unicast(const uint8_t* buf, uint16_t length, SensorNetAddress* sendToAddr);
	int broadcast(const uint8_t* buf, uint16_t length);
	int recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr);
	void setApiMode(uint8_t mode);

private:
	int readApiFrame(uint8_t* recvData);
	int recv(uint8_t* buf);
	int send(const uint8_t* payload, uint8_t pLen, SensorNetAddress* addr);
	void send(uint8_t b);

	Semaphore _sem;
	Mutex _meutex;
	SerialPort* _serialPort;
	uint8_t _frameId;
	uint8_t _respCd;
	uint8_t _respId;
	uint8_t _dataLen;
	uint8_t _apiMode;
};

/*===========================================
 Class  SensorNetwork
 ============================================*/
class SensorNetwork: public XBee
{
public:
	SensorNetwork();
	~SensorNetwork();

	int unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendto);
	int broadcast(const uint8_t* payload, uint16_t payloadLength);
	int read(uint8_t* buf, uint16_t bufLen);
	void initialize(void);
	const char* getDescription(void);
	SensorNetAddress* getSenderAddress(void);

private:
	SensorNetAddress _clientAddr;   // Sender's address. not gateway's one.
	string _description;
};

}

#endif /* SENSORNETWORKX_H_ */
