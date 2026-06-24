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
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

#ifndef MQTTSNCONNECT_H_
#define MQTTSNCONNECT_H_

/* The CONNECT, CONNACK, PING and DISCONNECT header file for MQTT-SN 2.0 */

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
	struct willFlagsBits
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
};

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
#define MQTTSNPacket_connectData_initializer { {0}, {0}, 0, 2, 60, 100, 5, 0, \
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
};

typedef struct MQTTSNPacket_connackData MQTTSNPacket_connackData;
struct MQTTSNPacket_connackData
{
	MQTTSNPacket_connackFlags flags;
	uint16_t packetId;
	uint8_t reasonCode;
	/* optional fields — written/read only when their flag bit is set */
	uint32_t sessionExpiryInterval;
	uint16_t serverKeepAlive;
	struct {
		MQTTSN_string method; /**< single byte length */
		MQTTSN_data   data;
	} auth;
	MQTTSN_string assignedClientID; /**< fills to end; inferred from length */
};

typedef union MQTTSNPacket_disconnectFlags MQTTSNPacket_disconnectFlags;
union MQTTSNPacket_disconnectFlags
{
	uint8_t all;
#if defined(REVERSED)
	struct disconnectFlagsBits
	{
		uint8_t reserved : 5;
		bool reasonCode : 1;
		bool sessionExpiryInterval : 1;
		bool packetId : 1;
	} bits;
#else
	struct disconnectFlagsBits
	{
		bool packetId : 1;              /**< bit 0 */
		bool sessionExpiryInterval : 1; /**< bit 1 */
		bool reasonCode : 1;            /**< bit 2 */
		uint8_t reserved : 5;           /**< bits 7-3: must be 0 */
	} bits;
#endif
};

typedef struct MQTTSNPacket_disconnectData MQTTSNPacket_disconnectData;
struct MQTTSNPacket_disconnectData
{
	MQTTSNPacket_disconnectFlags flags;
	uint16_t packetId;              /**< optional; diagnostic use by Server */
	uint8_t  reasonCode;            /**< optional; 0x00 assumed when absent  */
	uint32_t sessionExpiryInterval; /**< optional; Client only               */
	MQTTSN_string reasonString;     /**< optional; fills to end of packet    */
};

/*
 * Client transmit / Server receive
 */
int32_t MQTTSNSerialize_connect(uint8_t* buf, int32_t buflen,
        const MQTTSNPacket_connectData* options);

/*
 * Server receive / Client transmit (server-side deserialize)
 */
int32_t MQTTSNDeserialize_connect(MQTTSNPacket_connectData* data,
        uint8_t* buf, int32_t buflen);

/*
 * Server transmit / Client receive
 */
int32_t MQTTSNSerialize_connack(uint8_t* buf, int32_t buflen,
        MQTTSNPacket_connackData* data);

/*
 * Client receive / Server transmit (client-side deserialize)
 */
int32_t MQTTSNDeserialize_connack(MQTTSNPacket_connackData* data,
        uint8_t* buf, int32_t buflen);

/*
 * Either side transmit / receive
 */
int32_t MQTTSNSerialize_disconnect(uint8_t* buf, int32_t buflen,
        uint8_t reason_code);

int32_t MQTTSNDeserialize_disconnect(MQTTSNPacket_disconnectData* data,
        uint8_t* buf, int32_t buflen);

/*
 * Client transmit / Server receive
 */
int32_t MQTTSNSerialize_pingreq(uint8_t* buf, int32_t buflen,
        MQTTSN_string clientid);

/*
 * Server receive / Client transmit (server-side deserialize)
 */
int32_t MQTTSNDeserialize_pingreq(MQTTSN_string* clientID,
        uint8_t* buf, int32_t buflen);

/*
 * Server transmit / Client receive
 */
int32_t MQTTSNSerialize_pingresp(uint8_t* buf, int32_t buflen);

/*
 * Client receive / Server transmit (client-side deserialize)
 */
int32_t MQTTSNDeserialize_pingresp(uint8_t* buf, int32_t buflen);

#endif /* MQTTSNCONNECT_H_ */
