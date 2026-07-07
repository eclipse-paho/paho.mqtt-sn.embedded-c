/****************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi
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
 *---------------------------------------------------------------------------
 *
 *   MQTT-SN GATEWAY TEST CLIENT
 *
 *   Supported functions.
 *
 *   void PUBLISH  ( const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false );
 *
 *   void PUBLISH  ( uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false );
 *
 *  void SUBSCRIBE ( const char* topicName, TopicCallback onPublish, uint8_t qos );
 *
 *  void SUBSCRIBE ( uint16_t topicId, TopicCallback onPublish, uint8_t qos );
 *
 *  void UNSUBSCRIBE ( const char* topicName );
 *
 *  void UNSUBSCRIBE ( uint16_t topicId );
 *
 *   void DISCONNECT ( uint16_t sleepInSecs );
 *
 *   void CONNECT ( void );
 *
 *   void DISPLAY( format, .....);    <== instead of printf()
 *
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation
 ***************************************************************************/

#include "LMqttsnClientApp.h"
#include "LMqttsnClient.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;
extern LMqttsnClient* theClient;
extern LScreen* theScreen;

/*------------------------------------------------------
 *    UDP Configuration    (theNetcon)
 *------------------------------------------------------*/
UDPCONF = { "QoS-1_Client01", // ClientId
        { 225, 1, 1, 1 },         // Multicast group IP
        1883,                // Multicast group Port
        20001,               // Local PortNo
        };

/*------------------------------------------------------
 *    UDP6 Configuration    (theNetcon)
 *------------------------------------------------------*/
UDP6CONF = { "QoS-1_Client01",  // ClientId
        "ff1e:feed:caca:dead::1", // Multicast group IP
        "wlp4s0",                 // Network Interface
        1883,                     // Multicast group Port
        20020,                    // Local PortNo
        };

/*------------------------------------------------------
 *    RFCOMM Configuration    (theNetcon)
 *------------------------------------------------------*/
RFCOMMCONF = { "QoS-1_Client01",      // ClientId
        "60:57:18:06:8B:72",          // GW Address
        1,                            // Rfcomm channel
        };

/*------------------------------------------------------
 *    Client Configuration  (theMqcon)
 *------------------------------------------------------*/
MQTTSNCONF = {
    300,            //KeepAlive [seconds]
    true,          //Clean session
    300,           //Sleep duration [seconds]
    "",            //WillTopic
    "",            //WillMessage
    0,             //WillQos
    false          //WillRetain
};

/*------------------------------------------------------
 *     Define Topics
 *------------------------------------------------------*/


/*------------------------------------------------------
 *       Callback routines for Subscribed Topics
 *------------------------------------------------------*/

/*------------------------------------------------------
 *      A Link list of Callback routines and Topics
 *------------------------------------------------------*/
SUBSCRIBE_LIST = {// e.g. SUB(TopicType, topicName, TopicId, callback, QoSx),

                  END_OF_SUBSCRIBE_LIST
                 };

/*------------------------------------------------------
 *    Test functions
 *------------------------------------------------------*/

void publishTopic1(void)
{
    char payload[300];
    sprintf(payload, "publish \"ty4tw/Topic1\" \n");
    uint8_t qos = 3;
    PUBLISH(1,(uint8_t*)payload, strlen(payload), qos);
}

void publishTopic2(void)
{
    char payload[300];
    sprintf(payload, "publish \"ty4tw/topic2\" \n");
    uint8_t qos = 3;
    PUBLISH(2,(uint8_t*)payload, strlen(payload), qos);
}

void publishTopic57(void)
{
    char payload[300];
    sprintf(payload, "publish \"ty4tw/topic57\" \n");
    uint8_t qos = 3;
    PUBLISH(5,(uint8_t*)payload, strlen(payload), qos);
}


void disconnect(void)
{
    DISCONNECT(0);
}


/*------------------------------------------------------
 *    A List of Test functions is valid in case of
 *    line 23 of LMqttsnClientApp.h is commented out.
 *    //#define CLIENT_MODE
 *------------------------------------------------------*/

TEST_LIST = {// e.g. TEST( Label, Test),
             TEST("Step1:Publish topic1",     publishTopic1),
             TEST("Step2:Publish topic57",     publishTopic57),
             TEST("Step3:Publish topic2",     publishTopic2),
             END_OF_TEST_LIST
            };


/*------------------------------------------------------
 *    List of tasks is invalid in case of line23 of
 *    LMqttsnClientApp.h is commented out.
 *    #define CLIENT_MODE
 *------------------------------------------------------*/
TASK_LIST = {// e.g. TASK( task, executing duration in second),
            TASK(publishTopic1, 4),  // publishTopic1() is executed every 4 seconds
            TASK(publishTopic2, 7),  // publishTopic2() is executed every 7 seconds
             END_OF_TASK_LIST
            };


/*------------------------------------------------------
 *    Initialize function
 *------------------------------------------------------*/
void setup(void)
{
    SetQoSMinus1Mode(true);
}


/*****************  END OF  PROGRAM ********************/
