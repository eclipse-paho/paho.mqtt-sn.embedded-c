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
#include "MQTTSNGWAggregateTopicTable.h"
#include "MQTTSNGWClient.h"

/*=====================================
 Class ClientTopicElement
 =====================================*/
ClientTopicElement::ClientTopicElement(Client* client)
{
    _client = client;
}

ClientTopicElement::~ClientTopicElement()
{

}

Client* ClientTopicElement::getClient(void)
{
    return _client;
}

ClientTopicElement* ClientTopicElement::getNextClientElement(void)
{
    return _next;
}

/*=====================================
 Class AggregateTopicElement
 =====================================*/
AggregateTopicElement::AggregateTopicElement(void)
{

}

AggregateTopicElement::AggregateTopicElement(Topic* topic, Client* client)
{
    _topic = topic;
    ClientTopicElement* elm = new ClientTopicElement(client);
    if (elm != nullptr)
    {
        _head = elm;
        _tail = elm;
    }
}

AggregateTopicElement::~AggregateTopicElement(void)
{
    _mutex.lock();
    if (_head != nullptr)
    {
        ClientTopicElement* p = _tail;
        while (p)
        {
            ClientTopicElement* pPrev = p;
            delete p;
            p = pPrev->_prev;
        }
        _head = _tail = nullptr;
    }
    _mutex.unlock();
}

ClientTopicElement* AggregateTopicElement::add(Client* client)
{
    ClientTopicElement* elm = new ClientTopicElement(client);
    if (elm == nullptr)
    {
        return nullptr;
    }

    _mutex.lock();

    if (_head == nullptr)
    {
        _head = elm;
        _tail = elm;
    }
    else
    {
        ClientTopicElement* p = find(client);
        if (p == nullptr)
        {
            p = _tail;
            _tail = elm;
            elm->_prev = p;
            p->_next = elm;
        }
        else
        {
            delete elm;
            elm = p;
        }
    }
    _mutex.unlock();
    return elm;
}

ClientTopicElement* AggregateTopicElement::find(Client* client)
{
    ClientTopicElement* p = _head;
    while (p != nullptr)
    {
        if (p->_client == client)
        {
            break;
        }
        p = p->_next;
    }
    return p;
}

ClientTopicElement* AggregateTopicElement::getFirstClientTopicElement(void)
{
    return _head;
}

ClientTopicElement* AggregateTopicElement::getNextClientTopicElement(ClientTopicElement* elmClient)
{
    return elmClient->_next;
}

void AggregateTopicElement::eraseClient(Client* client)
{
    _mutex.lock();

    ClientTopicElement* p = find(client);
    if (p != nullptr)
    {
        if (p->_prev == nullptr)    // head element
        {
            _head = p->_next;
            if (p->_next == nullptr)   // head & only one
            {
                _tail = nullptr;
            }
            else
            {
                p->_next->_prev = nullptr;  // head & midle
            }
        }
        else if (p->_next != nullptr)  // middle
        {
            p->_prev->_next = p->_next;
        }
        else    // tail
        {
            p->_prev->_next = nullptr;
            _tail = p->_prev;
        }
        delete p;
    }
    _mutex.unlock();
}

/*=====================================
 Class AggregateTopicTable
 ======================================*/

AggregateTopicTable::AggregateTopicTable()
{

}

AggregateTopicTable::~AggregateTopicTable()
{

}

AggregateTopicElement* AggregateTopicTable::add(Topic* topic, Client* client)
{
    AggregateTopicElement* elm = nullptr;
    _mutex.lock();
    elm = getAggregateTopicElement(topic);
    if (elm != nullptr)
    {
        if (elm->find(client) == nullptr)
        {
            elm->add(client);
        }
    }
    else
    {
        Topic* newTopic = topic->duplicate();
        elm = new AggregateTopicElement(newTopic, client);
        if (_head == nullptr)
        {
            _head = elm;
            _tail = elm;
        }
        else
        {
            elm->_prev = _tail;
            _tail->_next = elm;
            _tail = elm;
        }
    }
    _mutex.unlock();
    return elm;
}

void AggregateTopicTable::erase(Topic* topic, Client* client)
{
    AggregateTopicElement* elm = nullptr;

    _mutex.lock();
    elm = getAggregateTopicElement(topic);

    if (elm != nullptr)
    {
        elm->eraseClient(client);
    }
    if (elm->_head == nullptr)
    {
        erase(elm);
    }
    _mutex.unlock();
    return;
}

void AggregateTopicTable::erase(AggregateTopicElement* elmTopic)
{
    if (elmTopic != nullptr)
    {
        if (elmTopic->_prev == nullptr)    // head element
        {
            _head = elmTopic->_next;
            if (elmTopic->_next == nullptr)   // head & only one
            {
                _tail = nullptr;
            }
            else
            {
                elmTopic->_next->_prev = nullptr;  // head & midle
            }
        }
        else if (elmTopic->_next != nullptr)  // middle
        {
            elmTopic->_prev->_next = elmTopic->_next;
        }
        else    // tail
        {
            elmTopic->_prev->_next = nullptr;
            _tail = elmTopic->_prev;
        }
        delete elmTopic;
    }
}

AggregateTopicElement* AggregateTopicTable::getAggregateTopicElement(Topic* topic)
{
    AggregateTopicElement* elm = _head;

    while (elm != nullptr)
    {
        if (elm->_topic->isMatch(topic->_topicName))
        {
            break;
        }
        elm = elm->_next;
    }
    return elm;
}

ClientTopicElement* AggregateTopicTable::getClientElement(Topic* topic)
{
    AggregateTopicElement* elm = getAggregateTopicElement(topic);
    if (elm != nullptr)
    {
        return elm->_head;
    }
    else
    {
        return nullptr;
    }
}

void AggregateTopicTable::print(void)
{
    AggregateTopicElement* elm = _head;

    printf("Beginning of AggregateTopicTable\n");
    while (elm != nullptr)
    {
        printf("%s\n", elm->_topic->getTopicName()->c_str());

        ClientTopicElement* clElm = elm->getFirstClientTopicElement();
        Client* client = clElm->getClient();

        while (client != nullptr)
        {
            printf("    %s\n", client->getClientId());
            clElm = clElm->getNextClientElement();
            if (clElm != nullptr)
            {
                client = clElm->getClient();
            }
            else
            {
                client = nullptr;
            }
        }
        elm = elm->_next;
    }
    printf("End of AggregateTopicTable\n");
}
