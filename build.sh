#!/bin/bash
# Script to build, flash, and monitor an ESP-IDF project
# Usage:
#   ./esp.sh build
#   ./esp.sh build flash -p /dev/ttyUSB0
#   ./esp.sh flash monitor -p /dev/ttyUSB0


. $HOME/esp/esp-idf/export.sh

PORT=""
ACTIONS=()

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--port)
            PORT=$2
            shift 2
            ;;
        build|flash|monitor)
            ACTIONS+=("$1")
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [build] [flash] [monitor] [-p <port>]"
            exit 1
            ;;
    esac
done

# Require at least one action
if [[ ${#ACTIONS[@]} -eq 0 ]]; then
    echo "No actions specified! Choose from: build, flash, monitor"
    exit 1
fi

# Check if port is required
if [[ " ${ACTIONS[*]} " =~ " flash " || " ${ACTIONS[*]} " =~ " monitor " ]]; then
    if [[ -z "$PORT" ]]; then
        echo "Error: Flash/monitor requires a port with -p \[PORT\] here is a list of your current ports:"
        ls /dev/ttyUSB*
        exit 1
    fi
fi

# Run actions
for action in "${ACTIONS[@]}"; do
    case $action in
        build)
            echo "🔨 Building project..."
            idf.py build
            ;;
        flash)
            echo "⚡ Flashing firmware to $PORT..."
            idf.py -p "$PORT" flash
            ;;
        monitor)
            echo "📟 Starting monitor on $PORT (Ctrl+] to exit)..."
            idf.py -p "$PORT" monitor
            ;;
    esac
done
