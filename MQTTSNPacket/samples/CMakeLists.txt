#*******************************************************************************
#  Copyright (c) 2017, 2026 IBM Corp., Ian Craggs
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

PROJECT(mqtt-sn-samples)

INCLUDE_DIRECTORIES(../src)

ADD_EXECUTABLE(
  qos0pub
  qos0pub.c transport.c
)

TARGET_LINK_LIBRARIES(qos0pub MQTTSNPacket)

ADD_EXECUTABLE(
  qos0pub_register
  qos0pub_register.c transport.c
)

TARGET_LINK_LIBRARIES(qos0pub_register MQTTSNPacket)

ADD_EXECUTABLE(
  qos-1pub
  qos-1pub.c transport.c
)

TARGET_LINK_LIBRARIES(qos-1pub MQTTSNPacket)

ADD_EXECUTABLE(
  qos-1pub_extended
  qos-1pub_extended.c transport.c
)

TARGET_LINK_LIBRARIES(qos-1pub_extended MQTTSNPacket)

ADD_EXECUTABLE(
  qos1pub
  qos1pub.c transport.c
)

TARGET_LINK_LIBRARIES(qos1pub MQTTSNPacket)

ADD_EXECUTABLE(
  pub0sub1
  pub0sub1.c transport.c
)

TARGET_LINK_LIBRARIES(pub0sub1 MQTTSNPacket)