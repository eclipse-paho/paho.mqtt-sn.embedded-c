#!/bin/bash
# Stop the MQTT-SN 2.0 UDP broker started by start-broker.sh.
# Usage: stop-broker.sh <broker-pid-file>

BROKER_PID_FILE="$1"

if [ -f "$BROKER_PID_FILE" ]; then
    BROKER_PID=$(cat "$BROKER_PID_FILE")
    kill -9 "$BROKER_PID" 2>/dev/null || true
    rm -f "$BROKER_PID_FILE"
    echo "MQTT-SN broker stopped (PID=$BROKER_PID)"
fi

exit 0
