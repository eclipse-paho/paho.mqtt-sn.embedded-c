/**************************************************************************************
 * Copyright (c) 2018, Tomoaki Yamaguchi
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

#include "MQTTSNGWMessageIdTable.h"
#include "MQTTSNGWClient.h"

using namespace MQTTSNGW;

/*===============================
 * Class MessageIdTable
 ===============================*/
MessageIdTable::MessageIdTable()
{

}

MessageIdTable::~MessageIdTable()
{
    _mutex.lock();
    if (_head != nullptr)
    {
        MessageIdElement* p = _tail;
        while (p)
        {
            MessageIdElement* pPrev = p;
            delete p;
            _cnt--;
            p = pPrev->_prev;
        }
        _head = _tail = nullptr;
    }
    _mutex.unlock();
}

MessageIdElement* MessageIdTable::add(Aggregater* aggregater, Client* client, uint16_t clientMsgId)
{
    if (_cnt > _maxSize)
    {
        return nullptr;
    }

    MessageIdElement* elm = new MessageIdElement(0, client, clientMsgId);
    if (elm == nullptr)
    {
        return nullptr;
    }
    _mutex.lock();
    if (_head == nullptr)
    {
        elm->_msgId = aggregater->msgId();
        _head = elm;
        _tail = elm;
        _cnt++;
    }
    else
    {
        MessageIdElement* p = find(client, clientMsgId);
        if (p == nullptr)
        {
            elm->_msgId = aggregater->msgId();
            p = _tail;
            _tail = elm;
            elm->_prev = p;
            p->_next = elm;
            _cnt++;
        }
        else
        {
            delete elm;
            elm = nullptr;
        }
    }
    _mutex.unlock();
    return elm;
}

MessageIdElement* MessageIdTable::find(uint16_t msgId)
{
    MessageIdElement* p = _head;
    while (p)
    {
        if (p->_msgId == msgId)
        {
            break;
        }
        p = p->_next;
    }
    return p;
}

MessageIdElement* MessageIdTable::find(Client* client, uint16_t clientMsgId)
{
    MessageIdElement* p = _head;
    while (p)
    {
        if (p->_clientMsgId == clientMsgId && p->_client == client)
        {
            break;
        }
        p = p->_next;
    }
    return p;
}

Client* MessageIdTable::getClientMsgId(uint16_t msgId, uint16_t* clientMsgId)
{
    Client* clt = nullptr;
    *clientMsgId = 0;
    _mutex.lock();
    MessageIdElement* p = find(msgId);
    if (p != nullptr)
    {
        clt = p->_client;
        *clientMsgId = p->_clientMsgId;
        clear(p);
    }
    _mutex.unlock();
    return clt;
}

void MessageIdTable::erase(uint16_t msgId)
{
    _mutex.lock();
    MessageIdElement* p = find(msgId);
    clear(p);
    _mutex.unlock();
}

void MessageIdTable::clear(MessageIdElement* elm)
{
    if (elm == nullptr)
    {
        return;
    }

    if (elm->_prev == nullptr)
    {
        _head = elm->_next;
        if (_head == nullptr)
        {
            _tail = nullptr;
        }
        else
        {
            _head->_prev = nullptr;
        }
        delete elm;
        _cnt--;
        return;
    }
    else
    {
        elm->_prev->_next = elm->_next;
        if (elm->_next == nullptr)
        {
            _tail = elm->_prev;
        }
        else
        {
            elm->_next->_prev = elm->_prev;
        }
        delete elm;
        _cnt--;
        return;
    }
}

uint16_t MessageIdTable::getMsgId(Client* client, uint16_t clientMsgId)
{
    uint16_t msgId = 0;
    MessageIdElement* p = find(client, clientMsgId);
    if (p != nullptr)
    {
        msgId = p->_msgId;
    }
    return msgId;
}

/*===============================
 * Class MessageIdElement
 ===============================*/
MessageIdElement::MessageIdElement(void) :
        _msgId { 0 }, _clientMsgId { 0 }, _client { nullptr }, _next { nullptr }, _prev { nullptr }
{

}

MessageIdElement::MessageIdElement(uint16_t msgId, Client* client, uint16_t clientMsgId) :
        MessageIdElement()
{
    _msgId = msgId;
    _client = client;
    _clientMsgId = clientMsgId;
}

MessageIdElement::~MessageIdElement(void)
{

}
