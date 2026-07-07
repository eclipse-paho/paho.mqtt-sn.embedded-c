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
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *
 * Description:
 * Extension to the specs in which a node can send a normal (long) topic name inside the
 * payload area to avoid the registration process and the usage of short/predefined types
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "MQTTSNPacket.h"
#include "rfcomm.h"


int main(int argc, char** argv)
{
	unsigned char buf[200];
	int buflen = sizeof(buf);
	MQTTSN_topicid topic;
	unsigned char* payload = (unsigned char*)"mypayload";
	int payloadlen = strlen((char*)payload);
	int len = 0;
	int dup = 0;
	int qos = 3;
	int retained = 0;
	short packetid = 0;
	char *topicname = "a long topic name";
    char *host = "";
    int channel = 1;

	if (argc > 1)
		host = argv[1];

	if (argc > 2)
        channel = atoi(argv[2]);

    printf("Sending to Address %s channel %d\n", host, channel);
    if (rfcomm_open(host, channel) < 0)
    {
        return -1;;
    }

	topic.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic.data.long_.name = topicname;
	topic.data.long_.len = strlen(topicname);

	len = MQTTSNSerialize_publish(buf, buflen, dup, qos, retained, packetid,
			topic, payload, payloadlen);

    rfcomm_sendPacketBuffer(buf, len);

    rfcomm_close();

	return 0;
}
