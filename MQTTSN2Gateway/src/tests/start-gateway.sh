#!/bin/bash
# Start MQTT broker then MQTT-SN 2.0 gateway for the ctest integration fixture.
# Usage: start-gateway.sh <gw-binary> <gw-conf> <gw-pid-file> <broker-dir> <broker-pid-file>

set -e
GW_BIN="$1"
GW_CONF="$2"
GW_PID_FILE="$3"
BROKER_DIR="$4"
BROKER_PID_FILE="$5"

# --- Start MQTT broker ---
# Use the uv-managed Python 3.12 venv if present; fall back to plain python3.
cd "$BROKER_DIR"
if [ -x ".venv/bin/python3" ]; then
    PYTHON=".venv/bin/python3"
else
    PYTHON="python3"
fi
$PYTHON startbroker.py -c localhost_testing.conf > /tmp/mqttsn2-test-broker.log 2>&1 &
BROKER_PID=$!
echo "$BROKER_PID" > "$BROKER_PID_FILE"
sleep 1

if ! kill -0 "$BROKER_PID" 2>/dev/null; then
    echo "ERROR: MQTT broker failed to start"
    cat /tmp/mqttsn2-test-broker.log
    exit 1
fi
echo "Broker started (PID=$BROKER_PID)"

# --- Start MQTT-SN 2.0 gateway ---
"$GW_BIN" -f "$GW_CONF" > /tmp/mqttsn2-test-gateway.log 2>&1 &
GW_PID=$!
echo "$GW_PID" > "$GW_PID_FILE"
sleep 2

if kill -0 "$GW_PID" 2>/dev/null; then
    echo "Gateway started (PID=$GW_PID, conf=$GW_CONF)"
    exit 0
else
    echo "ERROR: MQTT-SN 2.0 gateway failed to start"
    kill "$BROKER_PID" 2>/dev/null || true
    rm -f "$BROKER_PID_FILE"
    exit 1
fi
