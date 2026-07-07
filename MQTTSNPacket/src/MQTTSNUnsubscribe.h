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

#if !defined(MQTTSNUNSUBSCRIBE_H_)
#define MQTTSNUNSUBSCRIBE_H_

int MQTTSNSerialize_unsubscribe(unsigned char* buf, int buflen,
		unsigned short packetid, MQTTSN_topicid* topicFilter);
int MQTTSNDeserialize_unsubscribe(unsigned short* packetid, MQTTSN_topicid* topicFilter,
		unsigned char* buf, int buflen);

int MQTTSNSerialize_unsuback(unsigned char* buf, int buflen, unsigned short packetid);
int MQTTSNDeserialize_unsuback(unsigned short* packetid, unsigned char* buf, int buflen);

#endif /* MQTTSNUNSUBSCRIBE_H_ */
