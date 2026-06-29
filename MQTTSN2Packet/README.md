# Eclipse Paho MQTT-SN 2.0 C client for Embedded platforms

This project aims to provide a simple portable library for MQTT-SN 2.0 implementations in C.

It provides functions to serialize MQTT-SN data into network ready packet buffers, and deserialize
incoming packet buffers into their component MQTT-SN data fields. It also provides tests and 
samples to validate and illustrate how to use these functions. 

There is a CMake build which can be invoked using the cmake-build.sh script, which works on
Linux and MacOS.

Currently, the tests and samples use UDP to communicate. It is intended that the library be 
portable to other transmission mechanisms, such as Bluetooth, as well.

There are two samples which are similar to command line MQTT clients such as mosquitto_pub
and mosquitto_sub (or the Paho C client paho_c_pub and paho_c_sub). Their names are
paho_sn_pub and paho_sn_sub. 

Some of the tests are both standalone, others require an MQTT-SN 2.0 UDP server. This can
be found in the https://github.com/eclipse-paho/paho.mqtt.testing project, by running a broker
with the udp.conf configuration. See that project for more information.