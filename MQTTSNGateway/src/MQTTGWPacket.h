/*******************************************************************************
 * Copyright (c) 2009, 2026 IBM Corp., Ian Craggs
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
 *    Ian Craggs, Allan Stockdill-Mander - SSL updates
 *    Ian Craggs - MQTT 3.1.1 support
 *    Tomoaki Yamaguchi - modify codes for MATT-SN Gateway
 *******************************************************************************/

#ifndef MQTTGWPACKET_H_
#define MQTTGWPACKET_H_

#include "Network.h"

namespace MQTTSNGW
{

typedef void* (*pf)(unsigned char, char*, size_t);

#define BAD_MQTT_PACKET -4

enum msgTypes
{
    CONNECT = 1,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT
};

/**
 * Bitfields for the MQTT header byte.
 */
typedef union
{
    /*unsigned*/
    char byte; /**< the whole byte */
#if defined(REVERSED)
    struct
    {
        unsigned int type : 4; /**< message type nibble */
        bool dup : 1; /**< DUP flag bit */
        unsigned int qos : 2; /**< QoS value, 0, 1 or 2 */
        bool retain : 1; /**< retained flag bit */
    }bits;
#else
    struct
    {
        bool retain :1; /**< retained flag bit */
        unsigned int qos :2; /**< QoS value, 0, 1 or 2 */
        bool dup :1; /**< DUP flag bit */
        unsigned int type :4; /**< message type nibble */
    } bits;
#endif
} Header;

/**
 * Data for a connect packet.
 */

enum MQTT_connackCodes
{
    MQTT_CONNECTION_ACCEPTED,
    MQTT_UNACCEPTABLE_PROTOCOL_VERSION,
    MQTT_IDENTIFIER_REJECTED,
    MQTT_SERVER_UNAVAILABLE,
    MQTT_BAD_USERNAME_OR_PASSWORD,
    MQTT_NOT_AUTHORIZED
};

typedef struct
{
    Header header; /**< MQTT header byte */
    union
    {
        unsigned char all; /**< all connect flags */
#if defined(REVERSED)
        struct
        {
            bool username : 1; /**< 3.1 user name */
            bool password : 1; /**< 3.1 password */
            bool willRetain : 1; /**< will retain setting */
            unsigned int willQoS : 2; /**< will QoS value */
            bool will : 1; /**< will flag */
            bool cleanstart : 1; /**< cleansession flag */
            int : 1; /**< unused */
        }bits;
#else
        struct
        {
            int :1; /**< unused */
            bool cleanstart :1; /**< cleansession flag */
            bool will :1; /**< will flag */
            unsigned int willQoS :2; /**< will QoS value */
            bool willRetain :1; /**< will retain setting */
            bool password :1; /**< 3.1 password */
            bool username :1; /**< 3.1 user name */
        } bits;
#endif
    } flags; /**< connect flags byte */

    char *Protocol, /**< MQTT protocol name */
    *clientID, /**< string client id */
    *willTopic, /**< will topic */
    *willMsg; /**< will payload */

    int keepAliveTimer; /**< keepalive timeout value in seconds */
    unsigned char version; /**< MQTT version number */
} Connect;

#define MQTTPacket_Connect_Initializer {{0}, {0}, nullptr, nullptr, nullptr, nullptr, 0, 0}
#define MQTTPacket_willOptions_initializer { {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }
#define MQTTPacket_connectData_initializer { {'M', 'Q', 'T', 'C'}, 0, 4, {NULL, {0, NULL}}, 60, 1, 0, \
        MQTTPacket_willOptions_initializer, {NULL, {0, NULL}}, {NULL, {0, NULL}} }

/**
 * Data for a willMessage.
 */
typedef struct
{
    char* topic;
    char* msg;
    int retained;
    int qos;
} willMessages;

/**
 * Data for a connack packet.
 */
typedef struct
{
    Header header; /**< MQTT header byte */
    union
    {
        unsigned char all; /**< all connack flags */
#if defined(REVERSED)
        struct
        {
            unsigned int reserved : 7; /**< message type nibble */
            bool sessionPresent : 1; /**< was a session found on the server? */
        }bits;
#else
        struct
        {
            bool sessionPresent :1; /**< was a session found on the server? */
            unsigned int reserved :7; /**< message type nibble */
        } bits;
#endif
    } flags; /**< connack flags byte */
    char rc; /**< connack return code */
} Connack;

/**
 * Data for a publish packet.
 */
typedef struct
{
    Header header; /**< MQTT header byte */
    char* topic; /**< topic string */
    int topiclen;
    int msgId; /**< MQTT message id */
    char* payload; /**< binary payload, length delimited */
    int payloadlen; /**< payload length */
} Publish;

#define MQTTPacket_Publish_Initializer {{0}, nullptr, 0, 0, nullptr, 0}

/**
 * Data for one of the ack packets.
 */
typedef struct
{
    Header header; /**< MQTT header byte */
    int msgId; /**< MQTT message id */
} Ack;

/**
 * UTF8String.
 */
typedef struct
{
    unsigned char len;
    char* data;
} UTF8String;

/**
 * Class MQTT Packet
 */
class MQTTGWPacket
{
public:
    MQTTGWPacket();
    ~MQTTGWPacket();
    int recv(Network* network);
    int send(Network* network);
    int getType(void);
    int getPacketData(unsigned char* buf);
    int getPacketLength(void);
    const char* getName(void);

    int getAck(Ack* ack);
    int getCONNACK(Connack* resp);
    int getSUBACK(unsigned short* msgId, unsigned char* rc);
    int getPUBLISH(Publish* pub);

    int setCONNECT(Connect* conect, unsigned char* username,
            unsigned char* password);
    int setPUBLISH(Publish* pub);
    int setAck(unsigned char msgType, unsigned short msgid);
    int setHeader(unsigned char msgType);
    int setSUBSCRIBE(const char* topic, unsigned char qos,
            unsigned short msgId);
    int setUNSUBSCRIBE(const char* topics, unsigned short msgid);

    UTF8String getTopic(void);
    char* getMsgId(char* buf);
    int getMsgId(void);
    void setMsgId(int msgId);
    char* print(char* buf);
    MQTTGWPacket& operator =(MQTTGWPacket& packet);

private:
    void clearData(void);
    Header _header;
    int _remainingLength;
    unsigned char* _data;
};

}

#endif /* MQTTGWPACKET_H_ */
