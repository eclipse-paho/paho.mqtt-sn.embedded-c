/****************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
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
UDPCONF = { "ClientSUB", // ClientId
        { 225, 1, 1, 1 },         // Multicast group IP
        1883,                // Multicast group Port
        20011,               // Local PortNo
        };

/*------------------------------------------------------
 *    UDP6 Configuration    (theNetcon)
 *------------------------------------------------------*/
UDP6CONF = { "ClientSUB",  // ClientId
        "ff1e:feed:caca:dead::1", // Multicast group IP
        "wlp4s0",                 // Network Interface
        1883,                     // Multicast group Port
        20020,                    // Local PortNo
        };

/*------------------------------------------------------
 *    RFCOMM Configuration    (theNetcon)
 *------------------------------------------------------*/
RFCOMMCONF = { "ClientSUB",      // ClientId
        "60:57:18:06:8B:72",          // GW Address
        1,                            // Rfcomm channel
        };

/*------------------------------------------------------
 *    Client Configuration  (theMqcon)
 *------------------------------------------------------*/
MQTTSNCONF = {
	 60,            //KeepAlive [seconds]
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
const char* topic1 = "ty4tw/topic1";
const char* topic2 = "ty4tw/topic2";
const char* topic3 = "ty4tw/topic3";
const char* topic4 = "a";
const char* topic5 = "ty4tw/#";


/*------------------------------------------------------
 *       Callback routines for Subscribed Topics
 *------------------------------------------------------*/
int on_Topic01(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nTopic1 recv.\n");
	char c = pload[ploadlen-1];
	pload[ploadlen-1]= 0;   // set null terminator
	DISPLAY("Payload -->%s%c<--\n\n",pload, c);
	return 0;
}

int on_Topic02(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nTopic2 recv.\n");
	pload[ploadlen-1]= 0;   // set null terminator
	DISPLAY("Payload -->%s　<--\n\n",pload);
	return 0;
}

int on_Topic03(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nNew callback recv TopicA\n");
	pload[ploadlen-1]= 0;   // set null terminator
	DISPLAY("Payload -->%s　<--\n\n",pload);
	return 0;
}

int on_Topic05(uint8_t* pload, uint16_t ploadlen)
{
    DISPLAY("\n\nNew callback recv TopicA\n");
    pload[ploadlen-1]= 0;   // set null terminator
    DISPLAY("Payload -->%s　<--\n\n",pload);
    return 0;
}

/*------------------------------------------------------
 *      A Link list of Callback routines and Topics
 *------------------------------------------------------*/

SUBSCRIBE_LIST = {// e.g. SUB(TopicType, topicName, TopicId, callback, QoSx),

                  // SUB(MQTTSN_TOPIC_TYPE_NORMAL, topic5, 0, on_Topic05, QoS1),
                  //SUB(MQTTSN_TOPIC_TYPE_NORMAL, topic2, 0, on_Topic02, QoS1),
                  END_OF_SUBSCRIBE_LIST
                 };



/*------------------------------------------------------
 *    Test functions
 *------------------------------------------------------*/


void subscribeTopic1(void)
{
    uint8_t qos = 1;
    SUBSCRIBE(topic1, on_Topic01, qos);
}

void subscribeTopic2(void)
{
	uint8_t qos = 1;
	SUBSCRIBE(topic2, on_Topic02, qos);
}

void subscribeTopic5(void)
{
    uint8_t qos = 1;
    SUBSCRIBE(topic5, on_Topic05, qos);
}

void disconnect(void)
{
	DISCONNECT(0);
}

void connect(void)
{
	CONNECT();
}

void asleep(void)
{
	DISCONNECT(theMqcon.sleepDuration);
}

/*------------------------------------------------------
 *    A List of Test functions is valid in case of
 *    line 23 of LMqttsnClientApp.h is commented out.
 *    //#define CLIENT_MODE
 *------------------------------------------------------*/

TEST_LIST = {// e.g. TEST( Label, Test),
			 TEST("Step1:Subscribe topic5",     subscribeTopic5),
			 //TEST("Step2:Subscribe topic2",     subscribeTopic2),
			 TEST("Step2:Disconnect",     asleep),
			 TEST("Step3:Cconnect",     connect),
			 TEST("Step4:Disconnect",     asleep),
			 END_OF_TEST_LIST
			};


/*------------------------------------------------------
 *    List of tasks is valid in case of line23 of
 *    LMqttsnClientApp.h is uncommented.
 *    #define CLIENT_MODE
 *------------------------------------------------------*/
TASK_LIST = {// e.g. TASK( task, executing duration in second),
             END_OF_TASK_LIST
            };


/*------------------------------------------------------
 *    Initialize function
 *------------------------------------------------------*/
void setup(void)
{
    SetForwarderMode(false);
}


/*****************  END OF  PROGRAM ********************/

