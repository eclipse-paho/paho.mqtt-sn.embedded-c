/*******************************************************************************
 * Copyright (c) 2014, 2026 IBM Corp., Ian Craggs
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
 *******************************************************************************/

#if !defined(MQTTSNPUBLISH_H_)
#define MQTTSNPUBLISH_H_

int MQTTSNSerialize_publish(unsigned char* buf, int buflen, unsigned char dup, int qos, unsigned char retained, unsigned short packetid,
		MQTTSN_topicid topic, unsigned char* payload, int payloadlen);
int MQTTSNDeserialize_publish(unsigned char* dup, int* qos, unsigned char* retained, unsigned short* packetid,
		MQTTSN_topicid* topic, unsigned char** payload, int* payloadlen, unsigned char* buf, int len);

int MQTTSNSerialize_puback(unsigned char* buf, int buflen, unsigned short topicid, unsigned short packetid,
		unsigned char returncode);
int MQTTSNDeserialize_puback(unsigned short* topicid, unsigned short* packetid,
		unsigned char* returncode, unsigned char* buf, int buflen);

int MQTTSNSerialize_pubrec(unsigned char* buf, int buflen, unsigned short packetid);
int MQTTSNSerialize_pubcomp(unsigned char* buf, int buflen, unsigned short packetid);

int MQTTSNDeserialize_ack(unsigned char* packettype, unsigned short* packetid, unsigned char* buf, int buflen);

int MQTTSNSerialize_register(unsigned char* buf, int buflen, unsigned short topicid, unsigned short packetid,
		MQTTSNString* topicname);
int MQTTSNDeserialize_register(unsigned short* topicid, unsigned short* packetid, MQTTSNString* topicname,
		unsigned char* buf, int buflen);

int MQTTSNSerialize_regack(unsigned char* buf, int buflen, unsigned short topicid, unsigned short packetid,
		unsigned char return_code);
int MQTTSNDeserialize_regack(unsigned short* topicid, unsigned short* packetid, unsigned char* return_code,
		unsigned char* buf, int buflen);
int MQTTSNSerialize_pubrel(unsigned char* buf, int buflen, unsigned short packetid);

#endif /* MQTTSNPUBLISH_H_ */
