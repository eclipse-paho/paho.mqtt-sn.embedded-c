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
 * Normal topic name is automatically registered at subscription, then
 * a message is published and the node receives it itself
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "MQTTSNPacket.h"
#include "rfcomm.h"


int main(int argc, char** argv)
{
    int rc = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    MQTTSN_topicid topic;
    unsigned char* payload = (unsigned char*)"mypayload";
    int payloadlen = strlen((char*)payload);
    int len = 0;
    unsigned char dup = 0;
    int qos = 1;
    unsigned char retained = 0;
    short packetid = 1;
    char *topicname = "a long topic name";
    char *host = "";
    int channel = 1;
    MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;
    unsigned short topicid;

    if (argc > 1)
        host = argv[1];

    if (argc > 2)
        channel = atoi(argv[2]);

    printf("Sending to address %s channel %d\n", host, channel);
    if (rfcomm_open(host, channel) < 0)
    {
        goto exit;
    }

    options.clientID.cstring = "pub0sub1 MQTT-SN";
    len = MQTTSNSerialize_connect(buf, buflen, &options);
    rc = rfcomm_sendPacketBuffer(buf, len);

    /* wait for connack */
    if (MQTTSNPacket_read(buf, buflen, rfcomm_getdata) == MQTTSN_CONNACK)
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


    /* subscribe */
    printf("Subscribing\n");
    topic.type = MQTTSN_TOPIC_TYPE_NORMAL;
    topic.data.long_.name = topicname;
    topic.data.long_.len = strlen(topic.data.long_.name);
    len = MQTTSNSerialize_subscribe(buf, buflen, 0, 2, packetid, &topic);
    rc = rfcomm_sendPacketBuffer(buf, len);

    if (MQTTSNPacket_read(buf, buflen, rfcomm_getdata) == MQTTSN_SUBACK) /* wait for suback */
    {
        unsigned short submsgid;
        int granted_qos;
        unsigned char returncode;

        rc = MQTTSNDeserialize_suback(&granted_qos, &topicid, &submsgid, &returncode, buf, buflen);
        if (granted_qos != 2 || returncode != 0)
        {
            printf("granted qos != 2, %d return code %d\n", granted_qos, returncode);
            goto exit;
        }
        else
            printf("suback topic id %d\n", topicid);
    }
    else
        goto exit;

    printf("Publishing\n");
    /* publish with short name */
    topic.type = MQTTSN_TOPIC_TYPE_NORMAL;
    topic.data.id = topicid;
    ++packetid;
    len = MQTTSNSerialize_publish(buf, buflen, dup, qos, retained, packetid,
            topic, payload, payloadlen);
    rc = rfcomm_sendPacketBuffer(buf, len);

    /* wait for puback */
    if (MQTTSNPacket_read(buf, buflen, rfcomm_getdata) == MQTTSN_PUBACK)
    {
        unsigned short packet_id, topic_id;
        unsigned char returncode;

        if (MQTTSNDeserialize_puback(&topic_id, &packet_id, &returncode, buf, buflen) != 1 || returncode != MQTTSN_RC_ACCEPTED)
            printf("Unable to publish, return code %d\n", returncode);
        else 
            printf("puback received, msgid %d topic id %d\n", packet_id, topic_id);
    }
    else
        goto exit;

    printf("Receive publish\n");
    if (MQTTSNPacket_read(buf, buflen, rfcomm_getdata) == MQTTSN_PUBLISH)
    {
        unsigned short packet_id;
        int qos, payloadlen;
        unsigned char* payload;
        unsigned char dup, retained;
        MQTTSN_topicid pubtopic;

        if (MQTTSNDeserialize_publish(&dup, &qos, &retained, &packet_id, &pubtopic,
                &payload, &payloadlen, buf, buflen) != 1)
            printf("Error deserializing publish\n");
        else 
            printf("publish received, id %d qos %d\n", packet_id, qos);

        if (qos == 1)
        {
            len = MQTTSNSerialize_puback(buf, buflen, pubtopic.data.id, packet_id, MQTTSN_RC_ACCEPTED);
            rc = rfcomm_sendPacketBuffer(buf, len);
            if (rc == 0)
                printf("puback sent\n");
        }
    }
    else
        goto exit;

    len = MQTTSNSerialize_disconnect(buf, buflen, 0);
    rc = rfcomm_sendPacketBuffer(buf, len);

exit:
    rfcomm_close();

    return 0;
}
