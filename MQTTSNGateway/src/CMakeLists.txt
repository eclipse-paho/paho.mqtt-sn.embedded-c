#*******************************************************************************
#  Copyright (c) 2022 a1lu
#
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v2.0
#  and Eclipse Distribution License v1.0 which accompany this distribution.
#
#  The Eclipse Public License is available at
#     http://www.eclipse.org/legal/epl-v20.html
#  and the Eclipse Distribution License is available at
#    http://www.eclipse.org/org/documents/edl-v10.php.
#
#  Contributors:
#     a1lu  - initial version
#     ty4tw - modify
#*******************************************************************************/
PROJECT(mqtt-sn-gateway CXX)

SET(GW_VERSION_MAJOR 1)
SET(GW_VERSION_MINOR 5)
SET(GW_VERSION_PATCH 1)

SET(GATEWAY_VERSION ${GW_VERSION_MAJOR}.${GW_VERSION_MINOR}.${GW_VERSION_PATCH})
MESSAGE(STATUS "VERSION : ${GATEWAY_VERSION}")

configure_file( MQTTSNGWVersion.h.in ${CMAKE_CURRENT_SOURCE_DIR}/MQTTSNGWVersion.h )

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../bin)
SET(CMAKE_CXX_STANDARD 11)

SET(OS linux)

IF(NOT DEFINED SENSORNET)
    SET(SENSORNET udp)
ENDIF()
MESSAGE(STATUS "SENSORNET: " ${SENSORNET})

ADD_DEFINITIONS(${DEFS})
MESSAGE(STATUS "Definitions: " ${DEFS})

ADD_LIBRARY(mqtt-sngateway_common
       MQTTGWConnectionHandler.cpp
       MQTTGWPacket.cpp
       MQTTGWPublishHandler.cpp
       MQTTGWSubscribeHandler.cpp
       MQTTSNGateway.cpp
       MQTTSNGWBrokerRecvTask.cpp
       MQTTSNGWBrokerSendTask.cpp
       MQTTSNGWClient.cpp
       MQTTSNGWClientRecvTask.cpp
       MQTTSNGWClientSendTask.cpp
       MQTTSNGWConnectionHandler.cpp
       MQTTSNGWLogmonitor.cpp
       MQTTSNGWPacket.cpp
       MQTTSNGWPacketHandleTask.cpp
       MQTTSNGWProcess.cpp
       MQTTSNGWPublishHandler.cpp
       MQTTSNGWSubscribeHandler.cpp
       MQTTSNGWEncapsulatedPacket.cpp
       MQTTSNGWForwarder.cpp
       MQTTSNGWQoSm1Proxy.cpp
       MQTTSNGWAdapter.cpp
       MQTTSNGWAggregater.cpp
       MQTTSNGWClientList.cpp
       MQTTSNGWTopic.cpp
       MQTTSNGWAdapterManager.cpp
       MQTTSNAggregateConnectionHandler.cpp
       MQTTSNGWMessageIdTable.cpp
       MQTTSNGWAggregateTopicTable.cpp
       ${OS}/${SENSORNET}/SensorNetwork.cpp
       ${OS}/${SENSORNET}/SensorNetwork.h
       ${OS}/Timer.cpp
       ${OS}/Timer.h
       ${OS}/Network.cpp
       ${OS}/Network.h
       ${OS}/Threading.cpp
       ${OS}/Threading.h
       )

# linux
link_directories("/usr/local/lib")

# Mac
link_directories("/usr/local/opt/openssl")


TARGET_INCLUDE_DIRECTORIES(mqtt-sngateway_common
       PUBLIC
       .
       ${OS}
       ${OS}/${SENSORNET}
       ../../MQTTSNPacket/src
       /usr/local/include
       )

IF(SENSORNET MATCHES "rfcomm")

TARGET_LINK_LIBRARIES(mqtt-sngateway_common
       PRIVATE
       MQTTSNPacket
       pthread
       ssl
       crypto
       bluetooth
       )
ELSE()

TARGET_LINK_LIBRARIES(mqtt-sngateway_common
       PRIVATE
       MQTTSNPacket
       pthread
       ssl
       crypto
       )
       
ENDIF()

ADD_EXECUTABLE(MQTT-SNGateway
       mainGateway.cpp
       )

TARGET_LINK_LIBRARIES(MQTT-SNGateway
       mqtt-sngateway_common
       )

ADD_EXECUTABLE(MQTT-SNLogmonitor
       mainLogmonitor.cpp
       )

TARGET_LINK_LIBRARIES(MQTT-SNLogmonitor
       mqtt-sngateway_common
       )

ADD_EXECUTABLE(testPFW
       tests/mainTestProcess.cpp
       tests/TestProcess.cpp
       tests/TestQue.cpp
       tests/TestTree23.cpp
       tests/TestTopics.cpp
       tests/TestTopicIdMap.cpp
       tests/TestTask.cpp
       )
TARGET_LINK_LIBRARIES(testPFW
       mqtt-sngateway_common
       )


ADD_TEST(NAME testPFW
       WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
       COMMAND testPFW -f ./gateway.conf)
