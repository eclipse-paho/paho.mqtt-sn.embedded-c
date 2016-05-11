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

#include "MQTTSNGWClientRecvTask.h"
#include "MQTTSNGateway.h"
char* currentDateTime(void);
/*=====================================
 Class ClientRecvTask
 =====================================*/

ClientRecvTask::ClientRecvTask(Gateway* gateway)
{
	_gateway = gateway;
	_gateway->attach((Thread*)this);
	_sensorNetwork = _gateway->getSensorNetwork();
}

ClientRecvTask::~ClientRecvTask()
{

}

/**
 * Initialize SensorNetwork
 */
void ClientRecvTask::initialize(int argc, char** argv)
{
	char param[MQTTSNGW_PARAM_MAX];
	uint16_t multicastPortNo = 0;
	uint16_t unicastPortNo = 0;
	char* ipAddress = NULL;

	if (_gateway->getParam("MulticastIP", param) == 0)
	{
		ipAddress = strdup(param);
	}
	if (_gateway->getParam("MulticastPortNo", param) == 0)
	{
		multicastPortNo = atoi(param);
	}
	if (_gateway->getParam("GatewayPortNo", param) == 0)
	{
		unicastPortNo = atoi(param);
	}
	_sensorNetwork->initialize(ipAddress, multicastPortNo, unicastPortNo);
	free(ipAddress);
}

/*
 * Receive a packet from clients via sensor netwwork
 * and generate a event to execute the packet handling  procedure
 * of MQTTSNPacketHandlingTask.
 */
void ClientRecvTask::run()
{
	Event* ev = 0;
	Client* client = 0;

	while (true)
	{


		MQTTSNPacket* packet = new MQTTSNPacket();
		int packetLen = packet->recv(_sensorNetwork);

		if (packetLen < 3 )
		{
			delete packet;
			continue;
		}

		if ( packet->getType() <= MQTTSN_ADVERTISE || packet->getType() == MQTTSN_GWINFO )
		{
			delete packet;
			continue;
		}

		if ( packet->getType() == MQTTSN_SEARCHGW )
		{
			/* write log and post Event */
			log(0, packet);
			ev = new Event();
			ev->setBrodcastEvent(packet);
			_gateway->getPacketEventQue()->post(ev);
			continue;
		}

		/* get client from the ClientList of Gateway by sensorNetAddress. */
		client = _gateway->getClientList()->getClient(_sensorNetwork->getSenderAddress());

		if ( client )
		{
			/* write log and post Event */
			log(client, packet);
			ev = new Event();
			ev->setClientRecvEvent(client,packet);
			_gateway->getPacketEventQue()->post(ev);
		}
		else
		{
			/* new client */
			if (packet->getType() == MQTTSN_CONNECT)
			{
				MQTTSNPacket_connectData data;
				memset(&data, 0, sizeof(MQTTSNPacket_connectData));
				packet->getCONNECT(&data);

				/* create a client */
				client = _gateway->getClientList()->createClient(_sensorNetwork->getSenderAddress(), &data.clientID, false, false);

				if (!client)
				{
					WRITELOG("%s Can't create a Client. CONNECT message has been discarded.\n", ERRMSG_HEADER);
					delete packet;
					continue;
				}

				log(client, packet);

				/* set sensorNetAddress & post Event */
				client->setClientAddress(_sensorNetwork->getSenderAddress());
				ev = new Event();
				ev->setClientRecvEvent(client, packet);
				_gateway->getPacketEventQue()->post(ev);
			}
			else
			{
				log(client, packet);
				delete packet;
				continue;
			}
		}
	}
}

void ClientRecvTask::log(Client* client, MQTTSNPacket* packet)
{
	char pbuf[SIZEOF_LOG_PACKET * 3];
	char msgId[6];
	const char* clientId = client ? (const char*)client->getClientId() :"Non Active Client !" ;

	switch (packet->getType())
	{
	case MQTTSN_SEARCHGW:
		WRITELOG(FORMAT_CY_NL, currentDateTime(), packet->getName(), LEFTARROW, CLIENT, packet->print(pbuf));
		break;
	case MQTTSN_CONNECT:
		WRITELOG(FORMAT_YE_WH_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_WILLTOPIC:
	case MQTTSN_WILLMSG:
	case MQTTSN_DISCONNECT:
	case MQTTSN_WILLTOPICUPD:
	case MQTTSN_WILLMSGUPD:
		WRITELOG(FORMAT_WHITE_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_PUBLISH:
	case MQTTSN_REGISTER:
	case MQTTSN_SUBSCRIBE:
	case MQTTSN_UNSUBSCRIBE:
		WRITELOG(FORMAT_WH_MSGID_NL, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_REGACK:
	case MQTTSN_PUBACK:
	case MQTTSN_PUBREC:
	case MQTTSN_PUBREL:
	case MQTTSN_PUBCOMP:
		WRITELOG(FORMAT_WH_MSGID, currentDateTime(), packet->getName(), packet->getMsgId(msgId), LEFTARROW, clientId, packet->print(pbuf));
		break;
	case MQTTSN_PINGREQ:
		WRITELOG(FORMAT_WH_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	default:
		WRITELOG(FORMAT_WH_NL, currentDateTime(), packet->getName(), LEFTARROW, clientId, packet->print(pbuf));
		break;
	}
}
