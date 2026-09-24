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
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, Preformatted, KeepTogether
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
        fontSize=22,
        leading=26,
        textColor=colors.HexColor("#1A365D"),
        spaceAfter=15
    )
    
    subtitle_style = ParagraphStyle(
        'DocSubTitle',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=13,
        leading=17,
        textColor=colors.HexColor("#2B6CB0"),
        spaceAfter=20
    )

    h1_style = ParagraphStyle(
        'DocH1',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=15,
        leading=19,
        textColor=colors.HexColor("#1A365D"),
        spaceBefore=14,
        spaceAfter=8,
        keepWithNext=True
    )

    h2_style = ParagraphStyle(
        'DocH2',
        parent=styles['Normal'],
        fontName='Helvetica-Bold',
        fontSize=12,
        leading=16,
        textColor=colors.HexColor("#2D3748"),
        spaceBefore=10,
        spaceAfter=6,
        keepWithNext=True
    )

    body_style = ParagraphStyle(
        'DocBody',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=10,
        leading=14,
        textColor=colors.HexColor("#2D3748"),
        spaceAfter=8
    )

    bullet_style = ParagraphStyle(
        'DocBullet',
        parent=styles['Normal'],
        fontName='Helvetica',
        fontSize=10,
        leading=14,
        textColor=colors.HexColor("#2D3748"),
        leftIndent=15,
        firstLineIndent=-10,
        spaceAfter=4
    )

    code_style = ParagraphStyle(
        'DocCode',
        parent=styles['Normal'],
        fontName='Courier',
        fontSize=8.5,
        leading=11,
        textColor=colors.HexColor("#1A202C")
    )

    callout_style = ParagraphStyle(
        'DocCallout',
        parent=styles['Normal'],
        fontName='Helvetica-Oblique',
        fontSize=9.5,
        leading=13.5,
        textColor=colors.HexColor("#2C5282")
    )

    return {
        'title': title_style,
        'subtitle': subtitle_style,
        'h1': h1_style,
        'h2': h2_style,
        'body': body_style,
        'bullet': bullet_style,
        'code': code_style,
        'callout': callout_style
    }

def format_code_block(code_text, styles):
    t = Table([[Paragraph(code_text.replace("\n", "<br/>").replace(" ", "&nbsp;"), styles['code'])]], colWidths=[letter[0] - 108])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EDF2F7")),
        ('BOX', (0,0), (-1,-1), 1, colors.HexColor("#CBD5E0")),
        ('LEFTPADDING', (0,0), (-1,-1), 10),
        ('RIGHTPADDING', (0,0), (-1,-1), 10),
        ('TOPPADDING', (0,0), (-1,-1), 8),
        ('BOTTOMPADDING', (0,0), (-1,-1), 8),
    ]))
    return t

def format_callout(text, styles):
    p = Paragraph(f"<b>Key Takeaway:</b> {text}", styles['callout'])
    t = Table([[p]], colWidths=[letter[0] - 108])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,-1), colors.HexColor("#EBF8FF")),
        ('LINELEFT', (0,0), (-1,-1), 3.5, colors.HexColor("#3182CE")),
        ('BOX', (0,0), (-1,-1), 0.5, colors.HexColor("#BEE3F8")),
        ('LEFTPADDING', (0,0), (-1,-1), 12),
        ('RIGHTPADDING', (0,0), (-1,-1), 12),
        ('TOPPADDING', (0,0), (-1,-1), 8),
        ('BOTTOMPADDING', (0,0), (-1,-1), 8),
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

    story.append(Paragraph("4. Classical CAN Frame Structure", styles['h1']))
    story.append(Paragraph(
        "A standard CAN 2.0B frame comprises the following fields:",
        styles['body']
    ))

    frame_table_data = [
        [Paragraph("<b>Field</b>", styles['body']), Paragraph("<b>Length</b>", styles['body']), Paragraph("<b>Description</b>", styles['body'])],
        [Paragraph("SOF", styles['body']), Paragraph("1 bit", styles['body']), Paragraph("Start of Frame (dominant bit to synchronize clocks).", styles['body'])],
        [Paragraph("CAN ID", styles['body']), Paragraph("11 bits", styles['body']), Paragraph("Message identifier & priority indicator (e.g. 0x100).", styles['body'])],
        [Paragraph("RTR", styles['body']), Paragraph("1 bit", styles['body']), Paragraph("Remote Transmission Request (data vs request frame).", styles['body'])],
        [Paragraph("Control / DLC", styles['body']), Paragraph("6 bits", styles['body']), Paragraph("Data Length Code (specifies 0 to 8 bytes of data).", styles['body'])],
        [Paragraph("Data Field", styles['body']), Paragraph("0-8 bytes", styles['body']), Paragraph("Application payload (e.g., speed, torque, SoC).", styles['body'])],
        [Paragraph("CRC Field", styles['body']), Paragraph("16 bits", styles['body']), Paragraph("Cyclic Redundancy Check for error detection.", styles['body'])],
        [Paragraph("ACK Slot", styles['body']), Paragraph("2 bits", styles['body']), Paragraph("Transmitting node sends recessive; any receiver drives dominant to acknowledge.", styles['body'])],
        [Paragraph("EOF", styles['body']), Paragraph("7 bits", styles['body']), Paragraph("End of Frame (recessive bits).", styles['body'])],
    ]
    t = Table(frame_table_data, colWidths=[100, 70, 334])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(t)

    story.append(Paragraph("5. Broadcast & Content-Addressed Model", styles['h1']))
    story.append(Paragraph(
        "In CAN, <b>there are no destination node addresses</b>. A transmitter broadcasts a message labeled by ID "
        "(e.g., ID 0x100 = 'Powertrain Speed & RPM'). All ECUs connected to the bus receive the message simultaneously. "
        "Each ECU configures hardware acceptance filters so it only interrupts the CPU for CAN IDs relevant to its function.",
        styles['body']
    ))

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

    story.append(Paragraph("Part 2: Linux SocketCAN & Virtual CAN (vcan)", styles['title']))
    story.append(Paragraph("Integrating Automotive Networks into the POSIX Operating System", styles['subtitle']))
    story.append(Spacer(1, 10))

    story.append(Paragraph("1. The Philosophy of SocketCAN", styles['h1']))
    story.append(Paragraph(
        "Historically, proprietary CAN drivers in Linux treated CAN controllers as serial character devices (`/dev/can0`), "
        "requiring custom `ioctl()` commands and preventing multiple applications from accessing the bus concurrently without complex user-space multiplexers.",
        styles['body']
    ))
    story.append(Paragraph(
        "In 2008, Volkswagen Research contributed <b>SocketCAN</b> to the mainline Linux kernel. SocketCAN models CAN controllers "
        "as standard <b>network interfaces</b> (like `eth0` or `wlan0`). Applications interact with CAN using the familiar "
        "BSD socket API (`socket()`, `bind()`, `read()`, `write()`, `select()`).",
        styles['body']
    ))

    story.append(Paragraph("2. The Virtual CAN (`vcan`) Driver", styles['h1']))
    story.append(Paragraph(
        "The Linux kernel module `vcan` implements virtual CAN interfaces entirely in software without needing physical CAN hardware. "
        "When an application writes a CAN frame to `vcan0`:",
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
    story.append(Spacer(1, 8))

    story.append(Paragraph("4. Essential can-utils Diagnostics", styles['h1']))
    story.append(Paragraph(
        "The open-source `can-utils` package provides low-level diagnostics and traffic inspection tools:",
        styles['body']
    ))

    tools_table = [
        [Paragraph("<b>Command</b>", styles['body']), Paragraph("<b>Usage Example</b>", styles['body']), Paragraph("<b>Purpose</b>", styles['body'])],
        [Paragraph("candump", styles['body']), Paragraph("candump -tz vcan0", styles['body']), Paragraph("Dumps incoming CAN traffic with timestamps.", styles['body'])],
        [Paragraph("cansend", styles['body']), Paragraph("cansend vcan1 201#0100000000000000", styles['body']), Paragraph("Transmits a single custom CAN frame manually.", styles['body'])],
        [Paragraph("cangen", styles['body']), Paragraph("cangen vcan0 -g 10", styles['body']), Paragraph("Generates random CAN traffic for load testing.", styles['body'])],
        [Paragraph("canplayer", styles['body']), Paragraph("canplayer -I trace.log", styles['body']), Paragraph("Replays recorded CAN logs accurately.", styles['body'])],
    ]
    t2 = Table(tools_table, colWidths=[90, 200, 214])
    t2.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 4),
        ('BOTTOMPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(t2)

    story.append(Spacer(1, 10))
    story.append(format_callout(
        "Because SocketCAN is native to the Linux network stack, standard Unix IPC and async tools "
        "(epoll, select, libuv, systemd socket activation) work seamlessly with vehicle communication.",
        styles
    ))

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
    story.append(Spacer(1, 10))

    story.append(Paragraph("1. Paradigm Shift: Legacy vs. SDV", styles['h1']))
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
        [Paragraph("<b>Metric</b>", styles['body']), Paragraph("<b>Legacy Distributed Architecture</b>", styles['body']), Paragraph("<b>Software-Defined Vehicle (SDV)</b>", styles['body'])],
        [Paragraph("ECU Count", styles['body']), Paragraph("70 - 120+ dedicated microcontrollers", styles['body']), Paragraph("1 - 3 Central Computers + Zonal Gateways", styles['body'])],
        [Paragraph("Control Logic", styles['body']), Paragraph("Burned into fixed ECU firmware", styles['body']), Paragraph("Consolidated in central software tasks", styles['body'])],
        [Paragraph("Wiring Harness", styles['body']), Paragraph("Heavy point-to-point domain wiring", styles['body']), Paragraph("Short zonal wiring to local edge hubs", styles['body'])],
        [Paragraph("Upgradability", styles['body']), Paragraph("Dealership flashing / physical recall", styles['body']), Paragraph("Over-The-Air (OTA) continuous updates", styles['body'])],
        [Paragraph("Feature Velocity", styles['body']), Paragraph("Years (hardware development cycle)", styles['body']), Paragraph("Weeks (agile software deployment)", styles['body'])],
    ]
    t = Table(comparison_data, colWidths=[90, 205, 209])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), colors.HexColor("#E2E8F0")),
        ('GRID', (0,0), (-1,-1), 0.5, colors.HexColor("#CBD5E0")),
        ('TOPPADDING', (0,0), (-1,-1), 5),
        ('BOTTOMPADDING', (0,0), (-1,-1), 5),
    ]))
    story.append(t)

    story.append(Paragraph("2. Our Dual-Bus Simulation Topology", styles['h1']))
    story.append(Paragraph(
        "To faithfully mirror modern vehicle domain isolation, our simulation organizes vehicle functions into two distinct virtual buses:",
        styles['body']
    ))
    story.append(Paragraph(
        "• <b>Bus 0 (`vcan0`) - Powertrain & High Voltage:</b> Connects the Powertrain Control Module (`pcm_node`) and Battery Management System (`bms_node`). "
        "High-voltage safety and motor control operate on this isolated bus.<br/>"
        "• <b>Bus 1 (`vcan1`) - Body & Cockpit:</b> Connects the Body Control Module (`bcm_node`) and Driver Cockpit Interface (`hmi_node`). "
        "Cabin lighting, locks, and driver pedal inputs reside here.",
        styles['bullet']
    ))
    story.append(Paragraph(
        "Crucially, <b>Bus 0 and Bus 1 are not physically or virtually bridged</b>. The <b>Central Vehicle Computer (`central_compute`)</b> "
        "is the sole dual-homed node connected to both interfaces, enforcing strict safety arbitration, privilege separation, and software routing.",
        styles['body']
    ))

    story.append(Paragraph("3. Cross-Domain Software Coordination", styles['h1']))
    story.append(Paragraph(
        "Consider the <b>Auto-Speed Door Lock</b> feature in our SDV simulation:",
        styles['body']
    ))
    story.append(Paragraph(
        "1. The driver applies accelerator pedal in `hmi_node` (Bus 1).<br/>"
        "2. Central Compute validates battery health from `bms_node` (Bus 0) and commands torque to `pcm_node` (Bus 0).<br/>"
        "3. As vehicle speed accelerates past 15.0 km/h, Central Compute detects the threshold on Bus 0.<br/>"
        "4. Central Compute autonomously dispatches an actuation frame on Bus 1 commanding `bcm_node` to lock all doors.<br/>"
        "5. Neither `pcm_node` nor `bcm_node` knew about each other; the entire feature was realized strictly in central software.",
        styles['bullet']
    ))

    story.append(Spacer(1, 8))
    story.append(format_callout(
        "This exemplifies the core SDV thesis: Hardware is abstracted into standardized sensors and actuators, "
        "while vehicle intelligence and business logic live entirely in central software.",
        styles
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
    story.append(Paragraph("Detailed Walkthrough of Modules, Physics, and Sockets", styles['subtitle']))
    story.append(Spacer(1, 10))

    story.append(Paragraph("1. Fixed-Point CAN Protocol Design", styles['h1']))
    story.append(Paragraph(
        "Examining `common/vehicle_protocol.h`: CAN payloads cannot exceed 8 bytes in standard 2.0B frames. "
        "To transmit non-integer values (e.g. speed of 55.4 km/h, voltage of 398.2V) without wasting 4-byte IEEE floats, "
        "we use fixed-point scaling:",
        styles['body']
    ))
    
    proto_snippet = """#pragma pack(push, 1)
typedef struct {
    uint16_t speed_kph_x10;   // Speed * 10 (0 to 6553.5 km/h) -> 2 bytes
    uint16_t motor_rpm;       // Motor RPM (0 to 15,000 RPM)   -> 2 bytes
    int16_t  motor_torque_nm; // Torque (-500 to +500 Nm)      -> 2 bytes
    int8_t   motor_temp_c;    // Motor temp (-40 to +150 °C)   -> 1 byte
    uint8_t  gear_state;      // 0: P, 1: R, 2: N, 3: D        -> 1 byte
} PcmTelemetryMsg; // Exact total: 8 bytes
#pragma pack(pop)"""
    story.append(format_code_block(proto_snippet, styles))
    story.append(Paragraph(
        "The `#pragma pack(push, 1)` directive instructs GCC not to insert padding bytes between struct members, "
        "guaranteeing identical memory layout across different compilers and architectures.",
        styles['body']
    ))

    story.append(Paragraph("2. Dual-Homed I/O Multiplexing in Central Compute", styles['h1']))
    story.append(Paragraph(
        "In `nodes/central_compute.c`, the controller must read from both `vcan0` and `vcan1` without blocking on either one. "
        "It achieves this using POSIX `select()` multiplexing:",
        styles['body']
    ))

    select_snippet = """fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(sock_pt, &read_fds);    // vcan0 socket
FD_SET(sock_body, &read_fds);  // vcan1 socket

struct timeval tv = { .tv_sec = 0, .tv_usec = 10000 }; // 10ms timeout
int ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

if (ret > 0) {
    if (FD_ISSET(sock_pt, &read_fds)) {
        // Handle incoming Powertrain / BMS frame
    }
    if (FD_ISSET(sock_body, &read_fds)) {
        // Handle incoming Body / Cockpit frame
    }
}"""
    story.append(format_code_block(select_snippet, styles))

    story.append(Paragraph("3. Mathematical Vehicle Physics Simulation", styles['h1']))
    story.append(Paragraph(
        "In `nodes/pcm_node.c`, the vehicle calculates realistic motion dynamics on every loop iteration ($dt = 20\\text{ms}$):",
        styles['body']
    ))
    story.append(Paragraph(
        "• <b>Aerodynamic Drag:</b> $F_{aero} = 0.5 \\cdot \\rho \\cdot C_d \\cdot A \\cdot v^2 \\approx 0.0035 \\cdot v^2$<br/>"
        "• <b>Net Force:</b> $F_{net} = (\\tau_{actual} \\cdot r_{gear}) - (F_{aero} + F_{roll})$<br/>"
        "• <b>Acceleration (Newton's 2nd Law):</b> $a = \\frac{F_{net}}{m_{vehicle}}$ ($m = 1600\\text{ kg}$)<br/>"
        "• <b>Integration:</b> $v_{new} = v_{old} + a \\cdot dt$",
        styles['bullet']
    ))

    story.append(Paragraph("4. Battery Energy & Thermal Dynamics", styles['h1']))
    story.append(Paragraph(
        "In `nodes/bms_node.c`, electrical current is calculated dynamically from mechanical shaft power:",
        styles['body']
    ))
    story.append(Paragraph(
        "$$P_{elec} = \\frac{\\tau \\cdot \\omega}{\\eta} + P_{parasitic} \\quad \\implies \\quad I_{pack} = \\frac{P_{elec}}{V_{pack}}$$<br/>"
        "• <b>State of Charge (SoC):</b> Coulomb counting decreases capacity by $\\Delta Q = I_{pack} \\cdot dt$.<br/>"
        "• <b>Joule Heating:</b> Battery internal heating is simulated via $P_{heat} = I_{pack}^2 \\cdot R_{int}$, causing cell temperature to rise under hard acceleration.",
        styles['bullet']
    ))

    story.append(Paragraph("5. Extensibility: Adding Future Nodes", styles['h1']))
    story.append(Paragraph(
        "To add a 6th node (e.g. ADAS radar):<br/>"
        "1. Define `CAN_ID_ADAS_TELEMETRY` (e.g. `0x300`) and struct `AdasTelemetryMsg` in `common/vehicle_protocol.h`.<br/>"
        "2. Implement `nodes/adas_node.c` using `open_can_socket()` and `can_send_msg()`.<br/>"
        "3. Add `adas_node` to the `Makefile` and `scripts/start_sim.sh`.<br/>"
        "4. Update `central_compute.c` to consume `0x300` and trigger automatic emergency braking.",
        styles['body']
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
