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
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *
 * Description:
 * Short topic name used to avoid registration process
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "MQTTSNPacket.h"
#include "lowlevel.h"


int main(int argc, char** argv)
{
	int rc = 0;
	int mysock;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	MQTTSN_topicid topic;
	unsigned char* payload = (unsigned char*)"mypayload";
	int payloadlen = strlen((char*)payload);
	int len = 0;
	int dup = 0;
	int qos = 1;
	int retained = 0;
	short packetid = 1;
	char *host = "127.0.0.1";
	int port = 1883;
	MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;

	mysock = lowlevel_open();
	if(mysock < 0)
		return mysock;

	if (argc > 1)
		host = argv[1];

	if (argc > 2)
		port = atoi(argv[2]);

	printf("Sending to hostname %s port %d\n", host, port);

	options.clientID.cstring = "myclientid";
	len = MQTTSNSerialize_connect(buf, buflen, &options);
	rc = lowlevel_sendPacketBuffer(host, port, buf, len);

	/* wait for connack */
	if (MQTTSNPacket_read(buf, buflen, lowlevel_getdata) == MQTTSN_CONNACK)
	{
		int connack_rc = -1;

		if (MQTTSNDeserialize_connack(&connack_rc, buf, buflen) != 1 || connack_rc != 0)
		{
			printf("Unable to connect, return code %d\n", connack_rc);
			goto exit;
		}
		else 
			printf("connected rc %d\n", connack_rc);
	}
	else
		goto exit;

	/* publish with short name */
	topic.type = MQTTSN_TOPIC_TYPE_SHORT;
	memcpy(topic.data.short_name, "tt", 2);
	len = MQTTSNSerialize_publish(buf, buflen - len, dup, qos, retained, packetid,
			topic, payload, payloadlen);
	rc = lowlevel_sendPacketBuffer(host, port, buf, len);

	/* wait for puback */
	if (MQTTSNPacket_read(buf, buflen, lowlevel_getdata) == MQTTSN_PUBACK)
	{
		unsigned short packet_id, topic_id;
		unsigned char returncode;

		if (MQTTSNDeserialize_puback(&topic_id, &packet_id, &returncode, buf, buflen) != 1 || returncode != MQTTSN_RC_ACCEPTED)
			printf("Unable to publish, return code %d\n", returncode);
		else 
			printf("puback received, id %d\n", packet_id);
	}
	else
		goto exit;

	len = MQTTSNSerialize_disconnect(buf, buflen, 0);
	rc = lowlevel_sendPacketBuffer(host, port, buf, len);

exit:
	lowlevel_close();

	return 0;
}
