/**************************************************************************************
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
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/
#ifndef MQTTSNGWPROTECTION_H_
#define MQTTSNGWPROTECTION_H_

#include "MQTTSNGWDefines.h"
extern "C" {
#include "MQTTSNProtection.h"
}

namespace MQTTSNGW
{

/*
 * Validate a received PROTECTION packet.
 *
 * Deserializes the envelope, looks up the sender_id in the key store, recomputes
 * the HMAC-SHA256 over the AAD, and compares it with the auth tag in the packet.
 *
 * @param buf         raw packet buffer
 * @param buflen      length of buf
 * @param key         pre-shared key (32 bytes for HMAC-SHA256)
 * @param keylen      key length in bytes
 * @param pd_out      output: parsed protection data (zero-copy into buf)
 * @return 1 if the auth tag is valid, 0 if invalid, -1 on parse error
 */
int GWProtection_validate(uint8_t* buf, int buflen,
                          const uint8_t* key, int keylen,
                          MQTTSNPacket_protectionData* pd_out);

/*
 * Wrap an inner MQTT-SN packet in a PROTECTION envelope.
 *
 * Uses scheme HMAC-SHA256 with a 32-byte auth tag (AUTHTAG_NOMINAL).
 * Generates 4 fresh random bytes for each call.
 *
 * @param sender_id   8-byte sender identifier to embed
 * @param key         pre-shared key
 * @param keylen      key length in bytes
 * @param inner_buf   serialized inner MQTT-SN packet
 * @param inner_len   length of inner packet
 * @param out_buf     output buffer for the serialized PROTECTION packet
 * @param out_buflen  capacity of out_buf
 * @return length of the resulting PROTECTION packet, or <= 0 on error
 */
int32_t GWProtection_wrap(const uint8_t* sender_id,
                          const uint8_t* key, int keylen,
                          const uint8_t* inner_buf, uint16_t inner_len,
                          uint8_t* out_buf, int32_t out_buflen);

} /* namespace MQTTSNGW */
#endif /* MQTTSNGWPROTECTION_H_ */
