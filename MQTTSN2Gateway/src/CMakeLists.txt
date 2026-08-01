#*******************************************************************************
#  Copyright (c) 2022, 2026 a1lu, Ian Craggs
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
PROJECT(mqtt-sn2-gateway CXX)

SET(GW_VERSION_MAJOR 2)
SET(GW_VERSION_MINOR 0)
SET(GW_VERSION_PATCH 0)

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

ADD_LIBRARY(mqtt-sn2gateway_common
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
       MQTTSNGWProtection.cpp
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

TARGET_INCLUDE_DIRECTORIES(mqtt-sn2gateway_common
       PUBLIC
       .
       ${OS}
       ${OS}/${SENSORNET}
       ../../MQTTSN2Packet/src
       /usr/local/include
       /opt/homebrew/opt/openssl@3/include
       )

IF(SENSORNET MATCHES "rfcomm")

TARGET_LINK_LIBRARIES(mqtt-sn2gateway_common
       PRIVATE
       MQTTSN2Packet
       pthread
       bluetooth
       /opt/homebrew/opt/openssl@3/lib/libcrypto.dylib
       )
ELSE()

TARGET_LINK_LIBRARIES(mqtt-sn2gateway_common
       PRIVATE
       MQTTSN2Packet
       pthread
       /opt/homebrew/opt/openssl@3/lib/libcrypto.dylib
       )

ENDIF()

ADD_EXECUTABLE(MQTT-SN2Gateway
       mainGateway.cpp
       )

TARGET_LINK_LIBRARIES(MQTT-SN2Gateway
       mqtt-sn2gateway_common
       )

TARGET_INCLUDE_DIRECTORIES(MQTT-SN2Gateway
        PUBLIC
        .
        ${OS}
        ${OS}/${SENSORNET}
        ../../MQTTSN2Packet/src
        /usr/local/include
        /opt/homebrew/opt/openssl@3/include
)

ADD_EXECUTABLE(MQTT-SN2Logmonitor
       mainLogmonitor.cpp
       )

TARGET_LINK_LIBRARIES(MQTT-SN2Logmonitor
       mqtt-sn2gateway_common
       )

ADD_EXECUTABLE(test2PFW
       tests/mainTestProcess.cpp
       tests/TestProcess.cpp
       tests/TestQue.cpp
       tests/TestTree23.cpp
       tests/TestTopics.cpp
       tests/TestTopicIdMap.cpp
       tests/TestTask.cpp
       )
TARGET_LINK_LIBRARIES(test2PFW
       mqtt-sn2gateway_common
       )

ADD_TEST(NAME test2PFW
       WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
       COMMAND test2PFW -f ./gateway.conf)

# Gateway integration tests: test22 and test23 from MQTTSN2Packet exercise the
# gateway over UDP. They require a running MQTT broker on localhost:1883 (TCP).
# These tests are only registered when the MQTTSN2Packet test targets exist
# (i.e. when building from the top-level CMakeLists.txt).
#
# The broker is started automatically from PAHO_MQTT_TESTING_DIR using:
#   python3 startbroker.py -c localhost_testing.conf
# Override on the cmake command line: -DPAHO_MQTT_TESTING_DIR=/path/to/broker
if(TARGET test22)
    set(PAHO_MQTT_TESTING_DIR
        "$ENV{HOME}/git/paho.mqtt.testing/interoperability"
        CACHE PATH "paho.mqtt.testing interoperability broker directory")

    set(GW2_PID_FILE "${CMAKE_CURRENT_BINARY_DIR}/gateway2.pid")
    set(GW2_BROKER_PID_FILE "${CMAKE_CURRENT_BINARY_DIR}/broker2.pid")
    set(GW2_CONF "${CMAKE_CURRENT_LIST_DIR}/../test-gateway.conf")
    set(GW2_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")

    add_test(NAME gw2_start
        COMMAND bash ${GW2_SCRIPTS}/start-gateway.sh
            $<TARGET_FILE:MQTT-SN2Gateway>
            ${GW2_CONF}
            ${GW2_PID_FILE}
            ${PAHO_MQTT_TESTING_DIR}
            ${GW2_BROKER_PID_FILE})
    set_tests_properties(gw2_start PROPERTIES
        FIXTURES_SETUP MQTTSN2Gateway_fixture
        TIMEOUT 15)

    add_test(NAME gw2_stop
        COMMAND bash ${GW2_SCRIPTS}/stop-gateway.sh
            ${GW2_PID_FILE}
            ${GW2_BROKER_PID_FILE})
    set_tests_properties(gw2_stop PROPERTIES
        FIXTURES_CLEANUP MQTTSN2Gateway_fixture)

    # test22: basic MQTT-SN 2.0 connectivity (CONNECT, PUBLISH, SUBSCRIBE, PING)
    add_test(NAME gw2_test22
        COMMAND $<TARGET_FILE:test22>)
    set_tests_properties(gw2_test22 PROPERTIES
        FIXTURES_REQUIRED MQTTSN2Gateway_fixture
        TIMEOUT 120)

    # test23: PROTECTION-encapsulated packets — requires gateway-side security support
    add_test(NAME gw2_test23
        COMMAND $<TARGET_FILE:test23>)
    set_tests_properties(gw2_test23 PROPERTIES
        FIXTURES_REQUIRED MQTTSN2Gateway_fixture
        TIMEOUT 120)
endif()
