#!/bin/bash

set -e

rm -rf build.paho
mkdir build.paho
cd build.paho
cmake .. -DSENSORNET=udp
make
ctest -VV --timeout 600
#cmake .. -DSENSORNET=rfcomm
#make MQTT-SNGateway
#cmake .. -DSENSORNET=xbee
#make MQTT-SNGateway
#cmake .. -DSENSORNET=udp6
#make MQTT-SNGateway
#cmake .. -DSENSORNET=dtls
#make MQTT-SNGateway
#cmake .. -DSENSORNET=loralink
#make MQTT-SNGateway
cd ../MQTTSNGateway/GatewayTester
#make SN=UDP6
#make SN=DTLS
#make SN=DTLS6
#make SN=RFCOMM
make SN=UDP


