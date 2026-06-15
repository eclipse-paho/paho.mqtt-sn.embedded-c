/*******************************************************************************
* Copyright (c) 2014, 2026 IBM Corp., Ian Craggs
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Updated for MQTT-SN 2.0 (Committee Specification Draft 01, October 2025)
 *******************************************************************************/

#if !defined(MQTTSNUNSUBSCRIBE_H_)
#define MQTTSNUNSUBSCRIBE_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_topic */

/* ---- UNSUBSCRIBE (Section 3.9) / UNSUBACK (Section 3.10) ---- */

/*
 * Client transmit / Server receive
 *
 * packetid - packet identifier
 * topic    - topic type plus alias (alt.alias) or filter string (alt.string);
 *            UNSUBSCRIBE flags carry only bits 0-1 (Topic Type), all others reserved
 */
int32_t MQTTSNSerialize_unsubscribe(uint8_t* buf, int32_t buflen,
        uint16_t packetid, MQTTSN_topic* topic);

/*
 * Server receive / Client transmit (server side deserialize)
 *
 * packetid - returned packet identifier
 * topic    - returned topic type plus alias or filter string
 */
int32_t MQTTSNDeserialize_unsubscribe(uint16_t* packetid,
        MQTTSN_topic* topic, uint8_t* buf, int32_t buflen);

/*
 * Server transmit / Client receive
 *
 * packetid   - packet identifier matching the UNSUBSCRIBE
 * returncode - Reason Code: 0x00 = Success, 0x11 = No subscription existed,
 *              >= 0x80 = failure (see enum MQTTSN_reasonCodes in MQTTSNPacket.h)
 *              Omitted from the wire when MQTT_SN_RC_SUCCESS (Section 3.10.3)
 *
 * Note: UNSUBACK has NO flags byte — the wire format is:
 *   length + type + packetid(2) + [reasoncode(1, optional)]
 */
int32_t MQTTSNSerialize_unsuback(uint8_t* buf, int32_t buflen,
        uint16_t packetid, uint8_t returncode);

/*
 * Client receive / Server transmit (client side deserialize)
 *
 * packetid   - returned packet identifier matching the UNSUBSCRIBE
 * returncode - returned Reason Code; MQTT_SN_RC_SUCCESS assumed when absent
 */
int32_t MQTTSNDeserialize_unsuback(uint16_t* packetid, uint8_t* returncode,
        uint8_t* buf, int32_t buflen);

#endif /* MQTTSNSUBSCRIBE_H_ */
