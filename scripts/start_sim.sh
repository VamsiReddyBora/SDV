#!/bin/bash
# start_sim.sh - Launch the 5 SDV simulation nodes

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
LOG_DIR="$ROOT_DIR/logs"
PID_FILE="$ROOT_DIR/.sim_pids"

mkdir -p "$LOG_DIR"
rm -f "$PID_FILE"

echo "=== Initializing SDV Virtual CAN Environment ==="
"$SCRIPT_DIR/setup_vcan.sh"

echo ""
echo "=== Starting SDV ECU Nodes ==="

# 1. Start Powertrain Control Module (vcan0)
"$ROOT_DIR/bin/pcm_node" vcan0 > "$LOG_DIR/pcm.log" 2>&1 &
PCM_PID=$!
echo $PCM_PID >> "$PID_FILE"
echo "[+] PCM Node (Powertrain) started [PID: $PCM_PID] on vcan0"

# 2. Start Battery Management System (vcan0)
"$ROOT_DIR/bin/bms_node" vcan0 > "$LOG_DIR/bms.log" 2>&1 &
BMS_PID=$!
echo $BMS_PID >> "$PID_FILE"
echo "[+] BMS Node (Battery) started [PID: $BMS_PID] on vcan0"

# 3. Start Body Control Module (vcan1)
"$ROOT_DIR/bin/bcm_node" vcan1 > "$LOG_DIR/bcm.log" 2>&1 &
BCM_PID=$!
echo $BCM_PID >> "$PID_FILE"
echo "[+] BCM Node (Body) started [PID: $BCM_PID] on vcan1"

# 4. Start Driver / HMI Input Node (vcan1)
"$ROOT_DIR/bin/hmi_node" vcan1 > "$LOG_DIR/hmi.log" 2>&1 &
HMI_PID=$!
echo $HMI_PID >> "$PID_FILE"
echo "[+] HMI Node (Cockpit) started [PID: $HMI_PID] on vcan1"

sleep 0.5

# Check if foreground or background for Central Compute
if [ "$1" == "--daemon" ]; then
    "$ROOT_DIR/bin/central_compute" vcan0 vcan1 > "$LOG_DIR/central_compute.log" 2>&1 &
    CC_PID=$!
    echo $CC_PID >> "$PID_FILE"
    echo "[+] Central Vehicle Computer (CVC) started [PID: $CC_PID] in daemon mode."
    echo "Logs are available in $LOG_DIR/"
else
    echo ""
    echo "=== Starting Central Vehicle Computer Dashboard ==="
    echo "(To stop simulation, press Ctrl+C)"
    # Trap SIGINT to stop all child processes
    trap 'echo ""; echo "Stopping all simulation nodes..."; "$SCRIPT_DIR/stop_sim.sh"; exit 0' INT TERM
    "$ROOT_DIR/bin/central_compute" vcan0 vcan1
    "$SCRIPT_DIR/stop_sim.sh"
fi
