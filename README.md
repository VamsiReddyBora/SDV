# Software-Defined Vehicle (SDV) Linux Simulation

A modular, production-grade C simulation of a centralized Software-Defined Vehicle (SDV) architecture running on native Linux SocketCAN (`PF_CAN`, `SOCK_RAW`) across dual virtual CAN domains (`vcan0` and `vcan1`).

---

## 📚 Educational Study Guides (PDFs)

In-depth technical guides have been authored and compiled under [**`Information/`**](Information/):

| Guide | File Link | Focus Area |
|---|---|---|
| **Part 1: Fundamentals** | [**`01_CAN_Bus_Fundamentals.pdf`**](Information/01_CAN_Bus_Fundamentals.pdf) | Physical layer differential signaling, non-destructive bitwise arbitration, 11-bit frame anatomy, and broadcast addressing. |
| **Part 2: SocketCAN** | [**`02_Linux_SocketCAN_Architecture.pdf`**](Information/02_Linux_SocketCAN_Architecture.pdf) | Linux kernel `vcan` loopback driver, BSD socket C API (`PF_CAN`), and `can-utils` diagnostic workflows. |
| **Part 3: SDV Architecture** | [**`03_SDV_Centralization_And_Zonal_Architecture.pdf`**](Information/03_SDV_Centralization_And_Zonal_Architecture.pdf) | Transition from 100+ distributed domain ECUs to Central Compute (CVC), dual-bus domain isolation (`vcan0` vs `vcan1`), full 10-node topology, cross-bus software routing, and active safety arbitration (AEB/LKA/Thermal). |
| **Part 4: Code Walkthrough** | [**`04_Code_Implementation_Deep_Dive.pdf`**](Information/04_Code_Implementation_Deep_Dive.pdf) | Packed structs, fixed-point math, dual-bus `select()` I/O multiplexing in `central_compute`, powertrain physics, ABS slip modulation, EPS assist curves, battery coolant thermodynamics, and OTA lifecycle. |

---

## 1. Architecture Overview

In a traditional vehicle, independent ECUs are hardwired directly to specific actuators and communicate over isolated domain buses with static vendor firmware. In a **Software-Defined Vehicle (SDV)**:
- **Central Vehicle Computer (CVC / Central Compute)** runs consolidated supervisory control, dynamic power arbitration, active safety arbitration, and cross-bus software routing.
- **Multiple Virtual CAN Buses** isolate network domains:
  - `vcan0`: Powertrain, Chassis, High-Voltage Battery & Active Safety
  - `vcan1`: Body, Cabin, Cockpit HMI, Climate Thermal Loop & Telematics OTA Gateway
- **Processes as Nodes**: Each ECU or compute unit runs as an independent Linux process interacting via standard raw CAN sockets (`PF_CAN`, `SOCK_RAW`).

```
                    +-------------------------------------------------------+
                    |                 Central Compute (CVC)                 |
                    |               [central_compute process]               |
                    +---------------------------+---------------------------+
                                                |
               +--------------------------------+--------------------------------+
               |                                                                 |
     [vcan0: Powertrain, Chassis & Safety]                     [vcan1: Body, Cockpit, HVAC & Telematics]
               |                                                                 |
   +-----------+-----------+---------+----------+              +-----------+-----------+----------+
   |           |           |         |          |              |           |           |          |
+-----+     +-----+     +-----+   +-----+    +-----+        +-----+     +-----+     +-----+    +-----+
| PCM |     | BMS |     |BRAKE|   | EPS |    |ADAS |        | BCM |     | HMI |     |HVAC |    | TCU |
+-----+     +-----+     +-----+   +-----+    +-----+        +-----+     +-----+     +-----+    +-----+
```

---

## 2. Complete 10-Node Topology

| Node Binary | Name | Bus | CAN ID(s) | Description |
|---|---|---|---|---|
| `central_compute` | **Central Vehicle Computer (CVC)** | `vcan0` & `vcan1` | `0x101`, `0x121`, `0x131`, `0x201`, `0x221`, `0x401` (TX)<br>`0x100`, `0x110`, `0x120`, `0x130`, `0x200`, `0x210`, `0x220`, `0x300`, `0x400` (RX) | Dual-homed central orchestrator. Arbitrates throttle/brake torque blending, AEB emergency braking, speed-sensitive EPS assist, LKA lane centering, battery thermal chiller loop, speed-governed auto door locks, and displays live ASCII HUD. |
| `pcm_node` | **Powertrain Control Module** | `vcan0` | `0x100` (TX)<br>`0x101` (RX) | Simulates electric motor, inverter, transmission, inertia (1600 kg), aerodynamic drag, rolling friction, speed (km/h), RPM, and motor thermal heating. |
| `bms_node` | **Battery Management System** | `vcan0` | `0x110` (TX)<br>`0x100` (RX) | Simulates 400V high-voltage battery pack (75 kWh), State of Charge (SoC %), internal resistance, current draw / regen charging, and internal Joule heating ($I^2 R$). |
| `brake_node` | **ABS & Electronic Braking** | `vcan0` | `0x120` (TX)<br>`0x121`, `0x100` (RX) | Simulates master cylinder hydraulic pressure (0-140 bar), 4-wheel slip ratio estimation, 15Hz Anti-lock Braking System (ABS) solenoid pulsing, and brake rotor thermals. |
| `eps_node` | **Electric Power Steering** | `vcan0` | `0x130` (TX)<br>`0x131`, `0x100` (RX) | Simulates rack & pinion dynamics, torsion bar driver hand torque sensor, speed-sensitive assist motor torque curves, and Lane Keeping Assist (LKA) corrective torque overlay. |
| `adas_node` | **ADAS Radar & Vision** | `vcan0` | `0x300` (TX)<br>`0x100` (RX) | Simulates 77 GHz forward radar and vision perception stack. Calculates lead distance, relative closing speed, Time-To-Collision (TTC), Forward Collision Warning (FCW), AEB requests, and lane departure warnings. |
| `bcm_node` | **Body Control Module** | `vcan1` | `0x200` (TX)<br>`0x201` (RX) | Controls door locks (FL, FR, RL, RR), low beams, high beams, hazard flasher, brake lights, wipers, and ambient light sensor. |
| `hmi_node` | **Cockpit Driver Interface** | `vcan1` | `0x210` (TX) | Simulates driver accelerator pedal (0-100%), brake pedal (0-100%), steering angle, gear selector (P/R/N/D), turn signals, and drive modes (ECO/COMFORT/SPORT). |
| `hvac_node` | **HVAC & Thermal Management** | `vcan1` | `0x220` (TX)<br>`0x221` (RX) | Simulates cabin interior climate control, AC heat pump compressor electrical load, evaporator core, and liquid coolant loop plate for high-voltage battery and inverter chilling. |
| `telematics_node` | **Telematics & Cloud Gateway** | `vcan1` | `0x400` (TX)<br>`0x401` (RX) | Simulates 5G cellular modem (CSQ signal), GNSS RTK 3D positioning, OEM cloud telemetry ping, and A/B dual-partition Over-The-Air (OTA) firmware update lifecycle. |

---

## 3. Full CAN Protocol Matrix

All messages use packed standard 11-bit CAN frames (DLC <= 8 bytes) defined in [common/vehicle_protocol.h](common/vehicle_protocol.h):

| CAN ID | Struct Name | Domain | Period | Payload Summary (Packed <= 8 Bytes) |
|---|---|---|---|---|
| `0x100` | `PcmTelemetryMsg` | `vcan0` | 100 ms | Speed (km/h * 10), Motor RPM, Actual Torque (Nm), Motor Temp (°C), Gear state |
| `0x101` | `PcmCmdMsg` | `vcan0` | 20 ms | Desired Motor Torque (Nm), Speed Limit, Inverter Enable, Regen Level |
| `0x110` | `BmsTelemetryMsg` | `vcan0` | 200 ms | SoC %, Pack Voltage (V * 10), Pack Current (A * 10), Max Cell Temp (°C), Status, SoH % |
| `0x120` | `BrakeTelemetryMsg` | `vcan0` | 100 ms | Actual Brake Torque (Nm), ABS Active Bitmask, Hydraulic Pressure (bar), Rotor Temps (°C) |
| `0x121` | `BrakeCmdMsg` | `vcan0` | 20 ms | Commanded Friction Torque (Nm), Emergency Braking Enable, Parking Brake Request |
| `0x130` | `SteerTelemetryMsg` | `vcan0` | 50 ms | Actual Pinion Angle (deg), Motor Assist (Nm * 10), Driver Hand Torque (Nm * 10), LKA Active |
| `0x131` | `SteerCmdMsg` | `vcan0` | 20 ms | Driver Steering Angle (deg), LKA Corrective Torque Overlay (Nm * 10), Mode |
| `0x300` | `AdasTelemetryMsg` | `vcan0` | 50 ms | Lead Target Dist (m * 10), Rel Speed (km/h * 10), TTC (s * 10), FCW Alert, AEB Request, LDW |
| `0x301` | `AdasCmdMsg` | `vcan0` | 20 ms | ADAS Operational Mode, AEB Acknowledged, LKA Enable Flag |
| `0x200` | `BcmTelemetryMsg` | `vcan1` | 250 ms | Doors Locked Bitmask, Lights Bitmask, Cabin Temp (°C), Ambient Lux, Wipers |
| `0x201` | `BcmCmdMsg` | `vcan1` | 20 ms | Lock Command (Lock/Unlock), Light Command (LowBeam/Hazard/Brake), Horn Sound |
| `0x210` | `HmiInputMsg` | `vcan1` | 50 ms | Throttle %, Brake %, Steering Angle, Selected Gear (P/R/N/D), Drive Mode, Turn Signal |
| `0x220` | `HvacTelemetryMsg` | `vcan1` | 250 ms | Cabin Temp (°C), Evaporator Temp (°C), Coolant Loop Temp (°C), Compressor Power (W), Blower RPM |
| `0x221` | `HvacCmdMsg` | `vcan1` | 20 ms | Target Cabin Temp (°C), Fan Speed (0-7), AC Compressor Enable, Battery Cooling Request |
| `0x400` | `TelematicsStatusMsg` | `vcan1` | 500 ms | 5G Signal CSQ (0-31), Cloud Connect Status, OTA State, OTA Progress %, GNSS Fix, Ping (ms) |
| `0x401` | `TelematicsCmdMsg` | `vcan1` | 500 ms | Acknowledge Command, Firmware Version, Diagnostic DTC Count, Sync Rate |

---

## 4. How to Build & Run

### 4.1 Prerequisites
Virtual CAN kernel module and `can-utils` (already configured in this environment):
```bash
sudo modprobe vcan
./scripts/setup_vcan.sh
```

### 4.2 Compile All 10 Binaries
```bash
make
```

### 4.3 Run Interactive Central Compute Dashboard
```bash
./scripts/start_sim.sh
```
Press `Ctrl+C` to cleanly terminate all 10 simulation processes.

### 4.4 Run in Background Daemon Mode
```bash
./scripts/start_sim.sh --daemon
```

To view real-time Central Compute HUD:
```bash
tail -f logs/central_compute.log
```

To stop all nodes cleanly:
```bash
./scripts/stop_sim.sh
```

---

## 5. Monitoring CAN Traffic Live
Inspect raw CAN bus frames in real time with high-precision timestamps:
```bash
# Monitor Powertrain, Chassis & Safety bus (0x100, 0x101, 0x110, 0x120, 0x121, 0x130, 0x131, 0x300)
candump -tz vcan0

# Monitor Body, Cockpit, HVAC & Telematics bus (0x200, 0x201, 0x210, 0x220, 0x221, 0x400, 0x401)
candump -tz vcan1
```

---

## 6. Regenerating Educational Study PDFs
To regenerate all 4 comprehensive PDF study guides in `Information/`:
```bash
python3 Information/generate_pdfs.py
```
