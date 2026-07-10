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
 *******************************************************************************/

#if !defined(MQTTSNSUBSCRIBE_H_)
#define MQTTSNSUBSCRIBE_H_

int MQTTSNSerialize_subscribe(unsigned char* buf, int buflen, unsigned char dup, int qos, unsigned short packetid,
        MQTTSN_topicid* topicFilter);
int MQTTSNDeserialize_subscribe(unsigned char* dup, int* qos, unsigned short* packetid,
        MQTTSN_topicid* topicFilter, unsigned char* buf, int buflen);

int MQTTSNSerialize_suback(unsigned char* buf, int buflen, int qos, unsigned short topicid, unsigned short packetid,
		unsigned char returncode);
int MQTTSNDeserialize_suback(int* qos, unsigned short* topicid, unsigned short* packetid,
		unsigned char* returncode, unsigned char* buf, int buflen);

#endif /* MQTTSNSUBSCRIBE_H_ */
