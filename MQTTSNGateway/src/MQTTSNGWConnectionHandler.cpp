/**************************************************************************************
 * Copyright (c) 2016, Tomoaki Yamaguchi
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Tomoaki Yamaguchi - initial API and implementation and/or initial documentation
 **************************************************************************************/

#include "MQTTSNGWConnectionHandler.h"
#include "MQTTSNGateway.h"
#include "MQTTSNGWPacket.h"
#include "MQTTGWPacket.h"

using namespace std;
using namespace MQTTSNGW;

/*=====================================
 Class MQTTSNConnectionHandler
 =====================================*/
MQTTSNConnectionHandler::MQTTSNConnectionHandler(Gateway* gateway)
{
	_gateway = gateway;
}

MQTTSNConnectionHandler::~MQTTSNConnectionHandler()
{

}

/*
 *  ADVERTISE
 */
void MQTTSNConnectionHandler::sendADVERTISE()
{
	MQTTSNPacket* adv = new MQTTSNPacket();
	adv->setADVERTISE(_gateway->getGWParams()->gatewayId, _gateway->getGWParams()->keepAlive);
	Event* ev1 = new Event();
	ev1->setBrodcastEvent(adv);  //broadcast
	_gateway->getClientSendQue()->post(ev1);
}

/*
 *  SEARCHGW
 */
void MQTTSNConnectionHandler::handleSearchgw(MQTTSNPacket* packet)
{
	if (packet->getType() == MQTTSN_SEARCHGW)
	{
		if (_gateway->getClientList()->getClientCount() < DEFAULT_MAX_CLIENTS)
		{
			MQTTSNPacket* gwinfo = new MQTTSNPacket();
			gwinfo->setGWINFO(_gateway->getGWParams()->gatewayId);
			Event* ev1 = new Event();
			ev1->setBrodcastEvent(gwinfo);
			_gateway->getClientSendQue()->post(ev1);
		}
	}
}

/*
 *  CONNECT
 */
void MQTTSNConnectionHandler::handleConnect(Client* client, MQTTSNPacket* packet)
{
	MQTTSNPacket_connectData data;
	packet->getCONNECT(&data);

	/* clear ConnectData of Client */
	Connect* connectData = client->getConnectData();
	memset(connectData, 0, sizeof(Connect));
	client->disconnected();

	Topics* topics = client->getTopics();

	/* CONNECT was not sent yet. prepare Connect data */
	connectData->header.bits.type = CONNECT;
	connectData->clientID = client->getClientId();
	connectData->version = _gateway->getGWParams()->mqttVersion;
	connectData->keepAliveTimer = data.duration;
	connectData->flags.bits.will = data.willFlag;

	if ((const char*) _gateway->getGWParams()->loginId != 0 && (const char*) _gateway->getGWParams()->password != 0)
	{
		connectData->flags.bits.password = 1;
		connectData->flags.bits.username = 1;
	}

	if (data.cleansession)
	{
		connectData->flags.bits.cleanstart = 1;
		/* reset the table of msgNo and TopicId pare */
		client->clearWaitedPubTopicId();
		client->clearWaitedSubTopicId();

		/* renew the TopicList */
		if (topics)
		{
			delete topics;
		}
		topics = new Topics();
		client->setTopics(topics);
	}

	if (data.willFlag)
	{
		/* create & send WILLTOPICREQ message to the client */
		MQTTSNPacket* reqTopic = new MQTTSNPacket();
		reqTopic->setWILLTOPICREQ();
		Event* evwr = new Event();
		evwr->setClientSendEvent(client, reqTopic);

		/* Send WILLTOPICREQ to the client */
		_gateway->getClientSendQue()->post(evwr);
	}
	else
	{

		/* CONNECT message was not qued in.
		 * create CONNECT message & send it to the broker */
		MQTTGWPacket* mqMsg = new MQTTGWPacket();
		mqMsg->setCONNECT(client->getConnectData(), (unsigned char*)_gateway->getGWParams()->loginId, (unsigned char*)_gateway->getGWParams()->password);
		Event* ev1 = new Event();
		ev1->setBrokerSendEvent(client, mqMsg);
		_gateway->getBrokerSendQue()->post(ev1);
	}
}

/*
 *  WILLTOPIC
 */
void MQTTSNConnectionHandler::handleWilltopic(Client* client, MQTTSNPacket* packet)
{
	int willQos;
	uint8_t willRetain;
	MQTTSNString willTopic;

	packet->getWILLTOPIC(&willQos, &willRetain, &willTopic);
	client->setWillTopic(willTopic);
	Connect* connectData = client->getConnectData();

	/* add the connectData for MQTT CONNECT message */
	connectData->willTopic = client->getWillTopic();
	connectData->flags.bits.willQoS = willQos;
	connectData->flags.bits.willRetain = willRetain;

	/* Send WILLMSGREQ to the client */
	client->setWaitWillMsgFlg(true);
	MQTTSNPacket* reqMsg = new MQTTSNPacket();
	reqMsg->setWILLMSGREQ();
	Event* evt = new Event();
	evt->setClientSendEvent(client, reqMsg);
	_gateway->getClientSendQue()->post(evt);
}

/*
 *  WILLMSG
 */
void MQTTSNConnectionHandler::handleWillmsg(Client* client, MQTTSNPacket* packet)
{
	if ( !client->isWaitWillMsg() )
	{
		DEBUGLOG("     MQTTSNConnectionHandler::handleWillmsg  WaitWillMsgFlg is off.\n");
		if ( !client->isSecureNetwork() )
		{
			/* create CONNACK message */
			MQTTSNPacket* connack =  new MQTTSNPacket();
			connack->setCONNACK(MQTTSN_RC_REJECTED_CONGESTED);

			/* return to the client */
			Event* evt = new Event();
			evt->setClientSendEvent(client, connack);
			_gateway->getClientSendQue()->post(evt);
		}
		return;
	}

	MQTTSNString willmsg;
	Connect* connectData = client->getConnectData();

	if( client->isConnectSendable() )
	{
		/* save WillMsg in the client */
		packet->getWILLMSG(&willmsg);
		client->setWillMsg(willmsg);

		/* create CONNECT message */
		MQTTGWPacket* mqttPacket =  new MQTTGWPacket();
		connectData->willMsg = client->getWillMsg();
		mqttPacket->setCONNECT(connectData, (unsigned char*)_gateway->getGWParams()->loginId, (unsigned char*)_gateway->getGWParams()->password);

		/* Send CONNECT to the broker */
		Event* evt = new Event();
		evt->setBrokerSendEvent(client, mqttPacket);
		_gateway->getBrokerSendQue()->post(evt);
	}
}

/*
 *  DISCONNECT
 */
void MQTTSNConnectionHandler::handleDisconnect(Client* client, MQTTSNPacket* packet)
{
	MQTTGWPacket* mqMsg = new MQTTGWPacket();
	MQTTSNPacket* snMsg = new MQTTSNPacket();

	mqMsg->setHeader(DISCONNECT);
	snMsg->setDISCONNECT(0);

	Event* ev1 = new Event();
	ev1->setClientSendEvent(client, snMsg);
	_gateway->getClientSendQue()->post(ev1);

	ev1 = new Event();
	ev1->setBrokerSendEvent(client, mqMsg);
	_gateway->getBrokerSendQue()->post(ev1);
}

/*
 *  WILLTOPICUPD
 */
void MQTTSNConnectionHandler::handleWilltopicupd(Client* client, MQTTSNPacket* packet)
{
	/* send NOT_SUPPORTED responce to the client */
	MQTTSNPacket* respMsg = new MQTTSNPacket();
	respMsg->setWILLTOPICRESP(MQTTSN_RC_NOT_SUPPORTED);
	Event* evt = new Event();
	evt->setClientSendEvent(client, respMsg);
	_gateway->getClientSendQue()->post(evt);
}

/*
 *  WILLMSGUPD
 */
void MQTTSNConnectionHandler::handleWillmsgupd(Client* client, MQTTSNPacket* packet)
{
	/* send NOT_SUPPORTED responce to the client */
	MQTTSNPacket* respMsg = new MQTTSNPacket();
	respMsg->setWILLMSGRESP(MQTTSN_RC_NOT_SUPPORTED);
	Event* evt = new Event();
	evt->setClientSendEvent(client, respMsg);
	_gateway->getClientSendQue()->post(evt);
}

/*
 *  PINGREQ
 */
void MQTTSNConnectionHandler::handlePingreq(Client* client, MQTTSNPacket* packet)
{
	/* send PINGREQ to the broker */
	MQTTGWPacket* pingreq = new MQTTGWPacket();
	pingreq->setHeader(PINGREQ);
	Event* evt = new Event();
	evt->setBrokerSendEvent(client, pingreq);
	_gateway->getBrokerSendQue()->post(evt);
}
