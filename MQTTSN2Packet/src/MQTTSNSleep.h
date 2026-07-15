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

#if !defined(MQTTSNSLEEP_H_)
#define MQTTSNSLEEP_H_

#include <stdint.h>

/*
 * SLEEPREQ (Section 3.15)
 *
 * Client transmit / Server receive.
 *
 * retainTopicAliases - Retain Topic Aliases flag (bit 0 of the SLEEPREQ
 *                       Flags, Section 3.15.2.1): 0 = remove Session Topic
 *                       Aliases during the Asleep state, 1 = retain them
 * packetid            - Packet Identifier, used to match the SLEEPRESP
 *                        (Section 3.15.3)
 * duration             - Sleep Duration in seconds; MUST be greater than 0
 *                        (Section 3.15.4)
 */
int32_t MQTTSNSerialize_sleepreq(uint8_t* buf, int32_t buflen,
        uint8_t retainTopicAliases, uint16_t packetid, uint32_t duration);

int32_t MQTTSNDeserialize_sleepreq(uint8_t* retainTopicAliases,
        uint16_t* packetid, uint32_t* duration,
        uint8_t* buf, int32_t buflen);

/*
 * SLEEPRESP (Section 3.16)
 *
 * Server transmit / Client receive.
 *
 * durationPresent - Sleep Duration Flag (bit 0 of the SLEEPRESP Flags,
 *                   Section 3.16.2.1): 1 = the Sleep Duration field is
 *                   present and overrides the value sent in SLEEPREQ,
 *                   0 = omitted, meaning the Client's requested Sleep
 *                   Duration remains in effect
 * packetid        - Packet Identifier matching the SLEEPREQ being
 *                    acknowledged (Section 3.16.3)
 * duration        - Sleep Duration in seconds (Section 3.16.4); only
 *                    meaningful when durationPresent is non-zero
 * returncode      - Reason Code (Section 3.16.5); optional on the wire,
 *                    inferred from the packet length; MQTT_SN_RC_SUCCESS
 *                    is assumed when absent
 */
int32_t MQTTSNSerialize_sleepresp(uint8_t* buf, int32_t buflen,
        uint8_t durationPresent, uint16_t packetid, uint32_t duration,
        uint8_t returncode);

int32_t MQTTSNDeserialize_sleepresp(uint8_t* durationPresent,
        uint16_t* packetid, uint32_t* duration, uint8_t* returncode,
        uint8_t* buf, int32_t buflen);

/*
 * WAKEUP (Section 3.14)
 *
 * Server transmit / Client receive.
 *
 * The WAKEUP packet has no fields beyond the Length and Packet Type; it is
 * a signal only. The Client need not respond to it.
 */
int32_t MQTTSNSerialize_wakeup(uint8_t* buf, int32_t buflen);

int32_t MQTTSNDeserialize_wakeup(uint8_t* buf, int32_t buflen);

#endif /* MQTTSNSLEEP_H_ */
