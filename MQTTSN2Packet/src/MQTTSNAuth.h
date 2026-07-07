/*******************************************************************************
 * Copyright (c) 2026 Ian Craggs
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
 *    Added for MQTT-SN 2.0 (Committee Specification Draft 02, February 2026)
 *******************************************************************************/

#if !defined(MQTTSNAUTH_H_)
#define MQTTSNAUTH_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_string, MQTTSN_data */

/*
 * AUTH - Authentication Exchange (Section 3.3)
 *
 * packetid   - identifies the corresponding CONNECT or AUTH packet
 *              (Section 3.3.2)
 * reasonCode - Authentication Reason Code, e.g. MQTT_SN_RC_CONTINUE_AUTH,
 *              MQTT_SN_RC_RE_AUTHENTICATE, or MQTT_SN_RC_SUCCESS
 *              (Section 3.3.3)
 * authMethod - Authentication Method name; 1-byte length prefix on the wire
 *              (Sections 3.3.4-3.3.5)
 * authData   - Authentication Data; fills to the end of the packet, no
 *              length prefix (Section 3.3.6)
 */
int32_t MQTTSNSerialize_auth(uint8_t* buf, int32_t buflen,
        uint16_t packetid, uint8_t reasonCode,
        const MQTTSN_string* authMethod, const MQTTSN_data* authData);

int32_t MQTTSNDeserialize_auth(uint16_t* packetid, uint8_t* reasonCode,
        MQTTSN_string* authMethod, MQTTSN_data* authData,
        uint8_t* buf, int32_t buflen);

#endif /* MQTTSNAUTH_H_ */
