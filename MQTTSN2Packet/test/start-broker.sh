#!/bin/bash
# Start the MQTT-SN 2.0 UDP broker (paho.mqtt.testing) for the ctest fixture
# wrapping test22/test23 - these exercise MQTTSN2Packet directly against a
# broker with native MQTT-SN support, with no gateway involved.
# Usage: start-broker.sh <broker-dir> <broker-conf> <broker-pid-file> <udp-port>

set -e
BROKER_DIR="$1"
BROKER_CONF="$2"
BROKER_PID_FILE="$3"
UDP_PORT="$4"

# Auto-clone the broker from GitHub and provision its venv if not already present.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"$SCRIPT_DIR/../../scripts/ensure-paho-mqtt-testing.sh" "$BROKER_DIR"

# Use the venv provisioned above if present; fall back to plain python3.
cd "$BROKER_DIR"
if [ -x ".venv/bin/python3" ]; then
    PYTHON=".venv/bin/python3"
else
    PYTHON="python3"
fi

# Free the broker's UDP port, in case a stray process is still on it
STALE_PIDS=$(lsof -tiUDP:"$UDP_PORT" 2>/dev/null || true)
if [ -n "$STALE_PIDS" ]; then
    echo "Stopping stale process(es) on UDP port $UDP_PORT: $STALE_PIDS"
    kill -9 $STALE_PIDS 2>/dev/null || true
    sleep 1
fi

$PYTHON startbroker.py -c "$BROKER_CONF" > /tmp/mqttsn2-packet-test-broker.log 2>&1 &
BROKER_PID=$!
echo "$BROKER_PID" > "$BROKER_PID_FILE"
sleep 1

if ! kill -0 "$BROKER_PID" 2>/dev/null; then
    echo "ERROR: MQTT-SN broker failed to start"
    cat /tmp/mqttsn2-packet-test-broker.log
    exit 1
fi
echo "MQTT-SN broker started (PID=$BROKER_PID, conf=$BROKER_CONF)"
