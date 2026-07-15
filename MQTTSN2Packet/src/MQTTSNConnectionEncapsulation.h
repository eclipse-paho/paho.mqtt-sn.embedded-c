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

#if !defined(MQTTSNCONNECTIONENCAPSULATION_H_)
#define MQTTSNCONNECTIONENCAPSULATION_H_

#include <stdint.h>
#include "MQTTSNPacket.h"   /* MQTTSN_string, MQTTSN_data */

/* -------------------------------------------------------------------------
 * Connection Encapsulation data structure (Section 3.18; packet type 0xFE,
 * enum MQTTSN_CONNECTION)
 *
 * Table 2 "MQTT-SN Control Packet Types" labels this packet type "Session
 * Encapsulation" - that is an error in the specification; the section
 * title (3.18) and body text consistently call it "Connection
 * Encapsulation", which is the name used throughout this implementation.
 *
 * As with the Forwarder Encapsulation (Section 3.19), the outer Length
 * field does NOT cover the whole wire message: it covers only the Length
 * field itself, the Packet Type, and the Client Identifier (Section
 * 3.18.1). The encapsulated MQTT-SN Packet that follows is self-describing
 * - it carries its own Length field - and is not counted in the outer
 * Length.
 *
 * Only Clients send this envelope, wrapping PUBLISH, SUBSCRIBE,
 * UNSUBSCRIBE, REGISTER, DISCONNECT, SLEEPREQ or PINGREQ (Section 3.18-3);
 * this library handles only the wire envelope and does not enforce that
 * restriction or interpret the encapsulated packet's contents.
 *
 * Both variable-length fields use zero-copy pointers into the wire buffer
 * on deserialization; the caller must keep the buffer alive for as long
 * as the data structure is in use. mqttsnPacket includes the encapsulated
 * packet's own Length/Type prefix, so it can be handed directly to
 * MQTTSNPacket_read()-style parsing.
 * ------------------------------------------------------------------------- */
typedef struct MQTTSNPacket_connectionData MQTTSNPacket_connectionData;
struct MQTTSNPacket_connectionData
{
	MQTTSN_string clientID;   /**< Client Identifier, no length prefix - fills to end of outer Length (Section 3.18.2) */
	MQTTSN_data   mqttsnPacket; /**< encapsulated MQTT-SN packet, raw wire bytes including its own length prefix (Section 3.18.3) */
};

/*
 * Serialize a Connection Encapsulation packet.
 *
 * The caller is responsible for providing the already-serialized inner
 * packet in data->mqttsnPacket (including its own Length/Type prefix).
 *
 * @param buf    the buffer into which the packet will be serialized
 * @param buflen the length in bytes of the supplied buffer
 * @param data   the connection data to serialize
 * @return the length of the serialized data.  <= 0 indicates error
 */
int32_t MQTTSNSerialize_connection(uint8_t* buf, int32_t buflen,
        const MQTTSNPacket_connectionData* data);

/*
 * Deserialize a Connection Encapsulation packet.
 *
 * Both fields in the output structure are set as zero-copy pointers into
 * buf; buf must remain valid for the lifetime of the structure.
 *
 * @param data   the structure to fill with parsed packet fields
 * @param buf    the raw buffer data
 * @param buflen the length in bytes of the data in the supplied buffer
 * @return error code.  1 is success, 0 is failure
 */
int32_t MQTTSNDeserialize_connection(MQTTSNPacket_connectionData* data,
        uint8_t* buf, int32_t buflen);

#endif /* MQTTSNCONNECTIONENCAPSULATION_H_ */
