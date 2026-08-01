#!/bin/bash
# Stop MQTT-SN 2.0 gateway and MQTT broker started by start-gateway.sh.
# Usage: stop-gateway.sh <gw-pid-file> <broker-pid-file>

GW_PID_FILE="$1"
BROKER_PID_FILE="$2"

if [ -f "$GW_PID_FILE" ]; then
    GW_PID=$(cat "$GW_PID_FILE")
    kill -9 "$GW_PID" 2>/dev/null || true
    rm -f "$GW_PID_FILE"
    echo "Gateway stopped (PID=$GW_PID)"
fi

if [ -f "$BROKER_PID_FILE" ]; then
    BROKER_PID=$(cat "$BROKER_PID_FILE")
    kill -9 "$BROKER_PID" 2>/dev/null || true
    rm -f "$BROKER_PID_FILE"
    echo "Broker stopped (PID=$BROKER_PID)"
fi

exit 0
