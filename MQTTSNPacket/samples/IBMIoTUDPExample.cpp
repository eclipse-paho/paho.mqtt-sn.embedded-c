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
 *    Ian Craggs - refactoring to remove STL and other changes
 *******************************************************************************/

#define WARN printf

#include "MQTTClient.h"

#define DEFAULT_STACK_SIZE -1

#include "linux.cpp"

#include <stdlib.h>

// Configuration values needed to connect to IBM IoT Cloud
#define ORG "quickstart"             // For a registered connection, replace with your org
#define ID "8002f7f1ad23"                        // For a registered connection, replace with your id
#define AUTH_TOKEN ""                // For a registered connection, replace with your auth-token
#define TYPE "mytype"       // For a registered connection, replace with your type

#define MQTT_PORT 1883
#define MQTT_TLS_PORT 8883
#define IBM_IOT_PORT MQTT_PORT

#define MQTT_MAX_PACKET_SIZE 250

bool quickstartMode = true;
char org[11] = ORG;  
char type[30] = TYPE;
char id[30] = ID;                 // mac without colons
char auth_token[30] = AUTH_TOKEN; // Auth_token is only used in non-quickstart mode

bool connected = false;
const char* joystickPos = "CENTRE";
int blink_interval = 0;



int connect(MQTT::Client<IPStack, Countdown, MQTT_MAX_PACKET_SIZE>* client, IPStack* ipstack)
{   
    const char* iot_ibm = ".messaging.internetofthings.ibmcloud.com";
    
    char hostname[strlen(org) + strlen(iot_ibm) + 1];
    sprintf(hostname, "%s%s", org, iot_ibm);
	DEBUG("connecting to %s\n", hostname);
    int rc = ipstack->connect(hostname, IBM_IOT_PORT);
    if (rc != 0)
        return rc;
     
    // Construct clientId - d:org:type:id
    char clientId[strlen(org) + strlen(type) + strlen(id) + 5];
    sprintf(clientId, "d:%s:%s:%s", org, type, id);
    DEBUG("clientid is %s\n", clientId);
    
    // MQTT Connect
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 4;
    data.clientID.cstring = clientId;
    
    if (!quickstartMode) 
    {        
        data.username.cstring = "use-token-auth";
        data.password.cstring = auth_token;
    }
    
    if ((rc = client->connect(data)) == 0) 
        connected = true;
    return rc;
}


int getConnTimeout(int attemptNumber)
{  // First 10 attempts try within 3 seconds, next 10 attempts retry after every 1 minute
   // after 20 attempts, retry every 10 minutes
    return (attemptNumber < 10) ? 3 : (attemptNumber < 20) ? 60 : 600;
}


void attemptConnect(MQTT::Client<IPStack, Countdown, MQTT_MAX_PACKET_SIZE>* client, IPStack* ipstack)
{
    int retryAttempt = 0;
    connected = false;
        
    while (connect(client, ipstack) != 0) 
    {    
        int timeout = getConnTimeout(++retryAttempt);
        WARN("Retry attempt number %d waiting %d\n", retryAttempt, timeout);
        
        // if ipstack and client were on the heap we could deconstruct and goto a label where they are constructed
        //  or maybe just add the proper members to do this disconnect and call attemptConnect(...)
        
        sleep(timeout);
    }
}


int publish(MQTT::Client<IPStack, Countdown, MQTT_MAX_PACKET_SIZE>* client, IPStack* ipstack)
{
    MQTT::Message message;
    char* pubTopic = "iot-2/evt/status/fmt/json";
	static const char* joypos[] = {"LEFT", "RIGHT", "CENTRE", "UP", "DOWN"};
            
    char buf[250];
    sprintf(buf,
     "{\"d\":{\"myName\":\"IoT mbed\",\"accelX\":%0.4f,\"accelY\":%0.4f,\"accelZ\":%0.4f,\"temp\":%0.4f,\"joystick\":\"%s\",\"potentiometer1\":%0.4f,\"potentiometer2\":%0.4f}}",
			(rand() % 10) * 2.0, (rand() % 10) * 2.0, (rand() % 10) * 2.0, (rand() % 10) + 18.0, joypos[rand() % 5], (rand() % 10) * 30.0, (rand() % 10) * 30.0); 
            //MMA.x(), MMA.y(), MMA.z(), sensor.temp(), joystickPos, ain1.read(), ain2.read());
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    
    LOG("Publishing %s\n", buf);
    return client->publish(pubTopic, message);
}


void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    char topic[md.topicName.lenstring.len + 1];
    
    sprintf(topic, "%.*s", md.topicName.lenstring.len, md.topicName.lenstring.data);
    
    LOG("Message arrived on topic %s: %.*s\n",  topic, message.payloadlen, message.payload);
          
    // Command topic: iot-2/cmd/blink/fmt/json - cmd is the string between cmd/ and /fmt/
    char* start = strstr(topic, "/cmd/") + 5;
    int len = strstr(topic, "/fmt/") - start;
    
    if (memcmp(start, "blink", len) == 0)
    {
        char payload[message.payloadlen + 1];
        sprintf(payload, "%.*s", message.payloadlen, (char*)message.payload);
    
        char* pos = strchr(payload, '}');
        if (pos != NULL)
        {
            *pos = '\0';
            if ((pos = strchr(payload, ':')) != NULL)
            {
                int blink_rate = atoi(pos + 1);       
                blink_interval = (blink_rate <= 0) ? 0 : (blink_rate > 50 ? 1 : 50/blink_rate);
            }
        }
    }
    else
        WARN("Unsupported command: %.*s\n", len, start);
}


int main()
{    
    quickstartMode = (strcmp(org, "quickstart") == 0);
    
    IPStack ipstack = IPStack();
    MQTT::Client<IPStack, Countdown, MQTT_MAX_PACKET_SIZE> client(ipstack);
    
    attemptConnect(&client, &ipstack);
    
    if (!quickstartMode) 
    {
        int rc = 0;
        if ((rc = client.subscribe("iot-2/cmd/+/fmt/json", MQTT::QOS1, messageArrived)) != 0)
            WARN("rc from MQTT subscribe is %d\n", rc); 
    }
   
    int count = 0;
    while (true)
    {
        if (++count == 100)
        {               // Publish a message every second
            if (publish(&client, &ipstack) != 0) 
                attemptConnect(&client, &ipstack);   // if we have lost the connection
            count = 0;
        }
        client.yield(10);  // allow the MQTT client to receive messages
    }
}

