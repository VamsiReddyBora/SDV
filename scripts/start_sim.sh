#!/bin/bash
# start_sim.sh - Launch the 10 SDV simulation nodes

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
LOG_DIR="$ROOT_DIR/logs"
PID_FILE="$ROOT_DIR/.sim_pids"

mkdir -p "$LOG_DIR"
rm -f "$PID_FILE"

echo "=== Initializing SDV Virtual CAN Environment ==="
"$SCRIPT_DIR/setup_vcan.sh"

echo ""
echo "=== Starting SDV Distributed ECU Nodes (10-Node Topology) ==="

# --- vcan0: Powertrain, Chassis & Active Safety Domain ---
# 1. Powertrain Control Module (vcan0)
"$ROOT_DIR/bin/pcm_node" vcan0 > "$LOG_DIR/pcm.log" 2>&1 &
PCM_PID=$!
echo $PCM_PID >> "$PID_FILE"
echo "[+] Node 1: PCM (Powertrain) started [PID: $PCM_PID] on vcan0"

# 2. Battery Management System (vcan0)
"$ROOT_DIR/bin/bms_node" vcan0 > "$LOG_DIR/bms.log" 2>&1 &
BMS_PID=$!
echo $BMS_PID >> "$PID_FILE"
echo "[+] Node 2: BMS (High-Voltage Battery) started [PID: $BMS_PID] on vcan0"

# 3. ABS / Electronic Braking Node (vcan0)
"$ROOT_DIR/bin/brake_node" vcan0 > "$LOG_DIR/brake.log" 2>&1 &
BRAKE_PID=$!
echo $BRAKE_PID >> "$PID_FILE"
echo "[+] Node 3: ABS/Brake (Brake System) started [PID: $BRAKE_PID] on vcan0"

# 4. Electric Power Steering (vcan0)
"$ROOT_DIR/bin/eps_node" vcan0 > "$LOG_DIR/eps.log" 2>&1 &
EPS_PID=$!
echo $EPS_PID >> "$PID_FILE"
echo "[+] Node 4: EPS (Electric Power Steering) started [PID: $EPS_PID] on vcan0"

# 5. ADAS Radar/Vision (vcan0)
"$ROOT_DIR/bin/adas_node" vcan0 > "$LOG_DIR/adas.log" 2>&1 &
ADAS_PID=$!
echo $ADAS_PID >> "$PID_FILE"
echo "[+] Node 5: ADAS (Radar/Vision Perception) started [PID: $ADAS_PID] on vcan0"

# --- vcan1: Body, Cabin, Thermal & Telematics Domain ---
# 6. Body Control Module (vcan1)
"$ROOT_DIR/bin/bcm_node" vcan1 > "$LOG_DIR/bcm.log" 2>&1 &
BCM_PID=$!
echo $BCM_PID >> "$PID_FILE"
echo "[+] Node 6: BCM (Body Control) started [PID: $BCM_PID] on vcan1"

# 7. Driver / HMI Input Node (vcan1)
"$ROOT_DIR/bin/hmi_node" vcan1 > "$LOG_DIR/hmi.log" 2>&1 &
HMI_PID=$!
echo $HMI_PID >> "$PID_FILE"
echo "[+] Node 7: HMI (Driver Cockpit) started [PID: $HMI_PID] on vcan1"

# 8. HVAC & Thermal Management (vcan1)
"$ROOT_DIR/bin/hvac_node" vcan1 > "$LOG_DIR/hvac.log" 2>&1 &
HVAC_PID=$!
echo $HVAC_PID >> "$PID_FILE"
echo "[+] Node 8: HVAC (Climate & Thermal Loop) started [PID: $HVAC_PID] on vcan1"

# 9. Telematics & Cloud OTA Gateway (vcan1)
"$ROOT_DIR/bin/telematics_node" vcan1 > "$LOG_DIR/telematics.log" 2>&1 &
TCU_PID=$!
echo $TCU_PID >> "$PID_FILE"
echo "[+] Node 9: Telematics (Cellular & OTA Gateway) started [PID: $TCU_PID] on vcan1"

sleep 0.5

# 10. Central Vehicle Computer (CVC)
if [ "$1" == "--daemon" ]; then
    "$ROOT_DIR/bin/central_compute" vcan0 vcan1 > "$LOG_DIR/central_compute.log" 2>&1 &
    CC_PID=$!
    echo $CC_PID >> "$PID_FILE"
    echo "[+] Node 10: Central Vehicle Computer (CVC) started [PID: $CC_PID] in daemon mode."
    echo "Logs are available in $LOG_DIR/"
else
    echo ""
    echo "=== Starting Central Vehicle Computer Dashboard (Node 10) ==="
    echo "(To stop simulation, press Ctrl+C)"
    trap 'echo ""; echo "Stopping all simulation nodes..."; "$SCRIPT_DIR/stop_sim.sh"; exit 0' INT TERM
    "$ROOT_DIR/bin/central_compute" vcan0 vcan1
    "$SCRIPT_DIR/stop_sim.sh"
fi
