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

#ifndef SENSORNETWORK_H_
#define SENSORNETWORK_H_

#include "MQTTSNGWDefines.h"
#include <string>
#include <poll.h>

using namespace std;

namespace MQTTSNGW
{

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
class SensorNetAddress
{
public:
	SensorNetAddress();
	~SensorNetAddress();
	void setAddress(uint32_t IpAddr, uint16_t port);
	int  setAddress(string* data);
	uint16_t getPortNo(void);
	uint32_t getIpAddress(void);
	bool isMatch(SensorNetAddress* addr);
	SensorNetAddress& operator =(SensorNetAddress& addr);
	char* sprint(char* buf);
private:
	uint16_t _portNo;
	uint32_t _IpAddr;
};

/*========================================
 Class UpdPort
 =======================================*/
class UDPPort
{
public:
	UDPPort();
	virtual ~UDPPort();

	int open(const char* ipAddress, uint16_t multiPortNo,	uint16_t uniPortNo, unsigned int hops);
	void close(void);
	int unicast(const uint8_t* buf, uint32_t length, SensorNetAddress* sendToAddr);
	int broadcast(const uint8_t* buf, uint32_t length);
	int recv(uint8_t* buf, uint16_t len, SensorNetAddress* addr);

private:
	void setNonBlocking(const bool);
	int recvfrom(int sockfd, uint8_t* buf, uint16_t len, uint8_t flags,	SensorNetAddress* addr);

    pollfd _pollFds[2];
	bool _disconReq;
    SensorNetAddress _multicastAddr;
};

/*===========================================
 Class  SensorNetwork
 ============================================*/
class SensorNetwork: public UDPPort
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
	SensorNetAddress _senderAddr;
	string _description;
};

}
#endif /* SENSORNETWORK_H_ */
