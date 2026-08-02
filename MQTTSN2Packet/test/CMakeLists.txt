#*******************************************************************************
#  Copyright (c) 2026 Ian Craggs
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
#     Ian Craggs - initial version
#*******************************************************************************/

PROJECT(mqtt-sn2-tests)

INCLUDE_DIRECTORIES(../src ../samples)

ADD_EXECUTABLE(
        test_connect
        test_connect.c
)

TARGET_LINK_LIBRARIES(test_connect MQTTSN2Packet)

ADD_TEST(NAME test_connect COMMAND test_connect)

ADD_EXECUTABLE(
        test_publish
        test_publish.c
)

TARGET_LINK_LIBRARIES(test_publish MQTTSN2Packet)

ADD_TEST(NAME test_publish COMMAND test_publish)

ADD_EXECUTABLE(
        test_subscribe
        test_subscribe.c
)

TARGET_LINK_LIBRARIES(test_subscribe MQTTSN2Packet)

ADD_TEST(NAME test_subscribe COMMAND test_subscribe)

ADD_EXECUTABLE(
        test_unsubscribe
        test_unsubscribe.c
)

TARGET_LINK_LIBRARIES(test_unsubscribe MQTTSN2Packet)

ADD_TEST(NAME test_unsubscribe COMMAND test_unsubscribe)

ADD_EXECUTABLE(
        test_protection
        test_protection.c
)

TARGET_LINK_LIBRARIES(test_protection MQTTSN2Packet)

ADD_TEST(NAME test_protection COMMAND test_protection)

ADD_EXECUTABLE(
        test_auth
        test_auth.c
)

TARGET_LINK_LIBRARIES(test_auth MQTTSN2Packet)

ADD_TEST(NAME test_auth COMMAND test_auth)

ADD_EXECUTABLE(
        test_pubwos
        test_pubwos.c
)

TARGET_LINK_LIBRARIES(test_pubwos MQTTSN2Packet)

ADD_TEST(NAME test_pubwos COMMAND test_pubwos)

ADD_EXECUTABLE(
        test_sleep
        test_sleep.c
)

TARGET_LINK_LIBRARIES(test_sleep MQTTSN2Packet)

ADD_TEST(NAME test_sleep COMMAND test_sleep)

ADD_EXECUTABLE(
        test_discovery
        test_discovery.c
)

TARGET_LINK_LIBRARIES(test_discovery MQTTSN2Packet)

ADD_TEST(NAME test_discovery COMMAND test_discovery)

ADD_EXECUTABLE(
        test_forwarder
        test_forwarder.c
)

TARGET_LINK_LIBRARIES(test_forwarder MQTTSN2Packet)

ADD_TEST(NAME test_forwarder COMMAND test_forwarder)

ADD_EXECUTABLE(
        test_connection_encapsulation
        test_connection_encapsulation.c
)

TARGET_LINK_LIBRARIES(test_connection_encapsulation MQTTSN2Packet)

ADD_TEST(NAME test_connection_encapsulation COMMAND test_connection_encapsulation)

ADD_EXECUTABLE(
        test22
        test2.c ../samples/transport.c
)

TARGET_LINK_LIBRARIES(test22 MQTTSN2Packet)

ADD_TEST(NAME test22 COMMAND test22)

ADD_EXECUTABLE(
        test23
        test3.c ../samples/transport.c
)

TARGET_LINK_LIBRARIES(test23 MQTTSN2Packet)

ADD_TEST(NAME test23 COMMAND test23)

# test22/test23 exercise MQTTSN2Packet directly against a live MQTT-SN 2.0 UDP
# broker (paho.mqtt.testing, run with its udp.conf - native MQTT-SN support,
# no gateway involved). The broker is auto-cloned from GitHub (if not already
# present) and started/stopped automatically via a ctest fixture, so the
# build is self-sufficient on a clean machine with no pre-installed broker.
# Point at an existing checkout instead (skips the auto-clone) by overriding
# on the cmake command line:
#   -DPAHO_MQTT_TESTING_DIR=/path/to/interoperability
set(PAHO_MQTT_TESTING_DIR
    "${CMAKE_BINARY_DIR}/paho-mqtt-testing/interoperability"
    CACHE PATH "paho.mqtt.testing interoperability broker directory (auto-cloned from GitHub here if missing)")

set(SN_BROKER_PID_FILE "${CMAKE_CURRENT_BINARY_DIR}/sn_broker.pid")

add_test(NAME sn_broker_start
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/start-broker.sh
        ${PAHO_MQTT_TESTING_DIR}
        udp.conf
        ${SN_BROKER_PID_FILE}
        1883)
set_tests_properties(sn_broker_start PROPERTIES
    FIXTURES_SETUP MQTTSN2Packet_broker_fixture
    TIMEOUT 15)

add_test(NAME sn_broker_stop
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/stop-broker.sh
        ${SN_BROKER_PID_FILE})
set_tests_properties(sn_broker_stop PROPERTIES
    FIXTURES_CLEANUP MQTTSN2Packet_broker_fixture)

set_tests_properties(test22 test23 PROPERTIES
    FIXTURES_REQUIRED MQTTSN2Packet_broker_fixture
    TIMEOUT 120)
