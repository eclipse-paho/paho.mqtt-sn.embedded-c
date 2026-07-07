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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#ifndef MQTTSNGWDEFINES_H_
#define MQTTSNGWDEFINES_H_

namespace MQTTSNGW
{
/*=================================
 *    Config Parametrs
 ==================================*/
#define CONFIG_DIRECTORY "./"
#define CONFIG_FILE      "gateway.conf"
#define CLIENT_LIST      "clients.conf"
#define PREDEFINEDTOPIC_FILE      "predefinedTopic.conf"
#define FORWARDER_LIST     "forwarders.conf"

/*==========================================================
 *    Gateway default parameters
 ===========================================================*/
#define DEFAULT_KEEP_ALIVE_TIME     (900)  // 900 secs = 15 mins
#define DEFAULT_MQTT_VERSION          (4)  // Defualt MQTT version

/*=================================
 *    MQTT-SN Parametrs
 ==================================*/
#define MAX_CLIENTS                 (100)  // Default number of Clients can be handled.
#define MAX_CLIENTID_LENGTH          (64)  // Max length of clientID
#define MAX_INFLIGHTMESSAGES         (10)  // Number of inflight messages
#define MAX_MESSAGEID_TABLE_SIZE    (500)  // Number of MessageIdTable size
#define MAX_SAVED_PUBLISH            (20)  // Max number of PUBLISH message for Asleep state
#define MAX_TOPIC_PAR_CLIENT         (50)  // Max Topic count for a client. it should be less than 256
#define MQTTSNGW_MAX_PACKET_SIZE   (1024)  // Max Packet size  (5+2+TopicLen+PayloadLen + Foward Encapsulation)
#define SIZE_OF_LOG_PACKET          (500)  // Length of the packet log in bytes

#define PROXY_KEEPALIVE_DURATION   (900)   // Seconds
#define PROXY_RESPONSE_DURATION     (10)   // Seconds
#define PROXY_MAX_RETRY_CNT          (3)

/*=================================
 *    Data Type
 ==================================*/
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/*=================================
 *    Log controls
 ==================================*/
//#define DEBUG_MQTTSN   // print out log for debug
//#define DEBUG_NW       // print out SensorNetwork log
#ifdef  DEBUG_MQTTSN
#define DEBUGLOG(...) printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif

#ifdef  DEBUG_NW
#define D_NWSTACK(...) printf(__VA_ARGS__)
#else
#define D_NWSTACK(...)
#endif

}
#endif /* MQTTSNGWDEFINES_H_ */
