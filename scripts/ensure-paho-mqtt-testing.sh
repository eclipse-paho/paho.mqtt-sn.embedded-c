#!/bin/bash
# Ensure a local checkout of the paho.mqtt.testing reference broker (used by
# the MQTT-SN 2.0 ctest integration fixtures, see MQTTSN2Packet/test/start-broker.sh
# and MQTTSN2Gateway/src/tests/start-gateway.sh) is available at <broker-dir>,
# cloning it from GitHub if it isn't already present, and that its Python venv
# has the packages the broker needs (cryptography, for MQTT-SN PROTECTION
# support exercised by test3/test23).
#
# Usage: ensure-paho-mqtt-testing.sh <broker-dir>
#   <broker-dir> is expected to be a ".../paho.mqtt.testing/interoperability"
#   style path. If it doesn't already contain a checkout, its parent directory
#   is populated by cloning https://github.com/eclipse-paho/paho.mqtt.testing.git.
#   If <broker-dir> already exists (e.g. the caller pointed
#   -DPAHO_MQTT_TESTING_DIR at an existing checkout), it is used as-is and
#   nothing is cloned.

set -e
BROKER_DIR="$1"
REPO_URL="https://github.com/eclipse-paho/paho.mqtt.testing.git"
# The MQTT-SN 2.0 broker support lives on "develop" - the default branch
# ("master") is stale and lacks it entirely (no MQTTSNProtection.py etc).
REPO_BRANCH="develop"

if [ ! -f "$BROKER_DIR/startbroker.py" ]; then
    CLONE_ROOT="$(dirname "$BROKER_DIR")"
    mkdir -p "$(dirname "$CLONE_ROOT")"
    echo "Cloning paho.mqtt.testing (MQTT-SN reference broker, branch $REPO_BRANCH) into $CLONE_ROOT"
    git clone --quiet --depth 1 --branch "$REPO_BRANCH" "$REPO_URL" "$CLONE_ROOT"
fi

VENV_DIR="$BROKER_DIR/.venv"
if [ ! -x "$VENV_DIR/bin/python3" ]; then
    echo "Creating Python venv for the MQTT-SN broker at $VENV_DIR"
    # The broker requires Python >= 3.10 (uses "X | None" type hints evaluated
    # at import time); macOS/older-distro system python3 is often older than
    # that. Prefer uv, which fetches a suitable interpreter on demand; fall
    # back to searching PATH for a new-enough python3.
    if command -v uv >/dev/null 2>&1; then
        uv venv --python 3.12 --quiet "$VENV_DIR"
        uv pip install --quiet --python "$VENV_DIR/bin/python3" cryptography
    else
        PYBIN=""
        for cand in python3.12 python3.11 python3.10 python3; do
            if command -v "$cand" >/dev/null 2>&1; then
                PYBIN="$cand"
                break
            fi
        done
        "$PYBIN" -m venv "$VENV_DIR"
        "$VENV_DIR/bin/pip" install --quiet --upgrade pip
        "$VENV_DIR/bin/pip" install --quiet cryptography
    fi
fi
