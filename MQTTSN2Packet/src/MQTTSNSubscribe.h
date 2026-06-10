/*******************************************************************************
 * Copyright (c) 2014, 2025 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

#if !defined(MQTTSNSUBSCRIBE_H_)
#define MQTTSNSUBSCRIBE_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_topic */

/*
 * Client transmit / Server receive
 *
 * qos             - requested maximum QoS (0, 1, or 2)
 * retain_handling - 0 = send retained on subscribe, 1 = only if new subscription,
 *                   2 = never send retained
 * rap             - Retain as Published: 1 = keep RETAIN flag on forwarded messages
 * no_local        - 1 = do not deliver messages published by this Client back to itself
 * packetid        - packet identifier
 * topic           - topic type plus alias (alt.alias) or filter string (alt.string)
 */
int32_t MQTTSNSerialize_subscribe(uint8_t* buf, int32_t buflen,
        int32_t qos, uint8_t retain_handling, uint8_t rap, uint8_t no_local,
        uint16_t packetid, MQTTSN_topic* topic);

/*
 * Server receive / Client transmit (server side deserialize)
 *
 * qos             - returned maximum QoS from the SUBSCRIBE flags
 * retain_handling - returned retain handling option
 * rap             - returned Retain as Published flag
 * no_local        - returned No Local flag
 * packetid        - returned packet identifier
 * topic           - returned topic type plus alias or filter string
 */
int32_t MQTTSNDeserialize_subscribe(int32_t* qos, uint8_t* retain_handling,
        uint8_t* rap, uint8_t* no_local, uint16_t* packetid,
        MQTTSN_topic* topic, uint8_t* buf, int32_t buflen);

/*
 * Server transmit / Client receive
 *
 * topic_type          - topic type to report in the SUBACK flags
 * topic_alias_present - 1 = include a Topic Alias field, 0 = omit it
 * topicid             - topic alias to assign (used only when topic_alias_present is 1)
 * packetid            - packet identifier matching the SUBSCRIBE
 * returncode          - Reason Code: MQTT_SN_RC_GRANTED_QOS_0/1/2 on success,
 *                       >= 0x80 on failure (see enum MQTTSN_reasonCodes in MQTTSNPacket.h)
 */
int32_t MQTTSNSerialize_suback(uint8_t* buf, int32_t buflen,
        uint8_t topic_type, uint8_t topic_alias_present,
        uint16_t topicid, uint16_t packetid, uint8_t returncode);

/*
 * Client receive / Server transmit (client side deserialize)
 *
 * topicid             - returned topic alias assigned by the Server
 *                       (valid only when topic_alias_present is non-zero)
 * topic_alias_present - returned: 1 if a Topic Alias field was present, 0 otherwise
 * packetid            - returned packet identifier matching the SUBSCRIBE
 * returncode          - returned Reason Code (see above)
 */
int32_t MQTTSNDeserialize_suback(uint16_t* topicid, uint8_t* topic_alias_present,
        uint16_t* packetid, uint8_t* returncode,
        uint8_t* buf, int32_t buflen);

#endif /* MQTTSNSUBSCRIBE_H_ */
