# Engineering Knowledge Base & Study Modules

This folder contains comprehensive, topic-wise technical documentation and downloadable PDF engineering guides for the Software-Defined Vehicle (SDV) simulation.

---

## 📚 Study Modules (PDF Guides)

| Module | PDF Document | Key Topics Covered |
|---|---|---|
| **Part 1** | [**`01_CAN_Bus_Fundamentals.pdf`**](01_CAN_Bus_Fundamentals.pdf) | Historical wiring problem, differential signaling (`CAN_H`/`CAN_L`), dominant vs recessive bits, non-destructive bitwise arbitration, 11-bit CAN frame structure, broadcast addressing. |
| **Part 2** | [**`02_Linux_SocketCAN_Architecture.pdf`**](02_Linux_SocketCAN_Architecture.pdf) | SocketCAN vs character devices, virtual CAN (`vcan`) kernel loopback driver, BSD socket C API (`socket`, `bind`, `read`, `write`), and `can-utils` CLI tools (`candump`, `cansend`, `cangen`). |
| **Part 3** | [**`03_SDV_Centralization_And_Zonal_Architecture.pdf`**](03_SDV_Centralization_And_Zonal_Architecture.pdf) | Transition from 100+ distributed domain ECUs to Central Compute (CVC/HPC), zonal gateways, dual-bus isolation (`vcan0` vs `vcan1`), cross-bus software routing, and auto-door lock supervisor logic. |
| **Part 4** | [**`04_Code_Implementation_Deep_Dive.pdf`**](04_Code_Implementation_Deep_Dive.pdf) | Fixed-point math and packed structs in C, dual-bus `select()` I/O multiplexing in `central_compute`, aerodynamic & motor physics in `pcm_node`, battery energy & Joule heating in `bms_node`, and how to add new nodes. |

---

## 🛠️ Regenerating / Updating PDFs

Whenever code, vehicle protocols, or architecture are updated, all PDFs can be regenerated with:

```bash
python3 Information/generate_pdfs.py
```
*(This script uses Python ReportLab to rebuild the formatted PDFs with headers, footers, code blocks, and dynamic page numbering.)*
