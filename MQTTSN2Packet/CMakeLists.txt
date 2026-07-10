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

cmake_minimum_required(VERSION 3.12)

PROJECT("paho-mqttsn2packet" C)

find_package(OpenSSL REQUIRED)
set(SSL_LIBRARY_NAME OpenSSL)
set(SSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
set(SSL_ROOT_DIR ${OPENSSL_ROOT_DIR})
message(STATUS "Using ${SSL_LIBRARY_NAME} with headers at ${SSL_INCLUDE_DIR}")

link_libraries(${SSL_LIBRARY_NAME}::SSL ${SSL_LIBRARY_NAME}::Crypto)

include_directories(${SSL_INCLUDE_DIR})

enable_testing()

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(samples)
ADD_SUBDIRECTORY(test)
