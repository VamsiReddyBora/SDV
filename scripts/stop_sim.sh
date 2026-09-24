#!/bin/bash
# stop_sim.sh - Terminate all running SDV simulation nodes

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
PID_FILE="$ROOT_DIR/.sim_pids"

echo "=== Stopping SDV Simulation Processes ==="

if [ -f "$PID_FILE" ]; then
    while read -r pid; do
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            echo "Terminating process $pid..."
            kill -TERM "$pid" 2>/dev/null || true
        fi
    done < "$PID_FILE"
    rm -f "$PID_FILE"
fi

# Fallback: kill any remaining node instances by binary name
killall -q central_compute pcm_node bms_node bcm_node hmi_node adas_node brake_node eps_node hvac_node telematics_node || true

echo "All simulation nodes stopped."
