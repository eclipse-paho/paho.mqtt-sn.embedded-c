/**************************************************************************************
 * Copyright (c) 2021, Tomoaki Yamaguchi
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

#ifndef NETWORKRFCOMM_H_
#define NETWORKRFCOMM_H_

#ifdef RFCOMM

#include <sys/time.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <bluetooth/bluetooth.h>


#define SOCKET_MAXHOSTNAME  200
#define SOCKET_MAXCONNECTIONS  5
#define SOCKET_MAXRECV  500
#define SOCKET_MAXBUFFER_LENGTH 500 // buffer size

#define STAT_UNICAST   1
#define STAT_MULTICAST 2

using namespace std;

namespace linuxAsyncClient
{
/*========================================
     Class LRfcommPort
 =======================================*/
class LRfcommPort
{
    friend class LNetwork;
public:
    LRfcommPort();
    virtual ~LRfcommPort();
    bool open(LRfcommConfig* config);
    int unicast(const uint8_t* buf, uint32_t length);
    int recv(uint8_t* buf, uint16_t len, bool nonblock);
    bool checkRecvBuf();
    bool isUnicast();

private:
    void close();

    int _sockRfcomm;
    uint8_t _gwAddress[6];
    uint8_t _channel;
    bool _disconReq;

};

#define NO_ERROR    0
#define PACKET_EXCEEDS_LENGTH  1
/*===========================================
 Class  Network
 ============================================*/
class LNetwork: public LRfcommPort
{
public:
    LNetwork();
    ~LNetwork();

    int broadcast(const uint8_t* payload, uint16_t payloadLen);
    int unicast(const uint8_t* payload, uint16_t payloadLen);
    void setGwAddress(void);
    void resetGwAddress(void);
    void setFixedGwAddress(void);
        bool initialize(LRfcommConfig* config);
    uint8_t* getMessage(int* len);
    bool isBroadcastable();

private:
    void setSleep();
    int readApiFrame(void);

    int _returnCode;
    bool _sleepflg;
    uint8_t _rxDataBuf[MQTTSN_MAX_PACKET_SIZE + 1];  // defined in MqttsnClientApp.h

};

} /* end of namespace */
#endif /* RFCOMM */
#endif /* NETWORKRFCOM_H_ */
