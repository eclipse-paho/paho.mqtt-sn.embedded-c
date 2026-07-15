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

#if !defined(MQTTSNFORWARDER_H_)
#define MQTTSNFORWARDER_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_data */

/* -------------------------------------------------------------------------
 * Forwarder Encapsulation data structure (Section 3.19)
 *
 * Unlike every other envelope/packet in this library, the outer Length
 * field of a Forwarder Encapsulated packet does NOT cover the whole wire
 * message: it covers only the Length field itself, the Packet Type, and
 * the Client Addressing Information (Section 3.19.1). The encapsulated
 * MQTT-SN Packet that follows is self-describing - it carries its own
 * Length field - and is not counted in the outer Length.
 *
 * This library handles only the wire envelope; it does not interpret the
 * encapsulated MQTT-SN packet's contents.
 *
 * Both variable-length fields use MQTTSN_data with zero-copy pointers into
 * the wire buffer on deserialization; the caller must keep the buffer
 * alive for as long as the data structure is in use. mqttsnPacket includes
 * the encapsulated packet's own Length/Type prefix, so it can be handed
 * directly to MQTTSNPacket_read()-style parsing.
 * ------------------------------------------------------------------------- */
typedef struct MQTTSNPacket_forwarderData MQTTSNPacket_forwarderData;
struct MQTTSNPacket_forwarderData
{
	MQTTSN_data clientAddress; /**< Client Addressing Information (Section 3.19.2) */
	MQTTSN_data mqttsnPacket;  /**< encapsulated MQTT-SN packet, raw wire bytes including its own length prefix (Section 3.19.3) */
};

/*
 * Serialize a Forwarder Encapsulation packet.
 *
 * The caller is responsible for providing the already-serialized inner
 * packet in data->mqttsnPacket (including its own Length/Type prefix).
 *
 * @param buf    the buffer into which the packet will be serialized
 * @param buflen the length in bytes of the supplied buffer
 * @param data   the forwarder data to serialize
 * @return the length of the serialized data.  <= 0 indicates error
 */
int32_t MQTTSNSerialize_forwarder(uint8_t* buf, int32_t buflen,
        const MQTTSNPacket_forwarderData* data);

/*
 * Deserialize a Forwarder Encapsulation packet.
 *
 * Both fields in the output structure are set as zero-copy pointers into
 * buf; buf must remain valid for the lifetime of the structure.
 *
 * @param data   the structure to fill with parsed packet fields
 * @param buf    the raw buffer data
 * @param buflen the length in bytes of the data in the supplied buffer
 * @return error code.  1 is success, 0 is failure
 */
int32_t MQTTSNDeserialize_forwarder(MQTTSNPacket_forwarderData* data,
        uint8_t* buf, int32_t buflen);

#endif /* MQTTSNFORWARDER_H_ */
