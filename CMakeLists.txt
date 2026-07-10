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

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT("paho-mqttsn" CXX)
MESSAGE(STATUS "CMake version: " ${CMAKE_VERSION})
MESSAGE(STATUS "CMake system name: " ${CMAKE_SYSTEM_NAME})

SET(CMAKE_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake")
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

## build settings
SET(PAHO_VERSION_MAJOR 1)
SET(PAHO_VERSION_MINOR 0)
SET(PAHO_VERSION_PATCH 0)
SET(CLIENT_VERSION ${PAHO_VERSION_MAJOR}.${PAHO_VERSION_MINOR}.${PAHO_VERSION_PATCH})

STRING(TIMESTAMP BUILD_TIMESTAMP UTC)
MESSAGE(STATUS "Timestamp is ${BUILD_TIMESTAMP}")

SET(CPACK_PACKAGE_VERSION_MAJOR ${PAHO_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PAHO_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PAHO_VERSION_PATCH})
INCLUDE(CPack)

find_package(OpenSSL REQUIRED)
set(SSL_LIBRARY_NAME OpenSSL)
set(SSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
set(SSL_ROOT_DIR ${OPENSSL_ROOT_DIR})
message(STATUS "Using ${SSL_LIBRARY_NAME} with headers at ${SSL_INCLUDE_DIR}")

link_libraries(${SSL_LIBRARY_NAME}::SSL ${SSL_LIBRARY_NAME}::Crypto)

include_directories(${SSL_INCLUDE_DIR})

ENABLE_TESTING()

ADD_SUBDIRECTORY(MQTTSNPacket)
ADD_SUBDIRECTORY(MQTTSN2Packet)
ADD_SUBDIRECTORY(MQTTSNGateway)
