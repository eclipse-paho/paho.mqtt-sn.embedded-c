/**************************************************************************************
 * Copyright (c) 2018, 2026 Tomoaki Yamaguchi, Ian Craggs
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
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 *    Ian Craggs - Updating to MQTT-SN 2.0
 **************************************************************************************/

#ifndef MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATETOPICTABLE_H_
#define MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATETOPICTABLE_H_

#include "MQTTSNGWDefines.h"
#include "MQTTSNGWProcess.h"
#include <stdint.h>
namespace MQTTSNGW
{

class Client;
class Topic;
class AggregateTopicElement;
class ClientTopicElement;
class Mutex;

/*=====================================
 Class AggregateTopicTable
 ======================================*/
class AggregateTopicTable
{
public:
    AggregateTopicTable();
    ~AggregateTopicTable();

    AggregateTopicElement* add(Topic* topic, Client* client);
    AggregateTopicElement* getAggregateTopicElement(Topic* topic);
    ClientTopicElement* getClientElement(Topic* topic);
    void erase(Topic* topic, Client* client);
    void clear(void);

    void print(void);

private:
    void erase(AggregateTopicElement* elmTopic);
    Mutex _mutex;
    AggregateTopicElement* _head { nullptr };
    AggregateTopicElement* _tail { nullptr };
    int _cnt { 0 };
    int _maxSize { MAX_MESSAGEID_TABLE_SIZE };
};

/*=====================================
 Class AggregateTopicElement
 =====================================*/
class AggregateTopicElement
{
    friend class AggregateTopicTable;
public:
    AggregateTopicElement(void);
    AggregateTopicElement(Topic* topic, Client* client);
    ~AggregateTopicElement(void);

    ClientTopicElement* add(Client* client);
    ClientTopicElement* getFirstClientTopicElement(void);
    ClientTopicElement* getNextClientTopicElement(
            ClientTopicElement* elmClient);
    void eraseClient(Client* client);
    ClientTopicElement* find(Client* client);

private:
    Mutex _mutex;
    Topic* _topic { nullptr };
    AggregateTopicElement* _next { nullptr };
    AggregateTopicElement* _prev { nullptr };
    ClientTopicElement* _head { nullptr };
    ClientTopicElement* _tail { nullptr };
};

/*=====================================
 Class ClientTopicElement
 =====================================*/
class ClientTopicElement
{
    friend class AggregateTopicTable;
    friend class AggregateTopicElement;
public:
    ClientTopicElement(Client* client);
    ~ClientTopicElement(void);

    ClientTopicElement* getNextClientElement(void);
    Client* getClient(void);

private:
    Client* _client { nullptr };
    ClientTopicElement* _next { nullptr };
    ClientTopicElement* _prev { nullptr };
};

}

#endif /* MQTTSNGATEWAY_SRC_MQTTSNGWAGGREGATETOPICTABLE_H_ */
