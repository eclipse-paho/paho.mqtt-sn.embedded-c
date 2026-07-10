/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    TomoakiiYamaguchi - modify for C++
 *******************************************************************************/

#ifndef MQTTSNPACKET_H_
#define MQTTSNPACKET_H_

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

enum errors
{
	MQTTSNPACKET_BUFFER_TOO_SHORT = -2,
	MQTTSNPACKET_READ_ERROR = -1,
	MQTTSNPACKET_READ_COMPLETE
};

#define MQTTSN_PROTOCOL_VERSION 0x01

enum MQTTSN_connackCodes
{
	MQTTSN_RC_ACCEPTED,
	MQTTSN_RC_REJECTED_CONGESTED,
	MQTTSN_RC_REJECTED_INVALID_TOPIC_ID,
	MQTTSN_RC_NOT_SUPPORTED
};

typedef enum
{
	MQTTSN_TOPIC_TYPE_NORMAL, /* topic id in publish, topic name in subscribe */
	MQTTSN_TOPIC_TYPE_PREDEFINED,
	MQTTSN_TOPIC_TYPE_SHORT
}MQTTSN_topicTypes;

enum MQTTSN_msgTypes
{
	MQTTSN_ADVERTISE, MQTTSN_SEARCHGW, MQTTSN_GWINFO, MQTTSN_RESERVED1,
	MQTTSN_CONNECT, MQTTSN_CONNACK,
	MQTTSN_WILLTOPICREQ, MQTTSN_WILLTOPIC, MQTTSN_WILLMSGREQ, MQTTSN_WILLMSG, 
	MQTTSN_REGISTER, MQTTSN_REGACK,
	MQTTSN_PUBLISH, MQTTSN_PUBACK, MQTTSN_PUBCOMP, MQTTSN_PUBREC, MQTTSN_PUBREL, MQTTSN_RESERVED2,
	MQTTSN_SUBSCRIBE, MQTTSN_SUBACK, MQTTSN_UNSUBSCRIBE, MQTTSN_UNSUBACK, 
	MQTTSN_PINGREQ, MQTTSN_PINGRESP,
	MQTTSN_DISCONNECT, MQTTSN_RESERVED3, 
	MQTTSN_WILLTOPICUPD, MQTTSN_WILLTOPICRESP, MQTTSN_WILLMSGUPD, MQTTSN_WILLMSGRESP,
	MQTTSN_ENCAPSULATED = 0xfe
};


typedef struct
{
	MQTTSN_topicTypes type;
	union
	{
		unsigned short id;
		char short_name[2];
		struct
		{
			char* name;
			int len;
		} long_;
	} data;
} MQTTSN_topicid;

/**
 * Bitfields for the MQTT-SN flags byte.
 */
typedef union
{
	unsigned char all;
#if defined(REVERSED)
	struct
	{
		int dup: 1;
		unsigned int QoS : 2;
		unsigned int retain : 1;
		unsigned int will : 1;
		unsigned int cleanSession : 1;
		unsigned int topicIdType : 2;
	} bits;
#else
	struct
	{
		unsigned int topicIdType : 2;
		unsigned int cleanSession : 1;
		unsigned int will : 1;
		unsigned int retain : 1;
		unsigned int QoS : 2;
		int dup: 1;
	} bits;
#endif
} MQTTSNFlags;


typedef struct
{
	int len;
	char* data;
} MQTTSNLenString;

typedef struct
{
	char* cstring;
	MQTTSNLenString lenstring;
} MQTTSNString;

#define MQTTSNString_initializer {NULL, {0, NULL}}

int MQTTSNstrlen(MQTTSNString mqttsnstring);

#include "MQTTSNConnect.h"
#include "MQTTSNPublish.h"
#include "MQTTSNSubscribe.h"
#include "MQTTSNUnsubscribe.h"
#include "MQTTSNSearch.h"

const char* MQTTSNPacket_name(int ptype);
int MQTTSNPacket_len(int length);

int MQTTSNPacket_encode(unsigned char* buf, int length);
int MQTTSNPacket_decode(unsigned char* buf, int buflen, int* value);

int readInt(unsigned char** pptr);
char readChar(unsigned char** pptr);
void writeChar(unsigned char** pptr, char c);
void writeInt(unsigned char** pptr, int anInt);
int readMQTTSNString(MQTTSNString* mqttstring, unsigned char** pptr, unsigned char* enddata);
void writeCString(unsigned char** pptr, char* string);
void writeMQTTSNString(unsigned char** pptr, MQTTSNString mqttstring);

int MQTTSNPacket_read(unsigned char* buf, int buflen, int (*getfn)(unsigned char*, int));
int MQTTSNPacket_read_nb(unsigned char* buf, int buflen);


#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
}
#endif


#endif /* MQTTSNPACKET_H_ */
