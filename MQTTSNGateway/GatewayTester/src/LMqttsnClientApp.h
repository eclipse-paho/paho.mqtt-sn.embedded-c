/**************************************************************************************
 * Copyright (c) 2016-2026, Tomoaki Yamaguchi
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

#ifndef MQTTSNCLIENTAPP_H_
#define MQTTSNCLIENTAPP_H_

/*======================================
 *         Debug Flag
 ======================================*/
//#define DEBUG_NW
//#define DEBUG_MQTTSN

/*======================================
 *     Program mode Flag
 ======================================*/
//#define CLIENT_MODE

/****************************************
      MQTT-SN Parameters
*****************************************/
#define MAX_INFLIGHT_MSG   10
#define MQTTSN_MAX_MSG_LENGTH  1024
#define MQTTSN_MAX_PACKET_SIZE 1024

#define MQTTSN_DEFAULT_KEEPALIVE   900     // 1H
#define MQTTSN_DEFAULT_DURATION    900     // 15min
#define MQTTSN_TIME_SEARCHGW         3
#define MQTTSN_TIME_RETRY           10
#define MQTTSN_RETRY_COUNT           3

/****************************************
      Application config structures
*****************************************/
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;

/****************************************
      Application config structures
*****************************************/

struct LMqttsnConfig
{
	uint16_t keepAlive;
	bool     cleanSession;
	uint32_t sleepDuration;
	const char* willTopic;
	const char* willMsg;
    uint8_t  willQos;
    bool     willRetain;
};

struct LUdpConfig
{
	const char* clientId;
	uint8_t  ipAddress[4];
	uint16_t gPortNo;
	uint16_t uPortNo;
};

struct LUdp6Config
{
    const char* clientId;
    const char* ipAddress;
    const char *interface;
    uint16_t gPortNo;
    uint16_t uPortNo;
};

struct LRfcommConfig
{
    const char* clientId;
    const char* gwAddress;
    uint8_t channel;
};


typedef enum
{
    MQTTSN_TOPIC_TYPE_NORMAL,
    MQTTSN_TOPIC_TYPE_PREDEFINED,
    MQTTSN_TOPIC_TYPE_SHORT
} MQTTSN_topicTypes;


/*======================================
      MACROs for Application
=======================================*/
#define MQTTSN_CONFIG    MqttsnConfig  theMqttsnConfig
#define MQTTSNCONF LMqttsnConfig  theMqcon

#if defined(UDP)
#define UDPCONF  LUdpConfig theNetcon
#define UDP6CONF  LUdp6Config theU6Conf
#define RFCOMMCONF  LRfcommConfig theRfConf
#define SENSORNET_CONFIG_t  LUdpConfig

#elif defined(UDP6)
#define UDP6CONF  LUdp6Config theNetcon
#define UDPCONF  LUdpConfig theUConf
#define RFCOMMCONF  LRfcommConfig theRfConf
#define SENSORNET_CONFIG_t LUdp6Config

#elif defined(RFCOMM)
#define RFCOMMCONF  LRfcommConfig theNetcon
#define UDPCONF  LUdpConfig theUConf
#define UDP6CONF  LUdp6Config theU6Conf
#define SENSORNET_CONFIG_t  LRfcommConfig

#elif defined(DTLS)
#define UDPCONF  LUdpConfig theNetcon
#define UDP6CONF  LUdp6Config theU6Conf
#define RFCOMMCONF  LRfcommConfig theRfConf
#define SENSORNET_CONFIG_t  LUdpConfig

#elif defined(DTLS6)
#define UDPCONF  LUdpConfig theUConf
#define UDP6CONF  LUdp6Config theNetcon
#define RFCOMMCONF  LRfcommConfig theRfConf
#define SENSORNET_CONFIG_t  LUdp6Config
#else
#error "UDP, UDP6, DTLS, DTLS6 or RFCOMM is not defined in LMqttsnClientApp.h"
#endif



#define CONNECT(...) theClient->getGwProxy()->connect(__VA_ARGS__)
#define PUBLISH(...)     theClient->publish(__VA_ARGS__)
#define SUBSCRIBE(...)   theClient->subscribe(__VA_ARGS__)
#define SUBSCRIBE_PREDEF(...)   theClient->subscribePredefinedId(__VA_ARGS__)
#define UNSUBSCRIBE(...) theClient->unsubscribe(__VA_ARGS__)
#define DISCONNECT(...)  theClient->disconnect(__VA_ARGS__)
#define ONCONNECT() theClient->getSubscribeManager()->onConnect()

#define TASK_LIST         TaskList theTaskList[]
#define TASK(...)         {__VA_ARGS__, 0, 0}
#define END_OF_TASK_LIST  {0, 0, 0, 0}
#define TEST_LIST		  TestList theTestList[]
#define TEST(...)         {__VA_ARGS__, 0}
#define END_OF_TEST_LIST  {0, 0, 0}
#define SUBSCRIBE_LIST    OnPublishList theOnPublishList[]
#define SUB(...)          {__VA_ARGS__}
#define END_OF_SUBSCRIBE_LIST {MQTTSN_TOPIC_TYPE_NORMAL,0,0,0, 0}


#define SetForwarderMode(...)  theClient->getGwProxy()->setForwarderMode(__VA_ARGS__)
#define SetQoSMinus1Mode(...) theClient->getGwProxy()->setQoSMinus1Mode(__VA_ARGS__)
#define SetAutoConnectMode(...) theClient->setAutoConnectMode(__VA_ARGS__)
#define SetAutoPingReqMode(...) theClient->getGwProxy()->setPingReqMode(__VA_ARGS__)
#define SetCleanSession(...) theClient->getGwProxy()->setSessionMode(__VA_ARGS__)
#ifdef CLIENT_MODE
#define DISPLAY(...)
#define PROMPT(...)
#define CHECKKEYIN(...) theScreen->checkKeyIn(__VA_ARGS__)
#else
#define DISPLAY(...) theScreen->display(__VA_ARGS__)
#define PROMPT(...) theScreen->prompt(__VA_ARGS__)
#define CHECKKEYIN(...) theScreen->checkKeyIn(__VA_ARGS__)
#endif
/*======================================
      MACROs for debugging
========================================*/
#ifndef DEBUG_NW
	#define D_NWLOG(...)
#else
	#define D_NWLOG(...)    printf(__VA_ARGS__)
#endif

#ifndef DEBUG_MQTTSN
	#define D_MQTTLOG(...)
#else
	#define D_MQTTLOG(...)  printf(__VA_ARGS__)
#endif

#ifndef DEBUG_OTA
	#define D_OTALOG(...)
#else
	#define D_OTALOG(...)   printf(__VA_ARGS__)
#endif

/*======================================
      MQTT-SN Defines
========================================*/
#define QoS0  0
#define QoS1  1
#define QoS2  2
#define Q0Sm1 3
#define MQTTSN_TYPE_ADVERTISE     0x00
#define MQTTSN_TYPE_SEARCHGW      0x01
#define MQTTSN_TYPE_GWINFO        0x02
#define MQTTSN_TYPE_CONNECT       0x04
#define MQTTSN_TYPE_CONNACK       0x05
#define MQTTSN_TYPE_WILLTOPICREQ  0x06
#define MQTTSN_TYPE_WILLTOPIC     0x07
#define MQTTSN_TYPE_WILLMSGREQ    0x08
#define MQTTSN_TYPE_WILLMSG       0x09
#define MQTTSN_TYPE_REGISTER      0x0A
#define MQTTSN_TYPE_REGACK        0x0B
#define MQTTSN_TYPE_PUBLISH       0x0C
#define MQTTSN_TYPE_PUBACK        0x0D
#define MQTTSN_TYPE_PUBCOMP       0x0E
#define MQTTSN_TYPE_PUBREC        0x0F
#define MQTTSN_TYPE_PUBREL        0x10
#define MQTTSN_TYPE_SUBSCRIBE     0x12
#define MQTTSN_TYPE_SUBACK        0x13
#define MQTTSN_TYPE_UNSUBSCRIBE   0x14
#define MQTTSN_TYPE_UNSUBACK      0x15
#define MQTTSN_TYPE_PINGREQ       0x16
#define MQTTSN_TYPE_PINGRESP      0x17
#define MQTTSN_TYPE_DISCONNECT    0x18
#define MQTTSN_TYPE_WILLTOPICUPD  0x1A
#define MQTTSN_TYPE_WILLTOPICRESP 0x1B
#define MQTTSN_TYPE_WILLMSGUPD    0x1C
#define MQTTSN_TYPE_WILLMSGRESP   0x1D
#define MQTTSN_TYPE_ENCAPSULATED  0xFE

#define MQTTSN_TOPIC_TYPE           0x03

#define MQTTSN_FLAG_DUP     0x80
#define MQTTSN_FLAG_QOS_0   0x0
#define MQTTSN_FLAG_QOS_1   0x20
#define MQTTSN_FLAG_QOS_2   0x40
#define MQTTSN_FLAG_QOS_M1  0x60
#define MQTTSN_FLAG_RETAIN  0x10
#define MQTTSN_FLAG_WILL    0x08
#define MQTTSN_FLAG_CLEAN   0x04

#define MQTTSN_PROTOCOL_ID  0x01
#define MQTTSN_HEADER_SIZE  2

#define MQTTSN_RC_ACCEPTED                  0x00
#define MQTTSN_RC_REJECTED_CONGESTION       0x01
#define MQTTSN_RC_REJECTED_INVALID_TOPIC_ID 0x02
#define MQTTSN_RC_REJECTED_NOT_SUPPORTED    0x03

/*=================================
 *    Starting prompt
 ==================================*/
#define TESTER_VERSION " * Version: 2.1.0"

#define PAHO_COPYRIGHT0 " * "
#define PAHO_COPYRIGHT1 " * Part of Project Paho in Eclipse"
#define PAHO_COPYRIGHT2 " * (https://github.com/eclipse/paho.mqtt-sn.embedded-c.git)"
#define PAHO_COPYRIGHT3 " * Author : Tomoaki YAMAGUCHI"
#define PAHO_COPYRIGHT4 " ***************************************************************************"

#endif /* MQTTSNCLIENTAPP_H_ */
