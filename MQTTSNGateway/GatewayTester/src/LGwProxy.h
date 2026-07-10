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

#ifndef GWPROXY_H_
#define GWPROXY_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "LMqttsnClientApp.h"
#include "LNetworkUdp.h"
#include "LNetworkUdp6.h"
#include "LNetworkRfcomm.h"
#include "LNetworkDtls.h"
#include "LNetworkDtls6.h"
#include "LRegisterManager.h"
#include "LTimer.h"
#include "LTopicTable.h"

using namespace std;

#define GW_LOST              0
#define GW_SEARCHING         1
#define GW_CONNECTING        2
#define GW_WAIT_WILLTOPICREQ 3
#define GW_SEND_WILLTOPIC    4
#define GW_WAIT_WILLMSGREQ   5
#define GW_SEND_WILLMSG      6
#define GW_WAIT_CONNACK      7
#define GW_CONNECTED         8
#define GW_DISCONNECTING     9
#define GW_SLEEPING         10
#define GW_DISCONNECTED     11
#define GW_SLEPT            12
#define SSL_CONNECTING      13

#define GW_WAIT_PINGRESP     1

namespace linuxAsyncClient {
/*========================================
       Class LGwProxy
 =======================================*/
class LGwProxy{
public:
    LGwProxy();
    ~LGwProxy();

    void initialize(SENSORNET_CONFIG_t* netconf, LMqttsnConfig* mqconf);
    void     connect(void);
    void     disconnect(uint16_t sec = 0);
    int      getMessage(void);
    uint16_t registerTopic(char* topic, uint16_t toipcId);

    void     setWillTopic(const char* willTopic, uint8_t qos, bool retain = false);
    void     setWillMsg(const char* willMsg);
    void     setCleanSession(bool);
    void     setKeepAliveDuration(uint16_t duration);
    void     setAdvertiseDuration(uint16_t duration);
    void     setForwarderMode(bool valid);
    void     setQoSMinus1Mode(bool valid);
	void     setPingReqMode(bool valid);
	void     setAutoConnectMode(bool valid);
	void     setSessionMode(bool valid);
    void     reconnect(void);
    int      writeMsg(const uint8_t* msg);
    void     setPingReqTimer(void);
    uint16_t getNextMsgId();
    LTopicTable* getTopicTable(void);
    LRegisterManager* getRegisterManager(void);
    const char*    getClientId(void);
	uint8_t getStatus(void);
private:
    int      readMsg(void);
    void     writeGwMsg(void);
    void     checkPingReq(void);
    void     checkAdvertise(void);
    int      getConnectResponce(void);
    int      getDisconnectResponce(void);
    bool     isPingReqRequired(void);

    LNetwork     _network;
    uint8_t*    _mqttsnMsg;
    uint16_t    _nextMsgId;
    const char* _clientId;
    const char* _willTopic;
    const char* _willMsg;
    uint8_t     _cleanSession;
    uint8_t    _initialized;
    uint8_t     _retainWill;
    uint8_t     _qosWill;
    uint8_t     _gwId;
    uint16_t    _tkeepAlive;
    uint32_t    _tAdv;
    time_t      _sendUTC;
    int         _retryCount;
    int         _connectRetry;
    uint8_t     _status;
    time_t    _pingSendUTC;
    uint8_t     _pingRetryCount;
    uint8_t     _pingStatus;
    LRegisterManager _regMgr;
    LTopicTable  _topicTbl;
    LTimer       _gwAliveTimer;
    LTimer       _keepAliveTimer;
    uint16_t    _tSleep;
    uint16_t    _tWake;
    bool        _isForwarderMode;
    bool        _isQoSMinus1Mode;
	bool        _isPingReqMode;
	bool        _isAutoConnectMode;
    char        _msg[MQTTSN_MAX_MSG_LENGTH + 1];
};

} /* end of namespace */
#endif /* GWPROXY_H_ */
