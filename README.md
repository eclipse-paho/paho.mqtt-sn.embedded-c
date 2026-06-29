# Eclipse Paho MQTT-SN C/C++ client for Embedded platforms

This repository contains the source code for the [Eclipse Paho](http://eclipse.org/paho) MQTT-SN C/C++ client library for Embedded platorms.

It is dual licensed under the EPL 2.0 and EDL (see about.html and notice.html for more details).  You can choose which of these licenses you want to use the code under.  The EDL allows you to embed the code into your application, and distribute your application in binary or source form without contributing any of your code, or any changes you make back to Paho.  See the EDL for the exact conditions.

There are four sub-projects:

1. MQTTSNPacket - simple de/serialization of MQTT-SN 1.2 packets, plus helper functions
2. MQTTSN2Packet - de/serialization of MQTT-SN 2.0 packets, with samples and tests
3. MQTTGateway - MQTT-SN 1.2 transparent/aggregating gateway - connects MQTT-SN clients with an MQTT server.  See the README within the project for more information.
4. MQTTSNClient - MQTT-SN 1.2 high(er) level C++ client (not yet complete)

The *MQTTSNPacket* directory contains the lowest level C library with the smallest requirements.  This supplies simple serialization
and deserialization routines.  They serve as a base for the higher level libraries, but can also be used on their own.
It is mainly up to you to write and read to and from the network.

See README.md in the MQTTSN2Packet directory for more information about this sub-project. 

The *MQTTSNGateway* directory contains an MQTT-SN to MQTT transparent/aggregating gateway (see the MQTT-SN specification for a description of that.)  It can
be used to connect the MQTT-SN client to an MQTT server.

The *MQTTSNClient* directory contains the next level C++ library.  This is intended to mirror the way the MQTTClient works in the Paho embedded
MQTT project, but it's not yet complete.

## Build requirements / compilation

CMake builds have been introduced, along a configuration for automated build & testing.

The cmake-build.sh file has the full build and test sequence for Linux and MacOS.

## Usage and API

See the samples directories for examples of intended use.  Doxygen config files are available in the doc directory.

## Runtime tracing


## Reporting bugs

This project uses GitHub Issues here: [github.com/eclipse/paho.mqtt-sn.embedded-c/issues](https://github.com/eclipse/paho.mqtt-sn.embedded-c/issues) to track ongoing development and issues.

## More information

Discussion of the Paho clients takes place on the [Eclipse Mattermost Paho channel](https://mattermost.eclipse.org/eclipse/channels/paho) and the [Eclipse paho-dev mailing list](https://dev.eclipse.org/mailman/listinfo/paho-dev).

General questions about the MQTT protocol are discussed in the [MQTT Google Group](https://groups.google.com/forum/?hl=en-US&fromgroups#!forum/mqtt).

More information is available via the [MQTT community](http://mqtt.org).

