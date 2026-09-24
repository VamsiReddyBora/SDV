# Software-Defined Vehicle (SDV) Linux Simulation

A modular C simulation of a centralized Software-Defined Vehicle (SDV) architecture running on Linux SocketCAN (`vcan`).

---

## 📚 Educational Study Guides (PDFs)

In-depth technical guides have been authored and compiled under [**`Information/`**](Information/):

| Guide | File Link | Focus Area |
|---|---|---|
| **Part 1: Fundamentals** | [**`01_CAN_Bus_Fundamentals.pdf`**](Information/01_CAN_Bus_Fundamentals.pdf) | Physical layer differential signaling, non-destructive bitwise arbitration, 11-bit frame anatomy, and broadcast addressing. |
| **Part 2: SocketCAN** | [**`02_Linux_SocketCAN_Architecture.pdf`**](Information/02_Linux_SocketCAN_Architecture.pdf) | Linux kernel `vcan` loopback driver, BSD socket C API (`PF_CAN`), and `can-utils` diagnostic workflows. |
| **Part 3: SDV Architecture** | [**`03_SDV_Centralization_And_Zonal_Architecture.pdf`**](Information/03_SDV_Centralization_And_Zonal_Architecture.pdf) | Transition from 100+ distributed domain ECUs to Central Compute (CVC), dual-bus domain isolation (`vcan0` vs `vcan1`), and cross-bus software routing. |
| **Part 4: Code Walkthrough** | [**`04_Code_Implementation_Deep_Dive.pdf`**](Information/04_Code_Implementation_Deep_Dive.pdf) | Packed structs, fixed-point math, dual-bus `select()` I/O multiplexing in `central_compute`, powertrain physics, and battery thermodynamics. |

---

## 1. Architecture Overview

In a traditional vehicle, independent ECUs are hardwired directly to specific actuators and communicate over isolated domain buses. In a **Software-Defined Vehicle (SDV)**:
- **Central Vehicle Computer (CVC / Central Compute)** runs consolidated supervisory control, dynamic power arbitration, and cross-bus software routing.
- **Multiple Virtual CAN Buses** isolate network domains:
  - `vcan0`: Powertrain & High-Voltage Traction Battery
  - `vcan1`: Body, Cabin & Cockpit Driver Interface (HMI)
- **Processes as Nodes**: Each ECU or compute unit runs as an independent Linux process interacting via standard raw CAN sockets (`PF_CAN`, `SOCK_RAW`).

```
                    +------------------------------------+
                    |       Central Compute (CVC)        |
                    |      [central_compute process]     |
                    +-----------------+------------------+
                                      |
              +-----------------------+-----------------------+
              |                                               |
     [vcan0: Powertrain Bus]                         [vcan1: Body & Cabin Bus]
              |                                               |
       +------+-------+                                +------+-------+
       |              |                                |              |
+-------------+ +-------------+                 +-------------+ +-------------+
|     PCM     | |     BMS     |                 |     BCM     | |  HMI / Cockpit|
| [pcm_node]  | | [bms_node]  |                 | [bcm_node]  | | [hmi_node]  |
+-------------+ +-------------+                 +-------------+ +-------------+
```

---

## 2. The 5 Initial Nodes

| Node Binary | Name | Bus | CAN ID(s) | Description |
|---|---|---|---|---|
| `central_compute` | **Central Vehicle Computer** | `vcan0` & `vcan1` | `0x101`, `0x201` (TX)<br>`0x100`, `0x110`, `0x200`, `0x210` (RX) | Dual-homed central orchestrator. Arbitrates driver requests, manages vehicle speed governing, regenerative braking, thermal derating, auto door locks, and displays live status dashboard. |
| `pcm_node` | **Powertrain Control Module** | `vcan0` | `0x100` (TX)<br>`0x101` (RX) | Simulates electric motor, inverter, transmission, inertia, aerodynamic drag, rolling friction, speed (km/h), RPM, and torque. |
| `bms_node` | **Battery Management System** | `vcan0` | `0x110` (TX)<br>`0x100` (RX) | Simulates 400V high-voltage battery pack, State of Charge (SoC %), internal resistance, current draw / regen charging, and thermal heating. |
| `bcm_node` | **Body Control Module** | `vcan1` | `0x200` (TX)<br>`0x201` (RX) | Controls door locks (FL, FR, RL, RR), low beams, high beams, brake lights, and ambient lighting. |
| `hmi_node` | **Cockpit Driver Interface** | `vcan1` | `0x210` (TX) | Simulates driver accelerator pedal (0-100%), brake pedal (0-100%), steering angle, gear selector (P/R/N/D), and drive mode (ECO/COMFORT/SPORT). |

---

## 3. CAN Protocol Matrix

All messages use packed standard 11-bit CAN frames (DLC <= 8 bytes) defined in [common/vehicle_protocol.h](common/vehicle_protocol.h):

- `0x100` (`PcmTelemetryMsg`): Speed (km/h * 10), Motor RPM, Actual Torque (Nm), Temp (°C), Gear.
- `0x101` (`PcmCmdMsg`): Target Torque (Nm), Speed Limit, Inverter Enable, Regen Level.
- `0x110` (`BmsTelemetryMsg`): SoC (%), Pack Voltage (V * 10), Pack Current (A * 10), Cell Temp (°C), BMS Status, SoH (%).
- `0x200` (`BcmTelemetryMsg`): Doors locked bitmask, Lights bitmask, Cabin temp, Ambient lux.
- `0x201` (`BcmCmdMsg`): Door lock command, Light command bitmask.
- `0x210` (`HmiInputMsg`): Throttle %, Brake %, Steering angle, Selected gear, Drive mode.

---

## 4. How to Build & Run

### 4.1 Prerequisites
Virtual CAN kernel module and `can-utils` (already configured in this environment):
```bash
sudo modprobe vcan
./scripts/setup_vcan.sh
```

### 4.2 Compile
```bash
make
```

### 4.3 Run Interactive Central Compute Dashboard
```bash
./scripts/start_sim.sh
```
Press `Ctrl+C` to cleanly terminate all simulation processes.

### 4.4 Run in Background Daemon Mode
```bash
./scripts/start_sim.sh --daemon
```

To stop:
```bash
./scripts/stop_sim.sh
```

---

## 5. Monitoring CAN Traffic Live
Inspect raw CAN bus frames in real time:
```bash
# Monitor Powertrain & Battery bus
candump -tz vcan0

# Monitor Body & Cockpit bus
candump -tz vcan1
```

---

## 6. How to Add Remaining Nodes (Extensibility)
To add a new node (e.g. `adas_node`, `brake_node`, `thermal_node`, or `telematics_node`):
1. Define the message payload struct and CAN ID in [common/vehicle_protocol.h](common/vehicle_protocol.h).
2. Create `nodes/<new_node>.c` utilizing [common/can_common.h](common/can_common.h).
3. Add the node to `NODES` in [Makefile](Makefile).
4. Add the startup command in [scripts/start_sim.sh](scripts/start_sim.sh).
5. Update or regenerate the PDFs in [Information/](Information/) via `python3 Information/generate_pdfs.py`.
