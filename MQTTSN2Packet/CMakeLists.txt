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

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(samples)
ADD_SUBDIRECTORY(test)

enable_testing()
