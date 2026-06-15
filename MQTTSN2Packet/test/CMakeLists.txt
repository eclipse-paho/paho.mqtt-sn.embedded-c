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

INCLUDE_DIRECTORIES(../src)

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