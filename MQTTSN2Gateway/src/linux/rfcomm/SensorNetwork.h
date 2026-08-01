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

#ifndef SENSORNETWORK_H_
#define SENSORNETWORK_H_

#include "MQTTSNGWDefines.h"
#include <string>
#include <bluetooth/bluetooth.h>

using namespace std;

namespace MQTTSNGW
{

#define MAX_RFCOMM_CH 30

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
class SensorNetAddress
{
public:
	SensorNetAddress();
	~SensorNetAddress();
	void setAddress(bdaddr_t bdAddr, uint16_t channel);
	int  setAddress(string* data);
	uint16_t getPortNo(void);
    bdaddr_t* getAddress(void);
	bool isMatch(SensorNetAddress* addr);
	SensorNetAddress& operator =(SensorNetAddress& addr);
	char* sprint(char* buf);
private:
	uint16_t _channel;
	bdaddr_t _bdAddr;
};

/*========================================
 Class RfcommPort
 =======================================*/
class RfcommPort
{
    friend class SensorNetwork;
public:
    RfcommPort();
    virtual ~RfcommPort();

    int open(bdaddr_t* devAddress, uint16_t channel);
	void close(void);
    int send(const uint8_t* buf, uint32_t length);
    int recv(uint8_t* buf, uint16_t len);
    int getSock(void);
    int accept(SensorNetAddress* addr);
private:
	int _rfCommSock;
    int _listenSock;
    uint16_t _channel;
	bool _disconReq;
};

/*===========================================
 Class  SensorNetwork
 ============================================*/
class SensorNetwork
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
    // sockets for RFCOMM
    RfcommPort _rfPorts[MAX_RFCOMM_CH];
	SensorNetAddress _senderAddr;
	string _description;
};

}
#endif /* SENSORNETWORK_H_ */
