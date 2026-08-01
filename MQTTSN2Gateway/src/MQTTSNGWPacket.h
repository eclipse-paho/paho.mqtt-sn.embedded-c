/**************************************************************************************
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
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/
#ifndef MQTTSNGWPACKET_H_
#define MQTTSNGWPACKET_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNPacket.h"   /* includes MQTTSNConnect.h and MQTTSNPublish.h inside extern "C" */
extern "C" {
#include "MQTTSNSubscribe.h"
#include "MQTTSNUnsubscribe.h"
#include "MQTTSNDiscovery.h"
#include "MQTTSNSleep.h"
}
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

    /* Debug */
    int setConnect(void);

    /* Discovery */
    int setADVERTISE(uint8_t gatewayid, uint16_t duration);
    int setGWINFO(uint8_t gatewayId);
    int getSERCHGW(MQTTSN_data* networkInfo);

    /* Connection */
    int setCONNECT(MQTTSNPacket_connectData* options);
    int getCONNECT(MQTTSNPacket_connectData* data);
    int setCONNACK(MQTTSNPacket_connackData* data);
    int getCONNACK(MQTTSNPacket_connackData* data);
    int setPINGREQ(uint16_t packetid);
    int setPINGRESP(uint16_t packetid, int messages_remaining);
    int getPINGREQ(uint16_t* packetid);
    int setDISCONNECT(uint8_t reason_code);
    int getDISCONNECT(MQTTSNPacket_disconnectData* data);

    /* Sleep — v2.0 replaces DISCONNECT-with-duration */
    int getSLEEPREQ(uint8_t* retainTopicAliases, uint16_t* packetid, uint32_t* duration);
    int setSLEEPRESP(uint8_t durationPresent, uint16_t packetid,
            uint32_t duration, uint8_t returncode);
    int setWAKEUP(void);

    /* Publish */
    int setPUBLISH(uint8_t dup, int qos, uint8_t retained, uint16_t packetid,
            const MQTTSN_topic* topic, uint8_t* payload, int32_t payloadlen);
    int getPUBLISH(uint8_t* dup, int* qos, uint8_t* retained, uint16_t* packetid,
            MQTTSN_topic* topic, uint8_t** payload, int32_t* payloadlen);
    int setPUBACK(uint16_t packetid, uint8_t returncode);
    int getPUBACK(uint16_t* packetid, uint8_t* returncode);
    int setPUBREC(uint16_t packetid, uint8_t returncode);
    int setPUBREL(uint16_t packetid, uint8_t returncode);
    int setPUBCOMP(uint16_t packetid, uint8_t returncode);
    int getACK(uint16_t* packetid, uint8_t* returncode);

    /* PUBWOS — Publish Without Session (v2.0 replacement for QoS -1) */
    int setPUBWOS(uint8_t retained, const MQTTSN_topic* topic,
            uint8_t* payload, int32_t payloadlen);
    int getPUBWOS(uint8_t* retained, MQTTSN_topic* topic,
            uint8_t** payload, int32_t* payloadlen);

    /* Register / RegAck */
    int setREGISTER(uint8_t topic_alias_present, uint16_t topicid,
            uint16_t packetid, MQTTSN_string* topicname);
    int getREGISTER(uint8_t* topic_alias_present, uint16_t* topicid,
            uint16_t* packetid, MQTTSN_string* topicname);
    int setREGACK(uint8_t topic_type, uint8_t topic_alias_present,
            uint16_t topicid, uint16_t packetid, uint8_t returncode);
    int getREGACK(uint8_t* topic_type, uint8_t* topic_alias_present,
            uint16_t* topicid, uint16_t* packetid, uint8_t* returncode);

    /* Subscribe / Suback */
    int getSUBSCRIBE(int* qos, uint8_t* retain_handling, uint8_t* rap,
            uint8_t* no_local, uint16_t* packetid, MQTTSN_topic* topic);
    int setSUBACK(uint8_t topic_type, uint8_t topic_alias_present,
            uint16_t topicid, uint16_t packetid, uint8_t returncode);

    /* Unsubscribe / Unsuback */
    int getUNSUBSCRIBE(uint16_t* packetid, MQTTSN_topic* topic);
    int setUNSUBACK(uint16_t packetid, uint8_t returncode);

    bool isAccepted(void);
    bool isDuplicate(void);
    bool isPUBWOS(void);
    char* getMsgId(char* buf);
    int getMsgId(void);
    void setMsgId(uint16_t msgId);
    char* print(char* buf);

private:
    unsigned char* _buf;
    int _bufLen;
};

}
#endif /* MQTTSNGWPACKET_H_ */
