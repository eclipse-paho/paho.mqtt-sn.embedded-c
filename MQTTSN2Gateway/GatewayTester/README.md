# Gateway Test Program.    
**sample/mainTest.cpp** is a Test sample coading.   
Each test is described as one function. test1(), test2()...     
```
void test1(void)
{
    char payload[300];
    sprintf(payload, "ESP8266-08b133 ");
    uint8_t qos = 0;
    PUBLISH(topic1,(uint8_t*)payload, strlen(payload), qos);
}

void test2(void)
{
    uint8_t qos = 1;
    SUBSCRIBE(topic2, on_publish02, qos);
}

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
 *--------------------------------------------------------------------------
 
```
**TEST_LIST** is a test senario. Test functions are executed interactively. 
``` 
/*------------------------------------------------------
 *    A List of Test Tasks
 *------------------------------------------------------*/

TEST_LIST = {// e.g. TEST( Label, Test),
             TEST("Publish topic1",     test1),
             TEST("Subscribe topic2",   test2),
             TEST("Publish topic2",     test3),
             TEST("Unsubscribe topic2", test4),
             TEST("Publish topic2",     test3),
             TEST("Disconnect",         test5),
             END_OF_TEST_LIST
        };
        
``` 

**UDP**, **DTLS**, **UDP6**, **DTLS6** or **Bluetooth** is available as a sensor network.
```
/*------------------------------------------------------
 *    UDP, DTLS Configuration    (theNetcon)
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
        "ff12::feed:caca:dead",    // Multicast group IP
        "wlp4s0",
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
```


## How to Build    
```
copy codes from the github.
$ cd paho.mqtt-sn.embedded-c/MQTTSNGateway/GatewayTester       
$ ./build.sh [udp | udp6 | dtls | dtls6 | rfcomm]
```       

    
## Execute Gateway Tester
```    
$ ./Build/MQTT-SNGatewayTester
  
 ***************************************************************************
 * MQTT-SN Gateway Tester   DTLS
 * Part of Project Paho in Eclipse
 * (http://git.eclipse.org/c/paho/org.eclipse.paho.mqtt-sn.embedded-c.git/)
 *
 * Author : Tomoaki YAMAGUCHI
 * Version: 2.0.0
 ***************************************************************************
Attempting to Connect the Broker.....
Execute "Step0:Connect" ? ( y/n ) : 

```    
