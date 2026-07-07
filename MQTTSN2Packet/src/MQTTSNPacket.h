/*******************************************************************************
 * Copyright (c) 2014, 2026 Ian Craggs, IBM Corp. and others
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial implementation based on the MQTT-SN 1.0 code
 *******************************************************************************/

#if !defined(MQTTSNPACKET_H_)
#define MQTTSNPACKET_H_

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

enum errors
{
	MQTTSNPACKET_INVALID_TOPIC_TYPE = -4,
	MQTTSNPACKET_INVALID_QOS = -3,
	MQTTSNPACKET_BUFFER_TOO_SHORT = -2,
	MQTTSNPACKET_READ_ERROR = -1,
	MQTTSNPACKET_READ_COMPLETE = 1
};

#define MQTTSN_PROTOCOL_VERSION 0x02

enum MQTTSN_reasonCodes
{
	/* Success codes (< 0x80) */
	MQTT_SN_RC_SUCCESS                      = 0x00,
	MQTT_SN_RC_NORMAL_DISCONNECT            = 0x00,
	MQTT_SN_RC_GRANTED_QOS_0                = 0x00,
	MQTT_SN_RC_GRANTED_QOS_1                = 0x01,
	MQTT_SN_RC_GRANTED_QOS_2                = 0x02,
	MQTT_SN_RC_DISCONNECT_WITH_WILL         = 0x04,
	MQTT_SN_RC_NO_MATCHING_SUBSCRIBERS      = 0x10,
	MQTT_SN_RC_NO_SUBSCRIPTION_EXISTED      = 0x11,
	MQTT_SN_RC_CONTINUE_AUTH                = 0x18,
	MQTT_SN_RC_RE_AUTHENTICATE              = 0x19,
	MQTT_SN_RC_TOPIC_ALIAS_EXISTS           = 0x1A,
	/* Error codes (>= 0x80) */
	MQTT_SN_RC_UNSPECIFIED_ERROR            = 0x80,
	MQTT_SN_RC_MALFORMED_PACKET             = 0x81,
	MQTT_SN_RC_PROTOCOL_ERROR               = 0x82,
	MQTT_SN_RC_IMPLEMENTATION_ERROR         = 0x83,
	MQTT_SN_RC_UNSUPPORTED_PROTOCOL         = 0x84,
	MQTT_SN_RC_CLIENT_ID_INVALID            = 0x85,
	MQTT_SN_RC_BAD_USER_PASSWORD            = 0x86,
	MQTT_SN_RC_NOT_AUTHORIZED               = 0x87,
	MQTT_SN_RC_SERVER_UNAVAILABLE           = 0x88,
	MQTT_SN_RC_SERVER_BUSY                  = 0x89,
	MQTT_SN_RC_BANNED                       = 0x8A,
	MQTT_SN_RC_SERVER_SHUTTING_DOWN         = 0x8B,
	MQTT_SN_RC_BAD_AUTH_METHOD              = 0x8C,
	MQTT_SN_RC_KEEP_ALIVE_TIMEOUT           = 0x8D,
	MQTT_SN_RC_SESSION_TAKEN_OVER           = 0x8E,
	MQTT_SN_RC_TOPIC_FILTER_INVALID         = 0x8F,
	MQTT_SN_RC_TOPIC_NAME_INVALID           = 0x90,
	MQTT_SN_RC_PACKET_ID_IN_USE             = 0x91,
	MQTT_SN_RC_PACKET_ID_NOT_FOUND          = 0x92,
	MQTT_SN_RC_RECEIVE_MAX_EXCEEDED         = 0x93,
	MQTT_SN_RC_TOPIC_ALIAS_INVALID          = 0x94,
	MQTT_SN_RC_PACKET_TOO_LARGE             = 0x95,
	MQTT_SN_RC_PACKET_RATE_TOO_HIGH         = 0x96,
	MQTT_SN_RC_QUOTA_EXCEEDED               = 0x97,
	MQTT_SN_RC_ADMINISTRATIVE_ACTION        = 0x98,
	MQTT_SN_RC_PAYLOAD_FORMAT_INVALID       = 0x99,
	MQTT_SN_RC_RETAIN_NOT_SUPPORTED         = 0x9A,
	MQTT_SN_RC_QOS_NOT_SUPPORTED            = 0x9B,
	MQTT_SN_RC_USE_ANOTHER_SERVER           = 0x9C,
	MQTT_SN_RC_SERVER_MOVED                 = 0x9D,
	MQTT_SN_RC_SHARED_SUB_NOT_SUPPORTED     = 0x9E,
	MQTT_SN_RC_CONNECTION_RATE_EXCEEDED     = 0x9F,
	MQTT_SN_RC_MAX_CONNECT_TIME             = 0xAD,
	MQTT_SN_RC_SUB_ID_NOT_SUPPORTED         = 0xA1,
	MQTT_SN_RC_WILDCARD_SUB_NOT_SUPPORTED   = 0xA2,
	MQTT_SN_RC_ONLY_PROTECTION_SUPPORTED    = 0xE6,
	MQTT_SN_RC_PROTECTION_SCHEME_INVALID    = 0xE7,
	MQTT_SN_RC_UNKNOWN_SENDER_ID            = 0xE8,
	MQTT_SN_RC_UNKNOWN_TOPIC_ALIAS          = 0xF0,
	MQTT_SN_RC_CONGESTION                   = 0xF1,
	MQTT_SN_RC_PROTECTION_NOT_SUPPORTED     = 0xF2,
	MQTT_SN_RC_FORWARDER_NOT_SUPPORTED      = 0xF3,
	MQTT_SN_RC_NO_VIRTUAL_CONNECTION        = 0xF4
};

typedef struct MQTTSN_string MQTTSN_string;
/**
  * A structure for length-delimited strings which are used in MQTT-SN
  * rather than the C standard null-delimited.
  */
struct MQTTSN_string
{
	bool islen8; /**< flag to indicate whether the length is one byte */
	uint16_t len;
	char* data;
};

#define MQTTSNString_initializer {0, {0, NULL}}

typedef enum MQTTSN_topicTypes
{
	MQTTSN_TOPIC_TYPE_SESSION = 0b00,
	MQTTSN_TOPIC_TYPE_PREDEFINED = 0b01,
	MQTTSN_TOPIC_TYPE_RESERVED = 0b10,
	MQTTSN_TOPIC_TYPE_NAME = 0b11,
	MQTTSN_TOPIC_TYPE_FILTER = 0b11
} MQTTSN_topicTypes;

typedef struct MQTTSN_topic MQTTSN_topic;
struct MQTTSN_topic
{
	MQTTSN_topicTypes type;
	union
	{
		uint16_t alias; /**< session or predefined */
		MQTTSN_string string; /**< topic name or filter */
	} alt;
};

enum MQTTSN_msgTypes
{
	MQTTSN_CONNECT = 0x01,
	MQTTSN_CONNACK,
	MQTTSN_PUBLISH, MQTTSN_PUBACK, MQTTSN_PUBREC, MQTTSN_PUBREL, MQTTSN_PUBCOMP,
	MQTTSN_SUBSCRIBE, MQTTSN_SUBACK, MQTTSN_UNSUBSCRIBE, MQTTSN_UNSUBACK, 
	MQTTSN_PINGREQ, MQTTSN_PINGRESP,
	MQTTSN_DISCONNECT,
	MQTTSN_AUTH, MQTTSN_REGISTER, MQTTSN_REGACK, MQTTSN_PUBWOS,
	MQTTSN_SLEEPREQ, MQTTSN_SLEEPRESP, MQTTSN_WAKEUP,
	MQTTSN_ADVERTISE, MQTTSN_SEARCHGW, MQTTSN_GWINFO,
	MQTTSN_FORWARDER = 0xFD, MQTTSN_SESSION, MQTTSN_PROTECTION
};

typedef struct MQTTSN_data MQTTSN_data;
/**
  * A structure for length-delimited binary data fields
  */
struct MQTTSN_data
{
	uint16_t len;
	unsigned char* data;
};

#include "MQTTSNConnect.h"
#include "MQTTSNPublish.h"
#if 0
#include "MQTTSNSubscribe.h"
#include "MQTTSNUnsubscribe.h"
#include "MQTTSNSearch.h"
#endif

const char* MQTTSNPacket_name(uint8_t ptype);
uint16_t MQTTSNPacket_len(uint16_t length);

int8_t MQTTSNPacket_encode(unsigned char* buf, uint16_t length);
int MQTTSNPacket_decode(unsigned char* buf, int buflen, int* value);

uint16_t readInt16(unsigned char** pptr);
char readChar(unsigned char** pptr);
void writeChar(unsigned char** pptr, char c);
void writeInt16(unsigned char** pptr, uint16_t anInt);
void writeInt32(unsigned char** pptr, uint32_t anInt);
int readMQTTSNString(MQTTSN_string* mqttstring, unsigned char** pptr, unsigned char* enddata);
void writeMQTTSNString(unsigned char** pptr, MQTTSN_string mqttstring, bool withLength);
void writeMQTTSNData(unsigned char** pptr, MQTTSN_data data, bool withLength);

int MQTTSNPacket_read(unsigned char* buf, int buflen, int (*getfn)(unsigned char*, int));
int MQTTSNPacket_read_nb(unsigned char* buf, int buflen);

/* Returns non-zero if at least n bytes remain between cur and end.
 * Used by all deserialize functions to guard every read against overrun.
 */
static inline int buf_avail(const uint8_t *cur, const uint8_t *end, int32_t n)
{
    return n >= 0 && (int32_t)(end - cur) >= n;
}


#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
}
#endif


#endif /* MQTTSNPACKET_H_ */
