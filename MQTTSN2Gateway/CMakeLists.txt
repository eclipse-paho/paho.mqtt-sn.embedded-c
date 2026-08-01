#*******************************************************************************
#  Copyright (c) 2022, 2026 a1lu
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
#     a1lu - initial version
#*******************************************************************************/
cmake_minimum_required(VERSION 3.5)

find_package(OpenSSL REQUIRED)
link_libraries(OpenSSL::SSL OpenSSL::Crypto)
include_directories(${OPENSSL_INCLUDE_DIR})

if(NOT TARGET MQTTSN2Packet)
    ADD_SUBDIRECTORY(../MQTTSN2Packet ${CMAKE_CURRENT_BINARY_DIR}/MQTTSN2Packet)
endif()

ADD_SUBDIRECTORY(src)