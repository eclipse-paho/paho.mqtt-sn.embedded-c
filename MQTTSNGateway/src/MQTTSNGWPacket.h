/**************************************************************************************
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
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/
#ifndef MQTTSNGWPACKET_H_
#define MQTTSNGWPACKET_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNPacket.h"
#include "SensorNetwork.h"

namespace MQTTSNGW
{
class SensorNetwork;

class MQTTSNPacket
{
public:
    MQTTSNPacket(void);
    MQTTSNPacket(MQTTSNPacket &packet);
    ~MQTTSNPacket(void);
    int unicast(SensorNetwork* network, SensorNetAddress* sendTo);
    int broadcast(SensorNetwork* network);
    int recv(SensorNetwork* network);
    int serialize(uint8_t* buf);
    int desirialize(unsigned char* buf, unsigned short len);
    int getType(void);
    unsigned char* getPacketData(void);
    int getPacketLength(void);
    const char* getName();

    int setConnect(void);   // Debug
    int setADVERTISE(uint8_t gatewayid, uint16_t duration);
    int setGWINFO(uint8_t gatewayId);
    int setCONNACK(uint8_t returnCode);
    int setWILLTOPICREQ(void);
    int setWILLMSGREQ(void);
    int setREGISTER(uint16_t topicId, uint16_t msgId, MQTTSNString* TopicName);
    int setREGACK(uint16_t topicId, uint16_t msgId, uint8_t returnCode);
    int setPUBLISH(uint8_t dup, int qos, uint8_t retained, uint16_t msgId,
            MQTTSN_topicid topic, uint8_t* payload, uint16_t payloadlen);
    int setPUBACK(uint16_t topicId, uint16_t msgId, uint8_t returnCode);
    int setPUBREC(uint16_t msgId);
    int setPUBREL(uint16_t msgId);
    int setPUBCOMP(uint16_t msgId);
    int setSUBACK(int qos, uint16_t topicId, uint16_t msgId,
            uint8_t returnCode);
    int setUNSUBACK(uint16_t msgId);
    int setPINGRESP(void);
    int setDISCONNECT(uint16_t duration);
    int setWILLTOPICRESP(uint8_t returnCode);
    int setWILLMSGRESP(uint8_t returnCode);

    int setCONNECT(MQTTSNPacket_connectData* options);
    int setPINGREQ(MQTTSNString* clientId);

    int getSERCHGW(uint8_t* radius);
    int getCONNECT(MQTTSNPacket_connectData* option);
    int getCONNACK(uint8_t* returnCode);
    int getWILLTOPIC(int* willQoS, uint8_t* willRetain,
            MQTTSNString* willTopic);
    int getWILLMSG(MQTTSNString* willmsg);
    int getREGISTER(uint16_t* topicId, uint16_t* msgId,
            MQTTSNString* topicName);
    int getREGACK(uint16_t* topicId, uint16_t* msgId, uint8_t* returnCode);
    int getPUBLISH(uint8_t* dup, int* qos, uint8_t* retained, uint16_t* msgId,
            MQTTSN_topicid* topic, unsigned char** payload, int* payloadlen);
    int getPUBACK(uint16_t* topicId, uint16_t* msgId, uint8_t* returnCode);
    int getACK(uint16_t* msgId);
    int getSUBSCRIBE(uint8_t* dup, int* qos, uint16_t* msgId,
            MQTTSN_topicid* topicFilter);
    int getUNSUBSCRIBE(uint16_t* msgId, MQTTSN_topicid* topicFilter);
    int getPINGREQ(void);
    int getDISCONNECT(uint16_t* duration);
    int getWILLTOPICUPD(uint8_t* willQoS, uint8_t* willRetain,
            MQTTSNString* willTopic);
    int getWILLMSGUPD(MQTTSNString* willMsg);

    bool isAccepted(void);
    bool isDuplicate(void);
    bool isQoSMinusPUBLISH(void);
    char* getMsgId(char* buf);
    int getMsgId(void);
    void setMsgId(uint16_t msgId);
    char* print(char* buf);

private:
    unsigned char* _buf;    // Ptr to a packet data
    int _bufLen; // length of the packet data
};

}
#endif /* MQTTSNGWPACKET_H_ */
