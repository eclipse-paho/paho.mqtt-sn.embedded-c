# MQTT-SN Transparent / Aggregating Gateway
MQTT-SN requires a MQTT-SN Gateway which acts as a protocol converter to convert MQTT-SN messages to MQTT messages.
MQTT-SN client over SensorNetwork can not communicate directly with MQTT broker(TCP/IP). 
This Gateway can run as a transparent or aggregating Gateway by specifying the gateway.conf. 
### step1. Build the gateway
copy and expand source code then, 
```
$ cd paho.mqtt-sn.embedded-c/MQTTSNGateway 
```
In order to build a gateway, one sensor network argument is required. 
```
$ ./build.sh [udp|udp6|xbee|loralink|rfcomm|dtls|dtls6]  
```     

MQTT-SNGateway and MQTT-SNLogmonitor (executable programs) are built in ./bin directory.

### step2. Execute the Gateway.    

``` 
$ cd bin 
$ ./MQTT-SNGateway 
```
If you get the error message as follows:
 
RingBuffer can't create a shared memory. ABORT Gateway!!! 
You have to start using sudo command only once for the first time.    
```
$ sudo ./MQTT-SNGateway 
```
## Contents of the gateway configuration file
**gateway.conf** is in bin directory.  It's contents are follows: 

```
#**************************************************************************
# Copyright (c) 2016-2021, Tomoaki Yamaguchi
#
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# and Eclipse Distribution License v1.0 which accompany this distribution.
#
# The Eclipse Public License is available at
#    http://www.eclipse.org/legal/epl-v10.html
# and the Eclipse Distribution License is available at
#   http://www.eclipse.org/org/documents/edl-v10.php.
#***************************************************************************
#
# config file of MQTT-SN Gateway
#

GatewayID=1
GatewayName=PahoGateway-01
MaxNumberOfClients=30
KeepAlive=60
#LoginID=your_ID
#Password=your_Password

BrokerName=mqtt.eclipseprojects.io
BrokerPortNo=1883
BrokerSecurePortNo=8883
```
**GatewayID** is a gateway ID which  used by GWINFO message.    
**GatewayName** is a name of the gateway.    
**MaxNumberOfClients** is a maxmum number of clients. Clients are dynamically allocated.    
**KeepAlive** is KeepAlive time in seconds.   
**LoginID** is used by CONNECT message.  
**Password** is used by CONNECT message.    
**BrokerName**is a domain name or IP address of a broker.    
**BrokerPortNo** is a broker's port no.    
**BrokerSecurePortNo** is a broker's port no of TLS connection.    
```
#
# CertKey for TLS connections to a broker
#

#RootCAfile=/etc/ssl/certs/ca-certificates.crt
#RootCApath=/etc/ssl/certs/
#CertKey=/path/to/certKey.pem
#PrivateKey=/path/to/privateKey.pem
```
**RootCAfile** is a CA file name.    
**RootCApath** is a CA path. **SSL_CTX_load_verify_locations(ctx, CAfile, CApath)** function requires these parameters.        
**CertKey** is a certificate pem file.
**PrivateKey** is a private key pem file.   
Clients can connect to the broker via TLS by setting '**Secure Connection**' for each client in the client conf file.   
```
#
# When AggregatingGateway=YES or ClientAuthentication=YES,
# All clients must be specified by the ClientList File  
#

AggregatingGateway=NO
QoS-1=NO
Forwarder=NO
PredefinedTopic=NO
ClientAuthentication=NO

ClientsList=/path/to/your_clients.conf
PredefinedTopicList=/path/to/your_predefinedTopic.conf
```
The gateway runs as a aggregating gateway when **AggregatingGateway** is 'YES'.   
If **QoS-1** is 'YES, the gateway prepares a proxy for the QoS-1 client.　QoS-1 client has a 'QoS-1' parameter in a clients.conf file.　For QoS-1 clients, set the QoS-1 parameters in the clients.conf file.
If **Forwarder** is 'YES', the gateway prepare a forwarder agent.   
If **ClientAuthentication** is 'YES', the client cannot connect unless it is registered in the clients.conf file.  
**ClientsList** defines clients and those address so on.    
**PredefinedTopicList** file defines Predefined Topic.    


```
#==============================
#  SensorNetworks parameters
#==============================
#
# UDP | DTLS 
# 

GatewayPortNo=10000
MulticastPortNo=1883
MulticastIP=225.1.1.1
MulticastTTL=1
```
**GatewayPortNo** is a unicast port no of the gateway.  
**MulticastIP** and **MulticastPortNo** are for GWSEARCH messages. Clients can get the gateway address (Gateway IP address and GatewayPortNo) from GWINFO message by means of std::recvfrom().   
Client needs to know the MulticastIP and MulticastPortNo to send a SEARCHGW message. 
**MulticastTTL** is a multicast TTL.    
```
#
# UDP6 | DTLS6
#

GatewayIPv6PortNo=10000
MulticastIPv6PortNo=1883
MulticastIPv6=ff1e:feed:caca:dead::feed:caca:dead
MulticastIPv6If=wlp4s0
MulticastHops=1
```
**GatewayIPv6PortNo** is a unicast port no of the gateway.
**MulticastIPv6PortNo** and **MulticastIPv6** are for GWSEARCH messages. Set the Global scope Multicast address so that the Global address is used for sending GWINFO.   
Clients can get the gateway address (Gateway IPv6 address and GatewayPortNo) from GWINFO message by means of std::recvfrom(). 
**MulticastIPv6If** is a  multicast interface name.    
**MulticastHops** is a multicast hops.    
```
#
# DTLS | DTLS6  DTLS CertsKey  
#

DtlsCertsKey=/etc/ssl/certs/gateway.pem
DtlsPrivKey=/etc/ssl/private/privkey.pem
```
**DtlsCertsKey** is a certs Key pem file for DTLS connection.        
**DtlsPrivKey** is a private key pem file for DTLS connection.    
```
#
# XBee
#

Baudrate=38400
SerialDevice=/dev/ttyUSB0
ApiMode=2
```
**Baudrate** is a baudrate of xbee.    
```
#
# LoRaLink
#

BaudrateLoRaLink=115200
DeviceRxLoRaLink=/dev/loralinkRx
DeviceTxLoRaLink=/dev/loralinkTx
```
https://github.com/ty4tw/MQTT-SN-LoRa    

```
#
# Bluetooth RFCOMM
#

RFCOMMAddress=60:57:18:06:8B:72.*
```
**RFCOMMAddress** is a bluetooth mac address and channel. channel should be * for the gateway.
```
#
# LOG
#

ShearedMemory=NO
```

### How to monitor the gateway from a remote terminal.
Change gateway.conf as follows:
```
# LOG
ShearedMemory=YES;
```
Restart the gateway with sudo only once to create shared memories. 
open ssh terminal and execute LogMonitor.
```
 $ cd bin
 $ ./MQTT-SNLogmonitor
```
Now you can get the Log on your terminal.


##### Tips 
Use compiler definitions then you can get more precise logs.    
**-DDEBUG_NW** is a flag for debug logs of Sensor network.   
**-DDEBUG_MQTTSN** is a flag for debug logs of MQTT-SN message haandling.    
One or both flags can be specified.    

```
./build.sh udp -DDEBUG -DDEBUG_NW
```
