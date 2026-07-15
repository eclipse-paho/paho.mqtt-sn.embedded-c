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

#if !defined(MQTTSNDISCOVERY_H_)
#define MQTTSNDISCOVERY_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_data */

/*
 * ADVERTISE - Gateway Advertisement (Section 3.20.1)
 *
 * Sent periodically by a Gateway (Server) to advertise its presence; the
 * Client does not respond.
 *
 * gatewayId - Gateway Identifier; not guaranteed unique by the protocol
 *             (Section 3.20.1.2)
 * duration  - time interval in seconds until the next ADVERTISE packet from
 *             this Gateway (Section 3.20.1.3)
 */
int32_t MQTTSNSerialize_advertise(uint8_t* buf, int32_t buflen,
        uint8_t gatewayId, uint16_t duration);

int32_t MQTTSNDeserialize_advertise(uint8_t* gatewayId, uint16_t* duration,
        uint8_t* buf, int32_t buflen);

/*
 * SEARCHGW - Search for a Gateway (Section 3.20.2)
 *
 * Sent by a Client to find a Gateway.
 *
 * networkInfo - Additional Network Information (Section 3.20.2.2); fills to
 *               the end of the packet, no length prefix; optional - a
 *               zero-length value means the field is absent from the wire
 */
int32_t MQTTSNSerialize_searchgw(uint8_t* buf, int32_t buflen,
        const MQTTSN_data* networkInfo);

int32_t MQTTSNDeserialize_searchgw(MQTTSN_data* networkInfo,
        uint8_t* buf, int32_t buflen);

/*
 * GWINFO - Gateway Information (Section 3.20.3)
 *
 * Sent in response to a SEARCHGW packet, either by a Gateway (Server) or,
 * if no Gateway responds, by a Client on the Gateway's behalf.
 *
 * gatewayId      - Gateway Identifier (Section 3.20.3.2)
 * gatewayAddress - Network Address of the Gateway (Section 3.20.3.3); fills
 *                  to the end of the packet, no length prefix; optional -
 *                  only present when the packet is sent by a Client, so a
 *                  zero-length value means the field is absent from the wire
 */
int32_t MQTTSNSerialize_gwinfo(uint8_t* buf, int32_t buflen,
        uint8_t gatewayId, const MQTTSN_data* gatewayAddress);

int32_t MQTTSNDeserialize_gwinfo(uint8_t* gatewayId, MQTTSN_data* gatewayAddress,
        uint8_t* buf, int32_t buflen);

#endif /* MQTTSNDISCOVERY_H_ */
