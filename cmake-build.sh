#!/bin/bash

set -e

rm -rf build.paho
mkdir build.paho
cd build.paho
cmake .. -DSENSORNET=loralink
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
#cmake .. -DSENSORNET=udp
#make MQTT-SNGateway
#cd ../MQTTSNGateway/GatewayTester
#make SENSORNET=UDP6
#make SENSORNET=DTLS
#make SENSORNET=DTLS6
#make SENSORNET=RFCOMM
#make SENSORNET=UDP


