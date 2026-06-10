/*******************************************************************************
 * Copyright (c) 2014, 2025 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

#if !defined(MQTTSNPUBLISH_H_)
#define MQTTSNPUBLISH_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_topic, MQTTSN_string, MQTTSN_reasonCodes */

/*
 * PUBLISH (Sections 3.6.2, 3.6.3)
 *
 * dup      - DUP flag; meaningful for QoS 1/2 retransmissions only
 * qos      - QoS level (0, 1, or 2); no Packet Identifier is written for QoS 0
 * retained - Retain flag
 * packetid - Packet Identifier (ignored and not written for QoS 0)
 * topic    - topic type plus alias (alt.alias) or name string (alt.string)
 * payload  - application payload
 */
int32_t MQTTSNSerialize_publish(uint8_t* buf, int32_t buflen,
        uint8_t dup, int32_t qos, uint8_t retained, uint16_t packetid,
        const MQTTSN_topic* topic, uint8_t* payload, int32_t payloadlen);

int32_t MQTTSNDeserialize_publish(uint8_t* dup, int32_t* qos, uint8_t* retained,
        uint16_t* packetid, MQTTSN_topic* topic,
        uint8_t** payload, int32_t* payloadlen,
        uint8_t* buf, int32_t buflen);

/*
 * PUBACK (Section 3.6.4)
 *
 * topicid removed vs MQTT-SN 1.2.
 * returncode is optional on the wire: omitted when MQTT_SN_RC_SUCCESS.
 */
int32_t MQTTSNSerialize_puback(uint8_t* buf, int32_t buflen,
        uint16_t packetid, uint8_t returncode);

int32_t MQTTSNDeserialize_puback(uint16_t* packetid, uint8_t* returncode,
        uint8_t* buf, int32_t buflen);

/*
 * PUBREC / PUBREL / PUBCOMP (Sections 3.6.5, 3.6.6, 3.6.7)
 *
 * returncode added vs MQTT-SN 1.2.
 * Omitted from the wire when MQTT_SN_RC_SUCCESS.
 */
int32_t MQTTSNSerialize_pubrec(uint8_t* buf, int32_t buflen,
        uint16_t packetid, uint8_t returncode);

int32_t MQTTSNSerialize_pubrel(uint8_t* buf, int32_t buflen,
        uint16_t packetid, uint8_t returncode);

int32_t MQTTSNSerialize_pubcomp(uint8_t* buf, int32_t buflen,
        uint16_t packetid, uint8_t returncode);

int32_t MQTTSNDeserialize_ack(uint8_t* packettype, uint16_t* packetid,
        uint8_t* returncode, uint8_t* buf, int32_t buflen);

/*
 * REGISTER (Section 3.4)
 *
 * topic_alias_present - 1 = include a Topic Alias field, 0 = omit it;
 *                       MUST be 0 when sent by a Client (Section 3.4.2.1)
 * topicid             - Topic Alias to assign (Server → Client only)
 * topicname           - the topic name (fills to end of packet)
 */
int32_t MQTTSNSerialize_register(uint8_t* buf, int32_t buflen,
        uint8_t topic_alias_present, uint16_t topicid, uint16_t packetid,
        MQTTSN_string* topicname);

int32_t MQTTSNDeserialize_register(uint8_t* topic_alias_present, uint16_t* topicid,
        uint16_t* packetid, MQTTSN_string* topicname,
        uint8_t* buf, int32_t buflen);

/*
 * REGACK (Section 3.5)
 *
 * topic_type          - Topic Type in flags bits 0-1; MUST be SESSION or PREDEFINED
 * topic_alias_present - 1 = include a Topic Alias field, 0 = omit it
 * topicid             - Topic Alias assigned by the Server
 * returncode          - Reason Code; omitted from wire when MQTT_SN_RC_SUCCESS
 *                       and no topic alias is present (Section 3.5.5)
 */
int32_t MQTTSNSerialize_regack(uint8_t* buf, int32_t buflen,
        uint8_t topic_type, uint8_t topic_alias_present,
        uint16_t topicid, uint16_t packetid, uint8_t returncode);

int32_t MQTTSNDeserialize_regack(uint8_t* topic_type, uint8_t* topic_alias_present,
        uint16_t* topicid, uint16_t* packetid, uint8_t* returncode,
        uint8_t* buf, int32_t buflen);

#endif /* MQTTSNPUBLISH_H_ */
