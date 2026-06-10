#*******************************************************************************
#  Copyright (c) 2017, 2026 IBM Corp., Ian Craggs
#
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v1.0
#  and Eclipse Distribution License v1.0 which accompany this distribution.
#
#  The Eclipse Public License is available at
#     http://www.eclipse.org/legal/epl-v10.html
#  and the Eclipse Distribution License is available at
#    http://www.eclipse.org/org/documents/edl-v10.php.
#
#  Contributors:
#     Ian Craggs - initial version
#*******************************************************************************/

PROJECT(mqtt-sn-samples)

INCLUDE_DIRECTORIES(../src)

ADD_EXECUTABLE(
  qos0pub2
  qos0pub.c transport.c
)

TARGET_LINK_LIBRARIES(qos0pub2 MQTTSN2Packet)

ADD_EXECUTABLE(
  paho_sn_sub
  paho_sn_sub.c transport.c
)

TARGET_LINK_LIBRARIES(paho_sn_sub MQTTSN2Packet)

ADD_EXECUTABLE(
  paho_sn_pub
  paho_sn_pub.c transport.c
)

TARGET_LINK_LIBRARIES(paho_sn_pub MQTTSN2Packet)

