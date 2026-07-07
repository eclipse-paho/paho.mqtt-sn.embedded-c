/**************************************************************************************
 * Copyright (c) 2018, 2026 Tomoaki Yamaguchi
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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWENCAPSULATEDPACKET_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWENCAPSULATEDPACKET_H_

namespace MQTTSNGW
{

class WirelessNodeId
{
    friend class MQTTSNGWEncapsulatedPacket;
public:
    WirelessNodeId();
    ~WirelessNodeId();
    void setId(uint8_t* id, uint8_t len);
    void setId(WirelessNodeId* id);
    bool operator ==(WirelessNodeId& id);
private:
    uint8_t _len;
    uint8_t* _nodeId;
};

class MQTTSNGWEncapsulatedPacket
{
public:
    MQTTSNGWEncapsulatedPacket();
    MQTTSNGWEncapsulatedPacket(MQTTSNPacket* packet);
    ~MQTTSNGWEncapsulatedPacket();
    int unicast(SensorNetwork* network, SensorNetAddress* sendTo);
    int serialize(uint8_t* buf);
    int desirialize(unsigned char* buf, unsigned short len);
    int getType(void);
    unsigned char* getPacketData(void);
    int getPacketLength(void);
    const char* getName();
    MQTTSNPacket* getMQTTSNPacket(void);
    void setWirelessNodeId(WirelessNodeId* id);
    WirelessNodeId* getWirelessNodeId(void);
    char* print(char* buf);

private:
    MQTTSNPacket* _mqttsn;
    WirelessNodeId _id;
    uint8_t _ctrl;
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWENCAPSULATEDPACKET_H_ */
