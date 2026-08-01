/****************************************************************************
 * Copyright (c) 2016, 2026 Tomoaki Yamaguchi, Ian Craggs
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
 *   void SUBSCRIBE ( const char* topicName, TopicCallback onPublish, uint8_t qos );
 *
 *   void SUBSCRIBE ( uint16_t topicId, TopicCallback onPublish, uint8_t qos );
 *
 *   void UNSUBSCRIBE ( const char* topicName );
 *
 *   void UNSUBSCRIBE ( uint16_t topicId );
 *
 *   void DISCONNECT ( uint16_t sleepInSecs );
 *
 *   void CONNECT ( void );
 *
 *   void DISPLAY( format, .....);    <== instead of printf()
 *
 *
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation
 *    Ian Craggs - Updating to MQTT-SN 2.0
 ***************************************************************************/

#include "LMqttsnClientApp.h"
#include "LMqttsnClient.h"
#include "LScreen.h"

using namespace std;
using namespace linuxAsyncClient;
extern LMqttsnClient* theClient;
extern LScreen* theScreen;

/*------------------------------------------------------
 *    UDP,DTLS Configuration    (theNetcon)
 *------------------------------------------------------*/
UDPCONF = { "GatewayTestClient",  // ClientId
        { 225, 1, 1, 1 },         // Multicast group IP
        1883,                     // Multicast group Port
        20020,                    // Local PortNo
        };

/*------------------------------------------------------
 *    UDP6, DTLS6 Configuration    (theNetcon)
 *------------------------------------------------------*/
UDP6CONF = { "GatewayTestClient",  // ClientId
        "ff1e:feed:caca:dead::1", // Multicast group IP
        "wlp4s0",                  // Network Interface
        1883,                      // Multicast group Port
        20020,                     // Local PortNo
        };

/*------------------------------------------------------
 *    RFCOMM Configuration    (theNetcon)
 *------------------------------------------------------*/
RFCOMMCONF = { "GatewayTestClient",      // ClientId
        "60:57:18:06:8B:72",          // GW Address
        1,                            // Rfcomm channel
        };

/*------------------------------------------------------
 *    Client Configuration  (theMqcon)
 *------------------------------------------------------*/
MQTTSNCONF = { 60,    //KeepAlive [seconds]
        true,         //Clean session
        300,          //Sleep duration [seconds]
        "willtopic",  //WillTopic
        "willmsg",    //WillMessage
        0,            //WillQos
        false         //WillRetain
        };

/*------------------------------------------------------
 *     Define Topics
 *------------------------------------------------------*/
const char* topic1 = "ty4tw/topic1";
const char* topic2 = "ty4tw/topic2";
const char* topic3 = "ty4tw/topic3";
const char* topic4 = "ty4tw/topic4";
const char* topic40 = "ty4tw/#";
const char* topic51 = "ty4tw/topic5/1";
const char* topic52 = "ty4tw/topic5/2";
const char* topic53 = "ty4tw/topic5/3";
const char* topic50 = "ty4tw/topic5/+";

/*------------------------------------------------------
 *       Callback routines for Subscribed Topics
 *------------------------------------------------------*/
int on_Topic01(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nTopic1 recv.\n");
	char c = pload[ploadlen - 1];
	pload[ploadlen - 1] = 0;   // set null terminator
	DISPLAY("Payload -->%s%c<--\n\n", pload, c);
	return 0;
}

int on_Topic02(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nTopic2 recv.\n");
	pload[ploadlen - 1] = 0;   // set null terminator
	DISPLAY("Payload -->%s　<--\n\n", pload);
	return 0;
}

int on_Topic03(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nNew callback recv Topic3\n");
	pload[ploadlen - 1] = 0;   // set null terminator
	DISPLAY("Payload -->%s　<--\n\n", pload);
	return 0;
}

int on_TopicWildcard(uint8_t* pload, uint16_t ploadlen)
{
	DISPLAY("\n\nNew callback recv TopicWildcard\n");
	pload[ploadlen - 1] = 0;   // set null terminator
	DISPLAY("Payload -->%s　<--\n\n", pload);
	return 0;
}

/*------------------------------------------------------
 *      A Link list of Callback routines and Topics
 *------------------------------------------------------*/

SUBSCRIBE_LIST =
{   // e.g. SUB(TopicType, topicName, TopicId, callback, QoSx),
	SUB(MQTTSN_TOPIC_TYPE_NORMAL, topic1, 0, on_Topic01, QoS1),
	SUB(MQTTSN_TOPIC_TYPE_NORMAL, topic2, 0, on_Topic02, QoS1),
	END_OF_SUBSCRIBE_LIST
};

/*------------------------------------------------------
 *    Test functions
 *------------------------------------------------------*/
void subscribePredefTopic1(void)
{
	SUBSCRIBE_PREDEF(1, on_Topic03, QoS1);
}

void publishTopic1(void)
{
	char payload[300];
	sprintf(payload, "publish \"ty4tw/topic1\" \n");
	PUBLISH(topic1, (uint8_t* )payload, strlen(payload), QoS0);
}

void publishTopic2(void)
{
	char payload[300];
	sprintf(payload, "publish \"ty4tw/topic2\" \n");
	PUBLISH(topic2, (uint8_t* )payload, strlen(payload), QoS1);
}

void publishTopic4(void)
{
	char payload[300];
	sprintf(payload, "publish \"ty4tw/topic40\" \n");
	PUBLISH(topic4, (uint8_t* )payload, strlen(payload), QoS1);
}

void subscribeTopic10(void)
{
	SUBSCRIBE_PREDEF(10, on_Topic02, QoS1);
}

void subscribeWildcardTopic(void)
{
	SUBSCRIBE(topic50, on_TopicWildcard, QoS1);
}


void unsubscribe(void)
{
	UNSUBSCRIBE(topic2);
}

void subscribechangeCallback(void)
{
	SUBSCRIBE(topic2, on_Topic02, QoS1);
}

void test3(void)
{
	char payload[300];
	sprintf(payload, "TEST3 ");
	uint8_t qos = 0;
	PUBLISH(topic2, (uint8_t* )payload, strlen(payload), qos);
}

void disconnect(void)
{
	DISCONNECT(0);
}

void asleep(void)
{
	DISCONNECT(theMqcon.sleepDuration);
}

void onconnect(void)
{
	ONCONNECT();
}

void connect(void)
{
	CONNECT();
}

void DisableAutoPingreq(void)
{
	SetAutoPingReqMode(false);
}

void CleanSessionOff(void)
{
	SetCleanSession(false);
}

/*------------------------------------------------------
 *    A List of Test functions is valid in case of
 *    line 23 of LMqttsnClientApp.h is commented out.
 *    //#define CLIENT_MODE
 *------------------------------------------------------*/

TEST_LIST =
{   // e.g. TEST( Label, Test),
	TEST("Step0:Connect", connect),
	TEST("Step1:Subscribe list", onconnect),
	TEST("Step2:Subscribe predef topic1", subscribePredefTopic1),
	TEST("Step3:Publish topic1", publishTopic1),
	TEST("Step4:Publish topic2", publishTopic2),
	TEST("Step5:Subscribe PreDefined topic10. ID is not defined.", subscribeTopic10),
	TEST("Step6:Publish topic2", publishTopic2),
	TEST("Step7:Unsubscribe topic2", unsubscribe),
	TEST("Step8:Publish topic2", publishTopic2),
	TEST("Step9:subscribe again", subscribechangeCallback),
	TEST("Step10:Publish topic2", publishTopic2),

	TEST("Step11:Sleep     ", asleep),
	TEST("Step12:Publish topic1", publishTopic1),
	TEST("Step13:Disconnect", disconnect),
	TEST("Step14:Publish topic2", publishTopic1),
	TEST("Step15:Connect", connect),
	TEST("Step16:Publish topic2", publishTopic2),
	TEST("Step17:Auto Pingreq mode off", DisableAutoPingreq),
	TEST("Step18:Publish topic2", publishTopic1),
	TEST("Step19:Disconnect", disconnect),
	END_OF_TEST_LIST
};

/*------------------------------------------------------
 *    List of tasks is valid in case of line23 of
 *    LMqttsnClientApp.h is uncommented.
 *    #define CLIENT_MODE
 *------------------------------------------------------*/
TASK_LIST =
{   // e.g. TASK( task, executing duration in second),
	TASK(publishTopic1, 4),// publishTopic1() is executed every 4 seconds
	TASK(publishTopic2, 7),// publishTopic2() is executed every 7 seconds
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
