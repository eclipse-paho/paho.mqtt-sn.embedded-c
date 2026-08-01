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

#ifdef  DEBUG_NW
#define D_LRSTACK(...) printf(__VA_ARGS__); fflush(stdout)
#else
  #define D_LRSTACK(...)
#endif


#define XMIT_STATUS_TIME_OVER    5000

#define START_BYTE               0x7e
#define ESCAPE                   0x7d
#define XON                      0x11
#define XOFF                     0x13
#define PAD                      0x20

#define BROADCAST_DEVADDR        0xFF

#define LORA_PHY_MAXPAYLOAD      256

/*!
 * LoRaLink Modem Type
 */
typedef enum
{
	LORALINK_MODEM_TX,
	LORALINK_MODEM_RX,
}LoRaLinkModemType_t;

/*!
 * LoRaLink Serialized API
 */
typedef struct
{
	uint16_t  PanId;
	uint8_t   DestinationAddr;
	uint8_t   SourceAddr;
	uint16_t  Rssi;
	uint16_t  Snr;
	uint8_t   PayloadType;
	uint8_t   Payload[LORA_PHY_MAXPAYLOAD];
	uint16_t  PayloadLen;
}LoRaLinkFrame_t;

typedef struct
{
	bool Available;
	bool Error;
	bool Escape;
	uint16_t apipos;
	uint8_t checksum;
} LoRaLinkReadParameters_t;

typedef enum
{
	MQTT_SN        = 0x40,
	API_RSP_ACK    = 0x80,
	API_RSP_NFC,
	API_RSP_TOT,
	API_REQ_UTC,
	API_RSP_UTC,
	API_CHG_TASK_PARAM,
	API_REQ_RESET,

}LoRaLinkPayloadType_t;

/*===========================================
  Class  SerialPort
 ============================================*/
class SerialPort
{
	friend class LoRaLink;
public:
	SerialPort();
	~SerialPort();
	int open(char* devName, unsigned int baudrate,  bool parity, unsigned int stopbit, unsigned int flg);
	bool send(unsigned char b);
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
	friend class LoRaLink;
public:
	SensorNetAddress();
	~SensorNetAddress();
	void setAddress( uint8_t devAddr);
	int  setAddress(string* data);
	void setBroadcastAddress(void);
	bool isMatch(SensorNetAddress* addr);
	SensorNetAddress& operator =(SensorNetAddress& addr);
	char* sprint(char*);
private:
	uint8_t _devAddr;
//	uint8_t _destAddr;
};

/*========================================
 Class LoRaLink
 =======================================*/
class LoRaLink
{
public:
	LoRaLink();
	~LoRaLink();

	int open(LoRaLinkModemType_t type, char* device, int boudrate );
	void close(void);
	int unicast(const uint8_t* buf, uint16_t length, SensorNetAddress* sendToAddr);
	int broadcast(const uint8_t* buf, uint16_t length);
	int recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr);
	void setApiMode(uint8_t mode);

private:
	bool readApiFrame(LoRaLinkFrame_t* api, LoRaLinkReadParameters_t* para);
	int recv(uint8_t* buf);
	int send(LoRaLinkPayloadType_t type, const uint8_t* payload, uint16_t pLen, SensorNetAddress* addr);
	void send(uint8_t b);

	Semaphore _sem;
	Mutex _meutex;
	uint8_t _respCd;
	SerialPort* _serialPortRx;
	SerialPort* _serialPortTx;
	LoRaLinkFrame_t _loRaLinkApi;
	LoRaLinkReadParameters_t _loRaLinkPara;
};

/*===========================================
 Class  SensorNetwork
 ============================================*/
class SensorNetwork: public LoRaLink
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
