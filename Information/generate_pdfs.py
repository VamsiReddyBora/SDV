#!/usr/bin/env python3
"""
generate_pdfs.py - Generates topic-wise educational PDFs for the SDV simulation.
Topics:
  1. 01_CAN_Bus_Fundamentals.pdf
  2. 02_Linux_SocketCAN_Architecture.pdf
  3. 03_SDV_Centralization_And_Zonal_Architecture.pdf
  4. 04_Code_Implementation_Deep_Dive.pdf
"""

import os
from reportlab.lib.pagesizes import letter
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
)
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    """Adds 'Page X of Y' and header/footer to each page."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_header_footer(num_pages)
            super().showPage()
        super().save()

    def draw_header_footer(self, page_count):
        self.saveState()
        self.setFont("Helvetica", 9)
        self.setFillColor(colors.HexColor("#718096"))
        
        # Header (pages > 1)
        if self._pageNumber > 1:
            self.drawString(54, 750, "Software-Defined Vehicle (SDV) Engineering Guide")
            self.setStrokeColor(colors.HexColor("#CBD5E0"))
            self.setLineWidth(0.5)
            self.line(54, 742, letter[0] - 54, 742)

        # Footer
        text = f"Page {self._pageNumber} of {page_count}"
        self.drawRightString(letter[0] - 54, 36, text)
        self.drawString(54, 36, "Confidential - SDV Linux Virtual CAN Architecture")
        self.setStrokeColor(colors.HexColor("#CBD5E0"))
        self.setLineWidth(0.5)
        self.line(54, 48, letter[0] - 54, 48)
        self.restoreState()

def create_base_styles():
    styles = getSampleStyleSheet()
    
    title_style = ParagraphStyle(
        'DocTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=20,
        leading=24,
        textColor=colors.HexColor("#1A365D"),
        spaceAfter=12
    )
    
    subtitle_style = ParagraphStyle(
        'DocSubTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=colors.HexColor("#2B6CB0"),
        spaceAfter=16
    )

    h1_style = ParagraphStyle(
        'DocH1',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=13,
        leading=17,
        textColor=colors.HexColor("#1A365D"),
        spaceBefore=12,
        spaceAfter=6,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'DocH2',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=11,
        leading=15,
        textColor=colors.HexColor("#2D3748"),
        spaceBefore=8,
        spaceAfter=4,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'DocBody',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=9.5,
        leading=13.5,
        textColor=colors.HexColor("#2D3748"),
        spaceAfter=6
    )

    table_cell_style = ParagraphStyle(
        'DocTableCell',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=8.5,
        leading=11.5,
        textColor=colors.HexColor("#2D3748")
    )

    bullet_style = ParagraphStyle(
        'DocBullet',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=9.5,
        leading=13.5,
        textColor=colors.HexColor("#2D3748"),
        leftIndent=15,
        firstLineIndent=-10,
        spaceAfter=4
    )

    code_style = ParagraphStyle(
        'DocCode',
        parent=styles['Normal'],
        fontName='Courier',
        fontSize=8.0,
        leading=10.5,
        textColor=colors.HexColor("#1A202C")
    )

    callout_style = ParagraphStyle(
        'DocCallout',
        parent=styles['Normal'],
        fontName='Helvetica-Oblique',
        fontSize=9.0,
        leading=13.0,
        textColor=colors.HexColor("#2C5282")
    )

    return {
        'title': title_style,
        'subtitle': subtitle_style,
        'h1': h1_style,
        'h2': h2_style,
        'body': body_style,
        'table_cell': table_cell_style,
        'bullet': bullet_style,
        'code': code_style,
        'callout': callout_style
    }

def format_code_block(code_text, styles):
    t = Table([[Paragraph(code_text.replace("\n", "<br/>").replace(" ", "&nbsp;"), styles['code'])]], colWidths=[letter[0] - 108])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EDF2F7")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#CBD5E0")),
        ('LEFTPADDING', (0,0), (-1,-1), 8),
        ('RIGHTPADDING', (0,0), (-1,-1), 8),
        ('TOPPADDING', (0,0), (-1,-1), 6),
        ('BOTTOMPADDING', (0,0), (-1,-1), 6),
    ]))
    return t

def format_callout(text, styles):
    p = Paragraph(f"<b>Key Takeaway:</b> {text}", styles['callout'])
    t = Table([[p]], colWidths=[letter[0] - 108])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EBF8FF")),
        ('LINELEFT', (0,0), (-1,-1), 3.5, colors.HexColor("#3182CE")),
        ('BOX', (0,0), (-1,-1), 0.5, colors.HexColor("#BEE3F8")),
        ('LEFTPADDING', (0,0), (-1,-1), 10),
        ('RIGHTPADDING', (0,0), (-1,-1), 10),
        ('TOPPADDING', (0,0), (-1,-1), 6),
        ('BOTTOMPADDING', (0,0), (-1,-1), 6),
    ]))
    return t

# =========================================================================
# 1. CAN Bus Fundamentals PDF
# =========================================================================
def build_pdf_01(output_path):
    styles = create_base_styles()
    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=54, rightMargin=54, topMargin=54, bottomMargin=54
    )
    story = []

    story.append(Paragraph("Part 1: Controller Area Network (CAN) Fundamentals", styles['title']))
    story.append(Paragraph("The Foundation of Automotive Embedded Communication", styles['subtitle']))
    story.append(Spacer(1, 10))

    story.append(Paragraph("1. The Historical Automotive Problem", styles['h1']))
    story.append(Paragraph(
        "Prior to the 1980s, vehicle electrical components used point-to-point physical wiring. "
        "Every single switch was wired directly to its corresponding actuator. As safety and comfort features "
        "(ABS, cruise control, electronic ignition, central locking) proliferated, vehicle wire harnesses exceeded "
        "several kilometers in length and 100+ kg in weight, introducing severe reliability, maintenance, and space issues.",
        styles['body']
    ))
    story.append(Paragraph(
        "In 1986, Robert Bosch GmbH invented the <b>Controller Area Network (CAN)</b> to replace point-to-point "
        "wiring with a shared, high-reliability, two-wire digital bus.",
        styles['body']
    ))

    story.append(Paragraph("2. Physical Layer: Differential Signaling", styles['h1']))
    story.append(Paragraph(
        "A CAN bus operates using two balanced, twisted copper wires: <b>CAN High (CAN_H)</b> and <b>CAN Low (CAN_L)</b>, "
        "terminated at both ends with 120-ohm resistors (giving an overall nominal bus impedance of 60 ohms).",
        styles['body']
    ))
    story.append(Paragraph(
        "• <b>Recessive State (Logical '1'):</b> Both CAN_H and CAN_L sit at approximately 2.5V (Differential voltage = 0V).<br/>"
        "• <b>Dominant State (Logical '0'):</b> CAN_H is driven to ~3.5V and CAN_L is pulled to ~1.5V (Differential voltage = ~2.0V).",
        styles['bullet']
    ))
    story.append(Paragraph(
        "Because the signal is differential, any electromagnetic interference (EMI) induces equal noise voltages on both lines, "
        "which completely cancels out at the receiver's differential amplifier. This provides exceptional noise immunity in harsh automotive environments.",
        styles['body']
    ))

    story.append(Spacer(1, 6))
    story.append(format_callout(
        "A dominant bit ('0') always overrides a recessive bit ('1') on the physical bus. "
        "This electrical property is the bedrock of CAN's non-destructive bus arbitration.",
        styles
    ))

    story.append(Paragraph("3. Non-Destructive Bitwise Arbitration", styles['h1']))
    story.append(Paragraph(
        "Unlike Ethernet (CSMA/CD), where simultaneous transmissions result in collisions and lost packets requiring random backoff delays, "
        "CAN uses <b>lossless bitwise arbitration</b>:",
        styles['body']
    ))
    story.append(Paragraph(
        "1. Every node listens to the bus while transmitting bit-by-bit.<br/>"
        "2. If Node A transmits a recessive '1' but senses a dominant '0' on the wire, it immediately knows a higher-priority node is speaking.<br/>"
        "3. Node A backs off instantly and transitions to listening mode without disrupting the winning frame.<br/>"
        "4. The highest-priority frame (the one with the lowest numerical CAN ID) continues uninterrupted.",
        styles['bullet']
    ))

    story.append(Paragraph("4. Standard 11-Bit Frame Structure (CAN 2.0A)", styles['h1']))
    table_data = [
        [Paragraph("<b>Field</b>", styles['table_cell']), Paragraph("<b>Bits</b>", styles['table_cell']), Paragraph("<b>Purpose</b>", styles['table_cell'])],
        [Paragraph("SOF (Start of Frame)", styles['table_cell']), Paragraph("1", styles['table_cell']), Paragraph("Dominant bit for node clock synchronization.", styles['table_cell'])],
        [Paragraph("Identifier (ID)", styles['table_cell']), Paragraph("11", styles['table_cell']), Paragraph("Message priority and broadcast content identifier.", styles['table_cell'])],
        [Paragraph("RTR", styles['table_cell']), Paragraph("1", styles['table_cell']), Paragraph("Remote Transmission Request (0=Data, 1=Remote Request).", styles['table_cell'])],
        [Paragraph("Control (IDE, r0, DLC)", styles['table_cell']), Paragraph("6", styles['table_cell']), Paragraph("Data Length Code (0 to 8 bytes payload length).", styles['table_cell'])],
        [Paragraph("Data Field", styles['table_cell']), Paragraph("0-64", styles['table_cell']), Paragraph("0 to 8 bytes of application payload data.", styles['table_cell'])],
        [Paragraph("CRC & Delimiter", styles['table_cell']), Paragraph("16", styles['table_cell']), Paragraph("Cyclic Redundancy Check error detection.", styles['table_cell'])],
        [Paragraph("ACK & Delimiter", styles['table_cell']), Paragraph("2", styles['table_cell']), Paragraph("Receiving nodes assert dominant bit to confirm receipt.", styles['table_cell'])],
        [Paragraph("EOF (End of Frame)", styles['table_cell']), Paragraph("7", styles['table_cell']), Paragraph("7 consecutive recessive bits marking frame boundary.", styles['table_cell'])],
    ]
    t = Table(table_data, colWidths=[120, 50, 334])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
    ]))
    story.append(t)

    doc.build(story, canvasmaker=NumberedCanvas)

# =========================================================================
# 2. Linux SocketCAN Architecture PDF
# =========================================================================
def build_pdf_02(output_path):
    styles = create_base_styles()
    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=54, rightMargin=54, topMargin=54, bottomMargin=54
    )
    story = []

    story.append(Paragraph("Part 2: Linux SocketCAN Architecture", styles['title']))
    story.append(Paragraph("Automotive Networking as Native Linux Network Sockets", styles['subtitle']))
    story.append(Spacer(1, 10))

    story.append(Paragraph("1. The SocketCAN Revolution", styles['h1']))
    story.append(Paragraph(
        "Historically, CAN drivers on Unix and Windows were implemented as character devices (`/dev/can0`) "
        "requiring proprietary DLLs, vendor-specific APIs (Vector XL, Kvaser, PEAK PCAN), and cumbersome `ioctl()` calls. "
        "A major limitation was that only one process could hold the character device open at any time.",
        styles['body']
    ))
    story.append(Paragraph(
        "In 2006, Volkswagen Research and the Linux community developed <b>SocketCAN</b>. "
        "SocketCAN models CAN interfaces as native Linux network devices (just like `eth0` or `wlan0`), "
        "introducing the `PF_CAN` protocol family. Multiple independent user-space processes can concurrently read "
        "and write CAN frames through standard Berkeley socket APIs (`socket()`, `bind()`, `read()`, `write()`, `select()`).",
        styles['body']
    ))

    story.append(Paragraph("2. Virtual CAN (`vcan`) Kernel Driver", styles['h1']))
    story.append(Paragraph(
        "The Linux kernel module `vcan` provides virtual loopback CAN interfaces without requiring physical hardware transceivers:",
        styles['body']
    ))
    story.append(Paragraph(
        "• The frame enters the Linux network subsystem through `can_send()`.<br/>"
        "• The kernel replicates the frame to its loopback mechanism.<br/>"
        "• All other processes bound to `vcan0` receive the frame in their socket queues.<br/>"
        "• Kernel filters (`CAN_RAW_FILTER`) discard unwanted frames in kernel-space before waking user-space threads.",
        styles['bullet']
    ))

    story.append(Paragraph("3. SocketCAN in C: The API Workflow", styles['h1']))
    c_code_example = """#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

// 1. Create a raw CAN socket
int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);

// 2. Discover interface index for "vcan0"
struct ifreq ifr;
strcpy(ifr.ifr_name, "vcan0");
ioctl(s, SIOCGIFINDEX, &ifr);

// 3. Bind socket to interface
struct sockaddr_can addr;
memset(&addr, 0, sizeof(addr));
addr.can_family = AF_CAN;
addr.can_ifindex = ifr.ifr_ifindex;
bind(s, (struct sockaddr *)&addr, sizeof(addr));

// 4. Send a standard CAN frame
struct can_frame frame;
frame.can_id = 0x100;
frame.can_dlc = 8;
memcpy(frame.data, payload, 8);
write(s, &frame, sizeof(struct can_frame));"""

    story.append(format_code_block(c_code_example, styles))
    story.append(Spacer(1, 6))

    story.append(Paragraph("4. Essential can-utils Diagnostics", styles['h1']))
    tools_table = [
        [Paragraph("<b>Command</b>", styles['table_cell']), Paragraph("<b>Usage Example</b>", styles['table_cell']), Paragraph("<b>Purpose</b>", styles['table_cell'])],
        [Paragraph("candump", styles['table_cell']), Paragraph("candump -tz vcan0", styles['table_cell']), Paragraph("Dumps incoming CAN traffic with timestamps.", styles['table_cell'])],
        [Paragraph("cansend", styles['table_cell']), Paragraph("cansend vcan1 201#0100000000000000", styles['table_cell']), Paragraph("Transmits a single custom CAN frame manually.", styles['table_cell'])],
        [Paragraph("cangen", styles['table_cell']), Paragraph("cangen vcan0 -g 10", styles['table_cell']), Paragraph("Generates random CAN traffic for load testing.", styles['table_cell'])],
        [Paragraph("canplayer", styles['table_cell']), Paragraph("canplayer -I trace.log", styles['table_cell']), Paragraph("Replays recorded CAN logs accurately.", styles['table_cell'])],
    ]
    t2 = Table(tools_table, colWidths=[90, 200, 214])
    t2.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(t2)

    doc.build(story, canvasmaker=NumberedCanvas)

# =========================================================================
# 3. SDV Centralization & Zonal Architecture PDF
# =========================================================================
def build_pdf_03(output_path):
    styles = create_base_styles()
    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=54, rightMargin=54, topMargin=54, bottomMargin=54
    )
    story = []

    story.append(Paragraph("Part 3: Software-Defined Vehicle (SDV) Centralization", styles['title']))
    story.append(Paragraph("Migrating from Distributed ECUs to High-Performance Computing (HPC)", styles['subtitle']))
    story.append(Spacer(1, 8))

    story.append(Paragraph("1. Paradigm Shift: Legacy Distributed ECUs vs. SDV", styles['h1']))
    story.append(Paragraph(
        "Traditional vehicle electrical architectures are <b>distributed domain networks</b>. In this legacy model, "
        "each vehicular feature (door lock, wiper, seat motor, inverter, battery management) has its own dedicated micro-controller (ECU) "
        "supplied by different Tier-1 vendors with proprietary, static firmware.",
        styles['body']
    ))
    story.append(Paragraph(
        "A modern luxury vehicle often contains over <b>100 distributed ECUs</b>. This leads to massive architectural friction: "
        "implementing a feature requiring multiple domains (e.g. automatically locking doors and adjusting suspension when speed increases) "
        "requires coordinating software releases from multiple vendors.",
        styles['body']
    ))

    comparison_data = [
        [Paragraph("<b>Metric</b>", styles['table_cell']), Paragraph("<b>Legacy Distributed Architecture</b>", styles['table_cell']), Paragraph("<b>Software-Defined Vehicle (SDV)</b>", styles['table_cell'])],
        [Paragraph("ECU Count", styles['table_cell']), Paragraph("70 - 120+ dedicated microcontrollers", styles['table_cell']), Paragraph("1 - 3 Central Computers + Zonal Gateways", styles['table_cell'])],
        [Paragraph("Control Logic", styles['table_cell']), Paragraph("Burned into fixed ECU firmware", styles['table_cell']), Paragraph("Consolidated in central software tasks", styles['table_cell'])],
        [Paragraph("Wiring Harness", styles['table_cell']), Paragraph("Heavy point-to-point domain wiring", styles['table_cell']), Paragraph("Short zonal wiring to local edge hubs", styles['table_cell'])],
        [Paragraph("Upgradability", styles['table_cell']), Paragraph("Dealership flashing / physical recall", styles['table_cell']), Paragraph("Over-The-Air (OTA) continuous updates", styles['table_cell'])],
        [Paragraph("Feature Velocity", styles['table_cell']), Paragraph("Years (hardware development cycle)", styles['table_cell']), Paragraph("Weeks (agile software deployment)", styles['table_cell'])],
    ]
    t = Table(comparison_data, colWidths=[90, 205, 209])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
    ]))
    story.append(t)
    story.append(Spacer(1, 6))

    story.append(Paragraph("2. Complete 10-Node Architecture Topology", styles['h1']))
    story.append(Paragraph(
        "Our SDV platform organizes all vehicle functions into two distinct virtual buses and 10 independent C processes:",
        styles['body']
    ))

    nodes_table = [
        [Paragraph("<b>Node Binary</b>", styles['table_cell']), Paragraph("<b>ECU Subsystem</b>", styles['table_cell']), Paragraph("<b>Bus Interface</b>", styles['table_cell']), Paragraph("<b>Central Role & Responsibility</b>", styles['table_cell'])],
        [Paragraph("central_compute", styles['table_cell']), Paragraph("Central Vehicle Computer (CVC)", styles['table_cell']), Paragraph("vcan0 & vcan1", styles['table_cell']), Paragraph("Dual-homed orchestrator; arbitrates torque, braking, steering, safety, thermal loops & displays live HUD.", styles['table_cell'])],
        [Paragraph("pcm_node", styles['table_cell']), Paragraph("Powertrain Control Module", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Simulates electric motor, inverter, vehicle inertia, aerodynamic drag, speed, and torque.", styles['table_cell'])],
        [Paragraph("bms_node", styles['table_cell']), Paragraph("Battery Management System", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("400V traction pack, Coulomb-counting SoC %, pack current, and internal Joule heating.", styles['table_cell'])],
        [Paragraph("brake_node", styles['table_cell']), Paragraph("ABS & Stability Control", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Master cylinder pressure, 4-wheel slip detection, 15Hz ABS modulation pulsing, and rotor thermals.", styles['table_cell'])],
        [Paragraph("eps_node", styles['table_cell']), Paragraph("Electric Power Steering", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Speed-sensitive assist torque motor, driver hand torque sensor, and LKA overlay.", styles['table_cell'])],
        [Paragraph("adas_node", styles['table_cell']), Paragraph("Radar & Vision Perception", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Forward 77GHz radar, closing velocity, Time-To-Collision (TTC), FCW warnings, and AEB requests.", styles['table_cell'])],
        [Paragraph("bcm_node", styles['table_cell']), Paragraph("Body Control Module", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Door locks, low/high beams, brake lights, wipers, horn, and ambient light sensing.", styles['table_cell'])],
        [Paragraph("hmi_node", styles['table_cell']), Paragraph("Driver Cockpit Interface", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Simulates driver accelerator/brake pedals, steering wheel, gear selector, and drive modes.", styles['table_cell'])],
        [Paragraph("hvac_node", styles['table_cell']), Paragraph("Climate & Thermal Loop", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Dual cabin climate, evaporator, heat pump compressor load, and battery liquid coolant loop.", styles['table_cell'])],
        [Paragraph("telematics_node", styles['table_cell']), Paragraph("Telematics TCU & OTA", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Cellular 5G modem, GNSS RTK positioning, cloud telemetry ping, and A/B partition OTA updates.", styles['table_cell'])],
    ]
    t_nodes = Table(nodes_table, colWidths=[80, 110, 65, 249])
    t_nodes.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 2.5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2.5),
    ]))
    story.append(t_nodes)
    story.append(Spacer(1, 6))

    story.append(Paragraph("3. Cross-Domain Central Software Orchestration", styles['h1']))
    story.append(Paragraph(
        "Crucially, <b>Bus 0 (`vcan0`) and Bus 1 (`vcan1`) are never bridged directly</b>. The Central Compute process acts as an active gateway and safety supervisor:",
        styles['body']
    ))
    story.append(Paragraph(
        "• <b>Autonomous Emergency Braking (AEB):</b> `adas_node` on `vcan0` detects closing target and triggers AEB Level 2. Central Compute intercepts the signal, commands -180 Nm motor regen to `pcm_node`, commands 2400 Nm hydraulic pressure to `brake_node` (provoking ABS modulation), and flashes hazard lights via `bcm_node` on `vcan1`.<br/>"
        "• <b>Brake Blending:</b> Driver brake pedal (Bus 1) is dynamically arbitrated by Central Compute: light braking is harvested as electric regen (Bus 0), while heavy braking engages friction hydraulics (Bus 0).<br/>"
        "• <b>Lane Keeping Assist (LKA):</b> Lane drift detected by ADAS vision (Bus 0) causes Central Compute to inject corrective torque overlay directly into `eps_node` (Bus 0).<br/>"
        "• <b>Battery Active Chilling:</b> When `bms_node` (Bus 0) signals high cell temperatures, Central Compute commands `hvac_node` (Bus 1) to engage chiller heat pump valves to cool the battery plate.<br/>"
        "• <b>OTA Firmware Campaigns:</b> Cloud-directed firmware updates received by `telematics_node` (Bus 1) are coordinated by Central Compute across all vehicle nodes.",
        styles['bullet']
    ))

    doc.build(story, canvasmaker=NumberedCanvas)

# =========================================================================
# 4. Code Implementation Deep Dive PDF
# =========================================================================
def build_pdf_04(output_path):
    styles = create_base_styles()
    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=54, rightMargin=54, topMargin=54, bottomMargin=54
    )
    story = []

    story.append(Paragraph("Part 4: C Code Implementation Deep Dive", styles['title']))
    story.append(Paragraph("Mathematical Physics, SocketCAN Multiplexing & Full Protocol Matrix", styles['subtitle']))
    story.append(Spacer(1, 8))

    story.append(Paragraph("1. The 10-Node CAN Protocol Matrix (Standard 8-Byte Packed Frames)", styles['h1']))
    story.append(Paragraph(
        "All messages adhere to standard 11-bit identifiers and strict `#pragma pack(push, 1)` structs defined in `common/vehicle_protocol.h`:",
        styles['body']
    ))

    proto_table = [
        [Paragraph("<b>CAN ID</b>", styles['table_cell']), Paragraph("<b>Message Name</b>", styles['table_cell']), Paragraph("<b>Bus</b>", styles['table_cell']), Paragraph("<b>Signals & Fixed-Point Encoding</b>", styles['table_cell'])],
        [Paragraph("0x100", styles['table_cell']), Paragraph("PcmTelemetryMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Speed (kph*10), Motor RPM, Actual Torque (Nm), Motor Temp (°C), Gear.", styles['table_cell'])],
        [Paragraph("0x101", styles['table_cell']), Paragraph("PcmCmdMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Target Torque (Nm), Speed Limit, Inverter Enable, Regen Level.", styles['table_cell'])],
        [Paragraph("0x110", styles['table_cell']), Paragraph("BmsTelemetryMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("SoC %, Pack Voltage (V*10), Pack Current (A*10), Max Cell Temp, Status, SoH.", styles['table_cell'])],
        [Paragraph("0x120", styles['table_cell']), Paragraph("BrakeTelemetryMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Brake Torque (Nm), ABS Active Mask (FL/FR/RL/RR), Pressure (bar), Rotor Temp (°C).", styles['table_cell'])],
        [Paragraph("0x121", styles['table_cell']), Paragraph("BrakeCmdMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Req Brake Torque (Nm), Emergency Brake Enable, Parking Brake Request.", styles['table_cell'])],
        [Paragraph("0x130", styles['table_cell']), Paragraph("SteerTelemetryMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Actual Pinion Angle (deg), Motor Assist (Nm*10), Hand Torque (Nm*10), LKA State.", styles['table_cell'])],
        [Paragraph("0x131", styles['table_cell']), Paragraph("SteerCmdMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Driver Steering Angle (deg), LKA Corrective Torque Overlay, Steering Mode.", styles['table_cell'])],
        [Paragraph("0x300", styles['table_cell']), Paragraph("AdasTelemetryMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("Lead Distance (m*10), Rel Speed (kph*10), TTC (s*10), FCW Alert, AEB Request, LDW.", styles['table_cell'])],
        [Paragraph("0x301", styles['table_cell']), Paragraph("AdasCmdMsg", styles['table_cell']), Paragraph("vcan0", styles['table_cell']), Paragraph("ADAS Mode, Emergency Braking Ack, Lane Keeping Assist Enable.", styles['table_cell'])],
        [Paragraph("0x200", styles['table_cell']), Paragraph("BcmTelemetryMsg", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Doors Locked bitmask, Lights Active bitmask, Cabin Temp, Ambient Lux, Wipers.", styles['table_cell'])],
        [Paragraph("0x201", styles['table_cell']), Paragraph("BcmCmdMsg", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Lock Command (Lock/Unlock), Light Command (LowBeam/Hazard/Brake), Horn.", styles['table_cell'])],
        [Paragraph("0x210", styles['table_cell']), Paragraph("HmiInputMsg", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Throttle %, Brake %, Steering Angle, Selected Gear (P/R/N/D), Drive Mode.", styles['table_cell'])],
        [Paragraph("0x220", styles['table_cell']), Paragraph("HvacTelemetryMsg", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Cabin Temp, Evaporator Temp, Coolant Loop Temp, Compressor Power (W), Blower RPM.", styles['table_cell'])],
        [Paragraph("0x221", styles['table_cell']), Paragraph("HvacCmdMsg", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Target Cabin Temp, Fan Speed (0-7), AC Compress Enable, Battery Chill Req.", styles['table_cell'])],
        [Paragraph("0x400", styles['table_cell']), Paragraph("TelematicsStatusMsg", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("5G CSQ (0-31), Cloud Connect State, OTA State, OTA Progress %, GNSS Fix, Ping ms.", styles['table_cell'])],
        [Paragraph("0x401", styles['table_cell']), Paragraph("TelematicsCmdMsg", styles['table_cell']), Paragraph("vcan1", styles['table_cell']), Paragraph("Ack Command, Firmware Version, Diagnostic DTC Count, Sync Rate.", styles['table_cell'])],
    ]
    t_proto = Table(proto_table, colWidths=[45, 110, 45, 304])
    t_proto.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 2),
        ('BOTTOMPADDING', (0,0), (-1,-1), 2),
    ]))
    story.append(t_proto)
    story.append(Spacer(1, 6))

    story.append(Paragraph("2. Dual-Homed POSIX select() Multiplexing", styles['h1']))
    story.append(Paragraph(
        "In `nodes/central_compute.c`, the controller reads concurrently from both domain buses without blocking:",
        styles['body']
    ))

    select_snippet = """fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(sock_pt, &read_fds);    // vcan0: Powertrain, Chassis & Safety
FD_SET(sock_body, &read_fds);  // vcan1: Body, Cockpit, HVAC & Telematics

struct timeval tv = { .tv_sec = 0, .tv_usec = 10000 }; // 10ms timeout
int ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
if (ret > 0) {
    if (FD_ISSET(sock_pt, &read_fds)) {
        // Read 0x100, 0x110, 0x120, 0x130, 0x300 frames
    }
    if (FD_ISSET(sock_body, &read_fds)) {
        // Read 0x200, 0x210, 0x220, 0x400 frames
    }
}"""
    story.append(format_code_block(select_snippet, styles))
    story.append(Spacer(1, 6))

    story.append(Paragraph("3. Mathematical Physics & Safety Algorithms", styles['h1']))
    story.append(Paragraph(
        "• <b>Radar Time-To-Collision (TTC):</b> In `adas_node.c`, Doppler closing velocity calculates $TTC = \\frac{d_{lead}}{v_{closing}}$. If $TTC < 1.3\\text{s}$, AEB Full Emergency is asserted.<br/>"
        "• <b>Braking Blending & ABS Slip:</b> In `brake_node.c`, master cylinder pressure translates to torque ($\\tau = P \\cdot 21.5$). At high deceleration, wheel slip $s = \\frac{v_{veh} - v_{wheel}}{v_{veh}} > 0.18$ activates 15Hz solenoid valve dumping.<br/>"
        "• <b>EPS Speed-Sensitive Assist:</b> Assist gain scales dynamically with vehicle velocity: $K_{assist}(v) = 3.6 - 2.2 \\cdot \\frac{\\min(v, 120)}{120}$, delivering light parking effort and stiff highway stability.<br/>"
        "• <b>Battery Coulomb Counting & Joule Heating:</b> In `bms_node.c`, power is converted into current $I_{pack} = \\frac{P_{elec}}{V_{pack}}$, with thermal heating $P_{heat} = I_{pack}^2 \\cdot R_{int}$.<br/>"
        "• <b>OTA State Machine:</b> In `telematics_node.c`, firmware updates cycle from IDLE -> DOWNLOADING -> VERIFYING -> FLASHING (A/B dual partition) -> COMPLETE.",
        styles['bullet']
    ))

    doc.build(story, canvasmaker=NumberedCanvas)

def main():
    info_dir = "/home/bob/workspace/sdv/Information"
    os.makedirs(info_dir, exist_ok=True)

    print("Generating Part 1: CAN Bus Fundamentals...")
    build_pdf_01(os.path.join(info_dir, "01_CAN_Bus_Fundamentals.pdf"))

    print("Generating Part 2: Linux SocketCAN Architecture...")
    build_pdf_02(os.path.join(info_dir, "02_Linux_SocketCAN_Architecture.pdf"))

    print("Generating Part 3: SDV Centralization & Zonal Architecture...")
    build_pdf_03(os.path.join(info_dir, "03_SDV_Centralization_And_Zonal_Architecture.pdf"))

    print("Generating Part 4: Code Implementation Deep Dive...")
    build_pdf_04(os.path.join(info_dir, "04_Code_Implementation_Deep_Dive.pdf"))

    print("All educational PDFs successfully generated in:", info_dir)

if __name__ == "__main__":
    main()
