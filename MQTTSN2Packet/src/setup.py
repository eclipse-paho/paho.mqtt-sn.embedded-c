"""
/*******************************************************************************
 * Copyright (c) 2026 Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * AI Disclosure: This file was partly AI-generated. The AI-generated
 * portions are made available under CC0-1.0 and not subject to the
 * project's licence. The human contributor has reviewed and verified
 * that the code is correct.
 *
 * SPDX-License-Identifier: EPL-2.0 and CC0-1.0
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

setup.py — build the mqttsn2 Python extension module.

Usage:
    python setup.py build_ext --inplace

This compiles mqttsn2module.c together with the MQTT-SN 2.0 library
source files into a shared extension that can be imported directly:

    import mqttsn2
    pkt = mqttsn2.serialize_connect("my-client", keep_alive=60)

Directory layout expected:
    setup.py                        (this file)
    mqttsn2module.c                 (the Python extension wrapper)
    MQTTSNPacket2.c                 (v2.0 packet helper implementation)
    MQTTSNPacket.h                  (v2.0 packet header)
    MQTTSNConnect.h                 (v2.0 connect types)
    MQTTSNSubscribe.h               (v2.0 subscribe types)
    MQTTSNPublish.h                 (v2.0 publish types)
    StackTrace.h                    (no-op stub: defines NOSTACKTRACE)
    MQTTSNConnectClient.c
    MQTTSNSubscribeClient.c
    MQTTSNSerializePublish.c
    MQTTSNDeserializePublish.c

All source files are compiled as a single extension so that the
MQTT-SN library object files do not need to be built and installed
separately.
"""

from setuptools import setup, Extension
import os

here = os.path.dirname(os.path.abspath(__file__))

mqttsn2 = Extension(
    name="mqttsn2",
    sources=[
        "mqttsn2module.c",
        "MQTTSNPacket2.c",
        "MQTTSNConnectClient.c",
        "MQTTSNSubscribeClient.c",
        "MQTTSNSerializePublish.c",
        "MQTTSNDeserializePublish.c",
    ],
    include_dirs=[here],
    define_macros=[
        ("NOSTACKTRACE", "1"),   # compile out all FUNC_ENTRY / FUNC_EXIT_RC macros
        ("NDEBUG",       "1"),   # disable assert() so invalid packets return 0
                                 # instead of aborting the Python interpreter
    ],
    extra_compile_args=["-std=c11", "-Wall", "-Wextra",
                        "-Wno-unused-parameter"],
)

setup(
    name="mqttsn2",
    version="2.0.0",
    description="Python interface to MQTT-SN 2.0 packet serialization",
    long_description=__doc__,
    author="Ian Craggs",
    license="EPL-2.0",
    ext_modules=[mqttsn2],
    python_requires=">=3.8",
)
