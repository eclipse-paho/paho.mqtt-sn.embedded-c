/**************************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi
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

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include "SensorNetwork.h"
#include "MQTTSNGWProcess.h"

using namespace std;
using namespace MQTTSNGW;

#define LORA_PHY_MAXPAYLOAD      256
#define LORALINK_MAX_API_LEN     ( LORA_PHY_MAXPAYLOAD + 5 )  // PayloadType[1] + Rssi[2] + Snr[2]

#define LORALINK_ACK          0x10
#define LORALINK_NO_FREE_CH   0x20
#define LORALINK_TX_TIMEOUT   0x40

#define LORALINK_TIMEOUT_ACK  10000      // 10 secs

/*===========================================
 Class  SensorNetAddreess
 ============================================*/
SensorNetAddress::SensorNetAddress()
{
	_devAddr = 0;
}

SensorNetAddress::~SensorNetAddress()
{

}

void SensorNetAddress::setAddress( uint8_t devAddr)
{
	_devAddr = devAddr;
}


int SensorNetAddress::setAddress(string* address)
{
	_devAddr = atoi(address->c_str());

	if ( _devAddr == 0 )
	{
		return -1;
	}
	return 0;
}

void SensorNetAddress::setBroadcastAddress(void)
{
	_devAddr = BROADCAST_DEVADDR;
}

bool SensorNetAddress::isMatch(SensorNetAddress* addr)
{
	return _devAddr == addr->_devAddr;
}

SensorNetAddress& SensorNetAddress::operator =(SensorNetAddress& addr)
{
	_devAddr =  addr._devAddr;
	return *this;
}

char* SensorNetAddress::sprint(char* buf)
{
		snprintf(buf, 128, "%d", _devAddr);
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
	return LoRaLink::unicast(payload, payloadLength, sendToAddr);
}

int SensorNetwork::broadcast(const uint8_t* payload, uint16_t payloadLength)
{
	return LoRaLink::broadcast(payload, payloadLength);
}

int SensorNetwork::read(uint8_t* buf, uint16_t bufLen)
{
	return LoRaLink::recv(buf, bufLen, &_clientAddr);
}

void SensorNetwork::initialize(void)
{
	char param[MQTTSNGW_PARAM_MAX];
	uint32_t baudrate = 115200;

	if (theProcess->getParam("BaudrateLoRaLink", param) == 0)
	{
		baudrate = (uint32_t)atoi(param);
	}
	_description += "LoRaLink, Baudrate ";
	snprintf(param, sizeof(param), "%d", baudrate);
	_description += param;

	theProcess->getParam("DeviceRxLoRaLink", param);
	_description += ", SerialRx ";
	_description += param;
	errno = 0;

	if ( LoRaLink::open(LORALINK_MODEM_RX, param, baudrate) < 0 )
	{
		throw EXCEPTION("Can't open a LoRaLink", errno);
	}

	theProcess->getParam("DeviceTxLoRaLink", param);
	_description += ", SerialTx ";
	_description += param;
	errno = 0;

	if ( LoRaLink::open(LORALINK_MODEM_TX, param, baudrate) < 0 )
	{
		throw EXCEPTION("Can't open a LoRaLink", errno);
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
              Class  LoRaLink
 ============================================*/
LoRaLink::LoRaLink(){
    _serialPortRx = new SerialPort();
    _serialPortTx = new SerialPort();
    _respCd = 0;
}

LoRaLink::~LoRaLink(){
	if ( _serialPortRx )
	{
		delete _serialPortRx;
	}
	if ( _serialPortTx )
	{
		delete _serialPortTx;
	}
}

int LoRaLink::open(LoRaLinkModemType_t type, char* device, int baudrate)
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

	int rc = 0;
	if ( type == LORALINK_MODEM_RX )
	{
		if (  (rc = _serialPortRx->open(device, rate, false, 1, O_RDWR | O_NOCTTY)) < 0 )
		return rc;
	}
	else
	{
		rc =  _serialPortTx->open(device, rate, false, 1, O_RDWR | O_NOCTTY);
	}
	return rc;
}

int LoRaLink::broadcast(const uint8_t* payload, uint16_t payloadLen){
	SensorNetAddress addr;
	addr.setBroadcastAddress();
	return send(MQTT_SN, payload, (uint8_t) payloadLen, &addr);
}

int LoRaLink:: unicast(const uint8_t* payload, uint16_t payloadLen, SensorNetAddress* addr){
	return send(MQTT_SN, payload, (uint8_t) payloadLen, addr);
}

int LoRaLink::recv(uint8_t* buf, uint16_t bufLen, SensorNetAddress* clientAddr)
{
	while ( true )
	{
		if ( ( readApiFrame( &_loRaLinkApi, &_loRaLinkPara) == true ) && (_loRaLinkPara.Available == true) && ( _loRaLinkPara.Error == false ) )
		{
			clientAddr->_devAddr = _loRaLinkApi.SourceAddr;

			bufLen = _loRaLinkApi.PayloadLen;

			memcpy( buf, _loRaLinkApi.Payload, bufLen );

			switch ( (int) _loRaLinkApi.PayloadType )
			{
			case API_RSP_ACK:
				_respCd = LORALINK_ACK;
				break;

			case API_RSP_NFC:
				_respCd = LORALINK_NO_FREE_CH;
				break;

			case API_RSP_TOT:
				_respCd = LORALINK_TX_TIMEOUT;
				break;


			case MQTT_SN:
				memcpy( buf, _loRaLinkApi.Payload, bufLen );
				return bufLen;

			default:
				return 0;
			}
			_sem.post();
			return bufLen;

		}
		else
		{
		    return 0;
		}
	}
}



bool LoRaLink::readApiFrame(LoRaLinkFrame_t* api, LoRaLinkReadParameters_t* para)
{
	uint8_t byte = 0;
	uint16_t val = 0;

	while ( recv(&byte) == 0 )
	{
		if ( byte == START_BYTE )
		{
			para->apipos= 1;
			para->Error = true;
			para->Available = false;
			continue;
		}

		if ( para->apipos > 0 && byte == ESCAPE )
		{
			if( recv(&byte ) == 0 )
			{
				byte ^= PAD;  // decode
			}
			else
			{
				para->Escape = true;
				continue;
			}
		}

		if( para->Escape == true )
		{
			byte ^= PAD;
			para->Escape = false;
		}

		switch ( para->apipos )
		{
		case 0:
			break;

		case 1:
			val = (uint16_t)byte;
			api->PayloadLen = val << 8;
			break;

		case 2:
			api->PayloadLen += byte;
			break;

		case 3:
			api->SourceAddr = byte;
			para->checksum = byte;
			break;

		case 4:
			val = (uint16_t)byte;
			api->Rssi = val << 8;
			para->checksum += byte;
			break;

		case 5:
			api->Rssi += byte;
			para->checksum += byte;
			break;

		case 6:
			val = (uint16_t)byte;
			api->Snr = val << 8;
			para->checksum += byte;
			break;

		case 7:
			api->Snr += byte;
			para->checksum += byte;
			break;

		case 8:
			api->PayloadType = byte;
			para->checksum += byte;
			break;

		default:
			if ( para->apipos >= api->PayloadLen + 2 )    //  FRM_DEL + CRC = 2
			{
				para->Error = ( (0xff - para->checksum) != byte );
				para->Available = true;
				api->PayloadLen -= 7;   // 7 = SrcAddr[1] + Rssi[2] + Snr[2] + PlType[1] + Crc[1]
				para->apipos = 0;
				para->checksum = 0;
				return true;
			}
			else
			{
				para->checksum += byte;
				api->Payload[para->apipos - 9] = byte;
			}
			break;
		}

		para->apipos++;
	}
	return false;
}

int LoRaLink::send(LoRaLinkPayloadType_t type, const uint8_t* payload, uint16_t pLen, SensorNetAddress* addr)
{
    D_LRSTACK("\r\n===> Send:    ");
	uint8_t buf[2] = { 0 };
	uint8_t chks = 0;
	uint16_t len = pLen + 3;   // 3 = DestAddr[1] + PayloadType[1] + Crc[1]
	_respCd = 0;

    _serialPortTx->send(START_BYTE);

    buf[0] = (len >> 8) & 0xff;
    buf[1] = len & 0xff;
    send(buf[0]);
    send(buf[1]);

    send( addr->_devAddr );
    chks = addr->_devAddr;



    send(type);
    chks += type;

    D_LRSTACK("\r\n     Payload: ");

    for ( uint8_t i = 0; i < pLen; i++ ){
        send(payload[i]);     // Payload
        chks += payload[i];
    }

    chks = 0xff - chks;
    D_LRSTACK("   checksum  ");
    send(chks);
    D_LRSTACK("\r\n");

    /* wait ACK */
    _sem.timedwait(LORALINK_TIMEOUT_ACK);

    if ( _respCd == LORALINK_NO_FREE_CH )
    {
        D_LRSTACK(" Channel isn't free\r\n");
    	return -1;
    }
    else if ( _respCd != LORALINK_ACK )
	{
        D_LRSTACK(" Not Acknowleged\r\n");
		return -1;
	}
    return (int)pLen;
}

void LoRaLink::send(uint8_t c)
{
  if( (c == START_BYTE || c == ESCAPE || c == XON || c == XOFF)){
	  _serialPortTx->send(ESCAPE);
	  _serialPortTx->send(c ^ PAD);
  }else{
	  _serialPortTx->send(c);
  }
}

int LoRaLink::recv(uint8_t* buf)
{
    struct timeval timeout;
    int maxfd;
    int fd;
    fd_set rfds;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;    // 500ms
    FD_ZERO(&rfds);
    FD_SET(_serialPortRx->_fd, &rfds);
    FD_SET(_serialPortTx->_fd, &rfds);
    if ( _serialPortRx->_fd > _serialPortTx->_fd )
    {
    	maxfd = _serialPortRx->_fd;
    }
    else
    {
    	maxfd = _serialPortTx->_fd;
    }

    if ( select(maxfd + 1, &rfds, 0, 0, &timeout) > 0 )
    {
    	if ( FD_ISSET(_serialPortRx->_fd, &rfds) )
		{
    		fd = _serialPortRx->_fd;
		}
    	else
		{
    		fd = _serialPortTx->_fd;
		}

		if ( read(fd, buf, 1) == 1 )
		{
			/*
			if ( *buf == ESCAPE )
			{
             D_LRSTACK( " %02x",buf[0] );
				if ( read(fd, buf, 1) == 1 )
				{
					*buf = PAD ^ *buf;
				}
				else
				{
					return -1;
				}

			}
			*/
            D_LRSTACK(" %02x", buf[0]);
			return 0;
		}
	}
    return -1;
}

/*=========================================
 Class SerialPort
 =========================================*/
SerialPort::SerialPort()
{
	_tio.c_iflag = IGNBRK | IGNPAR;
	_tio.c_cflag = CS8 | CLOCAL | CREAD;
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
	if (write(_fd, &b, 1) <= 0)
	{
		return false;
	}
	else
	{
        D_LRSTACK(" %02x", b);
		return true;
	}
}



void SerialPort::flush(void)
{
	tcsetattr(_fd, TCSAFLUSH, &_tio);
}

