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

import mqttsn2, socket

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

connect = mqttsn2.serialize_connect("clientid", max_packet_size=50, keep_alive=4, packet_id=35)
sock.sendto(connect, ("127.0.0.1", 1883))
print("Connect sent", connect)

data, addr = sock.recvfrom(1024)
print("Connack returned", mqttsn2.deserialize_connack(data))


