/*******************************************************************************
 * Copyright (c) 2014, 2025 Ian Craggs, IBM Corp.
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
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "MQTTSNPacket.h"
#include "transport.h"


int main(int argc, char** argv)
{
	int rc = 0;
	int mysock;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	MQTTSN_topic topic;
	unsigned char* payload = (unsigned char*)"my payload";
	int payloadlen = strlen((char*)payload);
	int len = 0;
	int dup = 0;
	int qos = 0;
	int retained = 0;
	short packetid = 0;
//	char *topicname = "a long topic name";
	char *host = "127.0.0.1";
	int port = 1883;
	MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;

	mysock = transport_open();
	if (mysock < 0)
		return mysock;

	if (argc > 1)
		host = argv[1];

	if (argc > 2)
		port = atoi(argv[2]);

	printf("Sending to hostname %s port %d\n", host, port);

	options.packetId = 45;
	options.flags.bits.cleanStart = 1;
	options.clientID.data = "myclientid";
	options.clientID.len = strlen(options.clientID.data);

	len = MQTTSNSerialize_connect(buf, buflen, &options);
	printf("MQTTSNSerialize_connect len %d\n", len);
	rc = transport_sendPacketBuffer(host, port, buf, len);

	/* wait for connack */
	if (MQTTSNPacket_read(buf, buflen, transport_getdata) == MQTTSN_CONNACK)
	{
		MQTTSNPacket_connackData data;

		if (MQTTSNDeserialize_connack(&data, buf, buflen) != 1 || data.reasonCode != 0)
		{
			printf("Unable to connect, return code %d\n", data.reasonCode);
			goto exit;
		}
		else
			printf("connected rc %d, packet id %d\n", data.reasonCode, data.packetId);
	}
	else
		goto exit;

	 
	/* publish with short name */
	topic.type = MQTTSN_TOPIC_TYPE_NAME;
	topic.alt.string.data = "tt";
	topic.alt.string.len = strlen(topic.alt.string.data);

	len = MQTTSNSerialize_publish(buf, buflen, dup, qos, retained, packetid,
			&topic, payload, payloadlen);
	rc = transport_sendPacketBuffer(host, port, buf, len);

	printf("rc %d from send packet for publish length %d\n", rc, len);

exit:
	transport_close();

	return 0;
}
