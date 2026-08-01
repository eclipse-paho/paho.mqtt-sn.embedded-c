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

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"

using namespace std;
using namespace MQTTSNGW;

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
SensorNetAddress::SensorNetAddress()
{
	memset(_address64, 0, 8);
	memset(_address16, 0, 2);
}

SensorNetAddress::~SensorNetAddress()
{

}

void SensorNetAddress::setAddress(uint8_t* address64, uint8_t* address16)
{
	memcpy(_address64, address64, 8);
	memcpy(_address16, address16, 2);
}


int SensorNetAddress::setAddress(string* address64)
{
	memcpy(_address64, address64->c_str(), 8);
	memset(_address16, 0, sizeof(_address16));
	return 0;
}

void SensorNetAddress::setBroadcastAddress(void)
{
	memset(_address64, 0, 6);
	_address64[6] = 0xff;
	_address64[7] = 0xff;
	_address16[0] = 0xff;
	_address16[1] = 0xfe;
}

bool SensorNetAddress::isMatch(SensorNetAddress* addr)
{

	return (memcmp(this->_address64, addr->_address64, 8 ) == 0 &&  memcmp(this->_address16, addr->_address16, 2) == 0);
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress& addr)
{
	memcpy(_address64, addr._address64, 8);
	memcpy(_address16, addr._address16, 2);
	return *this;
}

char* SensorNetAddress::sprint(char* buf)
{
	char* pbuf = buf;
	for ( int i = 0; i < 8; i++ )
	{
		snprintf(pbuf, 3, "%02X", _address64[i]);
		pbuf += 2;
	}
	return buf;
}

/*===========================================
 Class  SensorNetwork
 ============================================*/
SensorNetwork::SensorNetwork()
{

}

SensorNetwork::~SensorNetwork()
{

}

int SensorNetwork::unicast(const uint8_t* payload, uint16_t payloadLength, SensorNetAddress* sendToAddr)
{
	return XBee::unicast(payload, payloadLength, sendToAddr);
}

int SensorNetwork::broadcast(const uint8_t* payload, uint16_t payloadLength)
{
	return XBee::broadcast(payload, payloadLength);
}

int SensorNetwork::read(uint8_t* buf, uint16_t bufLen)
{
	return XBee::recv(buf, bufLen, &_clientAddr);
}

void SensorNetwork::initialize(void)
{
	char param[MQTTSNGW_PARAM_MAX];
	uint32_t baudrate = 9600;
	uint8_t apimode = 2;

	if (theProcess->getParam("ApiMode", param) == 0)
	{
		apimode = (uint8_t)atoi(param);
	}
	setApiMode(apimode);
	_description = "API mode ";
	snprintf(param, sizeof(param), "%d", apimode);
	_description += param;

	if (theProcess->getParam("Baudrate", param) == 0)
	{
		baudrate = (uint32_t)atoi(param);
	}
	_description += ", Baudrate ";
	snprintf(param, sizeof(param), "%d", baudrate);
	_description += param;

	theProcess->getParam("SerialDevice", param);
	_description += ", SerialDevice ";
	_description += param;

	errno =0;

	if ( XBee::open(param, baudrate) < 0 )
	{
		throw EXCEPTION("Can't open a XBee", errno);
	}
}

const char* SensorNetwork::getDescription(void)
{
	return _description.c_str();
}

SensorNetAddress* SensorNetwork::getSenderAddress(void)
{
	return &_clientAddr;
}

/*===========================================
              Class  XBee
 ============================================*/
XBee::XBee(){
    _serialPort = new SerialPort();
    _respCd = 0;
    _respId = 0;
    _dataLen = 0;
    _frameId = 0;
    _apiMode = 2;
}

XBee::~XBee(){
	if ( _serialPort )
	{
		delete _serialPort;
	}
}

int XBee::open(char* device, int baudrate)
{
	int rate = B9600;

	switch (baudrate)
	{
	case 9600:
		rate = B9600;
		break;
	case 19200:
		rate = B19200;
		break;
	case 38400:
		rate = B38400;
		break;
	case 57600:
		rate = B57600;
		break;
	case 115200:
		rate = B115200;
		break;
	default:
		return -1;
	}

	return _serialPort->open(device, rate, false, 1, O_RDWR | O_NOCTTY);
}

int XBee::broadcast(const uint8_t* payload, uint16_t payloadLen){
	SensorNetAddress addr;
	addr.setBroadcastAddress();
	return send(payload, (uint8_t) payloadLen, &addr);
}

int XBee:: unicast(const uint8_t* payload, uint16_t payloadLen, SensorNetAddress* addr){
	return send(payload, (uint8_t) payloadLen, addr);
}

int XBee::recv(uint8_t* buf, uint16_t bufLen, SensorNetAddress* clientAddr)
{
	uint8_t data[128];
	int len;

	while ( true )
	{

		if ( (len = readApiFrame(data)) > 0 )
		{

			if ( data[0] == API_RESPONSE )
			{
				memcpy(clientAddr->_address64, data + 1, 8);
				memcpy(clientAddr->_address16, data + 9, 2);
				len -= 12;
				memcpy( buf, data + 12, len);
				return len;
			}
			else if ( data[0] == API_XMITSTATUS )
			{
				_respCd = data[5];
				_respId = data[1];
				_sem.post();
			}
		}
		else
		{
		    return 0;
		}
	}
}

int XBee::readApiFrame(uint8_t* recvData){
	uint8_t buf;
	uint8_t pos = 0;
	uint8_t checksum = 0;
	uint8_t len = 0;

	 while ( _serialPort->recv(&buf) )
	 {
		 if ( buf == START_BYTE)
		 {
			pos = 1;
			 D_NWSTACK("\r\n===> Recv:    ");
			 break;
		 }
	 }

	if ( pos == 0 )
	{
		goto errexit;
	}

    if ( recv(&buf) < 0 ) // MSB length
    {
    	goto errexit;
    }

    if ( recv(&buf) < 0 ) // LSB length
	{
    	goto errexit;
	}
	else
	{
		len = buf;
	}

    pos = 0;
    while ( len-- )
    {
    	if ( recv(&buf) < 0 )
    	{
    		goto errexit;
    	}
    	recvData[pos++] = buf;
    	checksum += buf;
    }

    recv(&buf);        // checksum
    if ( (0xff - checksum ) == buf ){
    	D_NWSTACK("    checksum ok\r\n");
    	return pos;
    }
    else
    {
    	D_NWSTACK("    checksum error  %02x\r\n", 0xff - checksum);
    	goto errexit;
    }
errexit:
	_serialPort->flush();
	return -1;
}

int XBee::send(const uint8_t* payload, uint8_t pLen, SensorNetAddress* addr){
	D_NWSTACK("\r\n===> Send:    ");
    uint8_t checksum = 0;
    _respCd = -1;

    _serialPort->send(START_BYTE);
    send(0x00);              // Message Length
    send(14 + pLen);         // Message Length

    _serialPort->send(API_XMITREQUEST); // Transmit Request API
    checksum += API_XMITREQUEST;

    if (_frameId++ == 0x00 ) // Frame ID
	{
    	_frameId = 1;
	}
    send(_frameId);
    checksum += _frameId;

	for ( int i = 0; i < 8; i++)    // Address64
	{
		send(addr->_address64[i]);
		checksum += addr->_address64[i];
	}
	for ( int i = 0; i < 2; i++)    // Address16
	{
		send(addr->_address16[i]);
		checksum += addr->_address16[i];
	}

    send(0x00);   // Broadcast Radius
    checksum += 0x00;

    send(0x00);   // Option: Use the extended transmission timeout 0x40
    checksum += 0x00;

    D_NWSTACK("\r\n     Payload: ");

    for ( uint8_t i = 0; i < pLen; i++ ){
        send(payload[i]);     // Payload
        checksum += payload[i];
    }

    checksum = 0xff - checksum;
    D_NWSTACK("   checksum  ");
    send(checksum);
    D_NWSTACK("\r\n");

    /* wait Txim Status 0x8B */
    _sem.timedwait(XMIT_STATUS_TIME_OVER);

    if ( _respCd || _frameId != _respId )
    {
    	 D_NWSTACK(" frameId = %02x  Not Acknowleged\r\n", _frameId);
    	return -1;
    }
    return (int)pLen;
}

void XBee::send(uint8_t c)
{
  if(_apiMode == 2 && (c == START_BYTE || c == ESCAPE || c == XON || c == XOFF)){
	  _serialPort->send(ESCAPE);
	  _serialPort->send(c ^ 0x20);
  }else{
	  _serialPort->send(c);
  }
}

int XBee::recv(uint8_t* buf)
{
	if (_serialPort->recv(buf) )
	{
		if ( *buf == ESCAPE && _apiMode == 2 )
		{
			_serialPort->recv(buf);
			*buf = 0x20 ^ *buf;
		}
		return 0;
	}
	return -1;
}

void XBee::setApiMode(uint8_t mode)
{
	_apiMode = mode;
}

/*=========================================
 Class SerialPort
 =========================================*/
SerialPort::SerialPort()
{
	_tio.c_iflag = IGNBRK | IGNPAR;
	_tio.c_cflag = CS8 | CLOCAL | CRTSCTS | CREAD;
	_tio.c_cc[VINTR] = 0;
	_tio.c_cc[VTIME] = 10;   // 1 sec.
	_tio.c_cc[VMIN] = 1;
	_fd = 0;
}

SerialPort::~SerialPort()
{
	if (_fd)
	{
		::close(_fd);
	}
}

int SerialPort::open(char* devName, unsigned int baudrate, bool parity,
		unsigned int stopbit, unsigned int flg)
{
	_fd = ::open(devName, flg);
	if (_fd < 0)
	{
		return _fd;
	}

	if (parity)
	{
		_tio.c_cflag = _tio.c_cflag | PARENB;
	}
	if (stopbit == 2)
	{
		_tio.c_cflag = _tio.c_cflag | CSTOPB;
	}

	if (cfsetspeed(&_tio, baudrate) < 0)
	{
		return errno;
	}
	return tcsetattr(_fd, TCSANOW, &_tio);
}

bool SerialPort::send(unsigned char b)
{
	if (write(_fd, &b, 1) < 0)
	{
		return false;
	}
	else
	{
		D_NWSTACK( " %02x", b);
		return true;
	}
}

bool SerialPort::recv(unsigned char* buf)
{
    struct timeval timeout;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(_fd, &rfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;    // 500ms
    if ( select(_fd + 1, &rfds, 0, 0, &timeout) > 0 )
    {
        if (read(_fd, buf, 1) > 0)
        {
            D_NWSTACK( " %02x",buf[0] );
            return true;
        }
    }
    return false;
}

void SerialPort::flush(void)
{
	tcsetattr(_fd, TCSAFLUSH, &_tio);
}

