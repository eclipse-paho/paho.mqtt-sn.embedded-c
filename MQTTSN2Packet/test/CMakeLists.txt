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
        test2
        test2.c ../samples/transport.c
)

TARGET_LINK_LIBRARIES(test2 MQTTSN2Packet)

ADD_TEST(NAME test2 COMMAND test2)

ADD_EXECUTABLE(
        test3
        test3.c ../samples/transport.c
)

TARGET_LINK_LIBRARIES(test3 MQTTSN2Packet /opt/homebrew/opt/openssl@3/lib/libcrypto.dylib)
TARGET_INCLUDE_DIRECTORIES(test3 PUBLIC /opt/homebrew/opt/openssl@3/include)

ADD_TEST(NAME test3 COMMAND test3)
