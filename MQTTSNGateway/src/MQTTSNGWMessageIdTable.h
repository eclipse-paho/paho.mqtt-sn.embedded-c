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

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWMESSAGEIDTABLE_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWMESSAGEIDTABLE_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWProcess.h"

#include <stdint.h>
namespace MQTTSNGW
{

class Client;
class MessageIdElement;
class Meutex;
class Aggregater;
/*=====================================
 Class MessageIdTable
 ======================================*/
class MessageIdTable
{
public:
    MessageIdTable();
    ~MessageIdTable();

    MessageIdElement* add(Aggregater* aggregater, Client* client,
            uint16_t clientMsgId);
    Client* getClientMsgId(uint16_t msgId, uint16_t* clientMsgId);
    uint16_t getMsgId(Client* client, uint16_t clientMsgId);
    void erase(uint16_t msgId);
    void clear(MessageIdElement* elm);
private:
    MessageIdElement* find(uint16_t msgId);
    MessageIdElement* find(Client* client, uint16_t clientMsgId);
    MessageIdElement* _head { nullptr };
    MessageIdElement* _tail{ nullptr };
    int _cnt { 0 };
    int _maxSize { MAX_MESSAGEID_TABLE_SIZE };
    Mutex _mutex;
};

/*=====================================
 Class MessageIdElement
 =====================================*/
class MessageIdElement
{
    friend class MessageIdTable;
    friend class Aggregater;
public:
    MessageIdElement(void);
    MessageIdElement(uint16_t msgId, Client* client, uint16_t clientMsgId);
    ~MessageIdElement(void);

private:
    uint16_t _msgId;
    uint16_t _clientMsgId;
    Client* _client;
    MessageIdElement* _next;
    MessageIdElement* _prev;
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWMESSAGEIDTABLE_H_ */
