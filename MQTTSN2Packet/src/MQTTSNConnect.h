/*******************************************************************************
 * Copyright (c) 2014, 2026 Ian Craggs, IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef MQTTSNCONNECT_H_
#define MQTTSNCONNECT_H_

/* The CONNECT, CONNACK, PING and DISCONNECT header file for MQTTSN 2.0 */

#include "MQTTSNPacket.h"

typedef union MQTTSNPacket_connectFlags MQTTSNPacket_connectFlags;
union MQTTSNPacket_connectFlags
{
	uint8_t all;
#if defined(REVERSED)
	struct connectFlagsBits
	{
		bool reserved : 1;
		bool allowServerSuggestions : 1;
		bool allowAddressChanges : 1;
		bool defaultAwakeMessages: 1;
		bool sessionExpiryInterval : 1;
		bool auth : 1;
		bool will : 1;
		bool cleanStart : 1;
	} bits;
#else
	struct connectFlagsBits
	{
		bool cleanStart : 1;
		bool will : 1;
		bool auth : 1;
		bool sessionExpiryInterval : 1;
		bool defaultAwakeMessages: 1;
		bool allowAddressChanges : 1;
		bool allowServerSuggestions : 1;
		bool reserved : 1;
	} bits;
#endif
};

typedef union MQTTSNPacket_willFlags MQTTSNPacket_willFlags;
union MQTTSNPacket_willFlags
{
	uint8_t all;
#if defined(REVERSED)
	struct bits
	{
		uint8_t reserved : 3;
		bool retain : 1;
		uint8_t QoS : 2;
		uint8_t topicType : 2;
	} bits;
#else
	struct willFlagsBits
	{
		uint8_t topicType : 2;
		uint8_t QoS : 2;
		bool retain : 1;
		uint8_t reserved : 3;
	} bits;
#endif
} MQTTSN_willFlags;

typedef struct MQTTSNPacket_connectData MQTTSNPacket_connectData;
struct MQTTSNPacket_connectData
{
	MQTTSNPacket_connectFlags flags;
	MQTTSNPacket_willFlags willFlags;
	uint16_t packetId;
	uint8_t protocolVersion;
	uint16_t keepAlive;
	uint16_t maxPacketSize;
	uint8_t defaultAwakeMessages;
	uint32_t sessionExpiryInterval;
	struct will {
		MQTTSN_topic topic;
		MQTTSN_data payload;
	} will;
	struct auth {
		MQTTSN_string method; /**< single byte length */
		MQTTSN_data data;
	} auth;
	MQTTSN_string clientID;
};
#define MQTTSNPacket_connectData_initializer { 0, 0, 0, 2, 60, 100, 5, 0, \
	{ {MQTTSN_TOPIC_TYPE_NAME, {0}}, {0, NULL} }, { {1, 0, NULL}, {0, NULL} }, {0, 0, NULL}}

typedef union MQTTSNPacket_connackFlags MQTTSNPacket_connackFlags;
union MQTTSNPacket_connackFlags
{
	uint8_t all;
#if defined(REVERSED)
	struct connackFlagsBits
	{
		uint8_t reserved : 4;
		bool auth : 1;
		bool serverKeepAlive : 1;
		bool sessionExpiryInterval : 1;
		bool sessionPresent : 1;
	} bits;
#else
	struct connackFlagsBits
	{
		bool sessionPresent : 1;
		bool sessionExpiryInterval : 1;
		bool serverKeepAlive : 1;
		bool auth : 1;
		uint8_t reserved : 4;
	} bits;
#endif
} MQTTSN_connackFlags;

typedef struct MQTTSNPacket_connackData MQTTSNPacket_connackData;
struct MQTTSNPacket_connackData
{
	MQTTSNPacket_connackFlags flags;
	uint16_t packetId;
	uint8_t reasonCode;
	/* optional fields follow */
	uint32_t sessionExpiryInterval;
	uint16_t serverKeepAlive;
	struct auth auth;
	MQTTSN_string assignedClientID;
} MQTTSN_connackData;

int MQTTSNSerialize_connect(unsigned char* buf, int buflen, MQTTSNPacket_connectData* options);
int MQTTSNDeserialize_connect(MQTTSNPacket_connectData* data, unsigned char* buf, int len);

int MQTTSNSerialize_connack(unsigned char* buf, int buflen, int connack_rc);
int MQTTSNDeserialize_connack(MQTTSNPacket_connackData* data, unsigned char* buf, int buflen);

int MQTTSNSerialize_disconnect(unsigned char* buf, int buflen, int duration);
int MQTTSNDeserialize_disconnect(int* duration, unsigned char* buf, int buflen);

int MQTTSNSerialize_pingreq(unsigned char* buf, int buflen, MQTTSN_string clientid);
int MQTTSNDeserialize_pingreq(MQTTSN_string* clientID, unsigned char* buf, int len);

int MQTTSNSerialize_pingresp(unsigned char* buf, int buflen);
int MQTTSNDeserialize_pingresp(unsigned char* buf, int buflen);

#endif /* MQTTSNCONNECT_H_ */
