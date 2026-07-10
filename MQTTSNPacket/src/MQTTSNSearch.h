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

#if !defined(MQTTSNSEARCH_H_)
#define MQTTSNSEARCH_H_

int MQTTSNSerialize_advertise(unsigned char* buf, int buflen, unsigned char gatewayid, unsigned short duration);
int MQTTSNDeserialize_advertise(unsigned char* gatewayid, unsigned short* duration, unsigned char* buf, int buflen);

int MQTTSNSerialize_searchgw(unsigned char* buf, int buflen, unsigned char radius);
int MQTTSNDeserialize_searchgw(unsigned char* radius, unsigned char* buf, int buflen);

int MQTTSNSerialize_gwinfo(unsigned char* buf, int buflen, unsigned char gatewayid, unsigned short gatewayaddress_len,
		unsigned char* gatewayaddress);
int MQTTSNDeserialize_gwinfo(unsigned char* gatewayid, unsigned short* gatewayaddress_len, unsigned char** gatewayaddress,
		unsigned char* buf, int buflen);

#endif /* MQTTSNSEARCH_H_ */
