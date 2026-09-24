#ifndef VEHICLE_PROTOCOL_H
#define VEHICLE_PROTOCOL_H

#include <stdint.h>

/* ========================================================================= */
/* Virtual CAN Network Topology                                              */
/* ========================================================================= */
#define CAN_BUS_POWERTRAIN "vcan0"  /* Powertrain / Chassis / High-Voltage / Safety */
#define CAN_BUS_BODY       "vcan1"  /* Body / Comfort / Cockpit / Infotainment / OTA */

/* ========================================================================= */
/* Standard CAN Identifiers (11-bit standard IDs)                            */
/* ========================================================================= */

/* --- Bus 0: Powertrain, Chassis & Active Safety (vcan0) --- */
#define CAN_ID_PCM_TELEMETRY   0x100  /* Powertrain -> Central Compute */
#define CAN_ID_PCM_CMD         0x101  /* Central Compute -> Powertrain */
#define CAN_ID_BMS_TELEMETRY   0x110  /* BMS -> Central Compute        */
#define CAN_ID_BRAKE_TELEMETRY 0x120  /* ABS/Brake -> Central Compute  */
#define CAN_ID_BRAKE_CMD       0x121  /* Central Compute -> ABS/Brake  */
#define CAN_ID_STEER_TELEMETRY 0x130  /* EPS Steering -> Central Compute */
#define CAN_ID_STEER_CMD       0x131  /* Central Compute -> EPS Steering */
#define CAN_ID_ADAS_TELEMETRY  0x300  /* ADAS Radar/Vision -> Central Compute */
#define CAN_ID_ADAS_CMD        0x301  /* Central Compute -> ADAS Radar/Vision */

/* --- Bus 1: Body, Cockpit, Thermal & Telematics (vcan1) --- */
#define CAN_ID_BCM_TELEMETRY      0x200  /* BCM -> Central Compute */
#define CAN_ID_BCM_CMD            0x201  /* Central Compute -> BCM */
#define CAN_ID_HMI_INPUT          0x210  /* HMI Cockpit -> Central Compute */
#define CAN_ID_HVAC_TELEMETRY     0x220  /* HVAC -> Central Compute */
#define CAN_ID_HVAC_CMD           0x221  /* Central Compute -> HVAC */
#define CAN_ID_TELEMATICS_STATUS  0x400  /* Telematics TCU -> Central Compute */
#define CAN_ID_TELEMATICS_CMD     0x401  /* Central Compute -> Telematics TCU */

/* ========================================================================= */
/* Payload Structures (CAN 2.0: max 8 bytes, packed)                        */
/* ========================================================================= */

#pragma pack(push, 1)

/* 0x100: Powertrain Control Module (PCM) Telemetry */
typedef struct {
    uint16_t speed_kph_x10;  /* Speed in km/h * 10 (e.g. 1000 = 100.0 km/h) */
    uint16_t motor_rpm;      /* Motor RPM (0 - 15000) */
    int16_t  motor_torque_nm;/* Actual torque (-500 to +500 Nm) */
    int8_t   motor_temp_c;   /* Temperature in Celsius (-40 to 150) */
    uint8_t  gear_state;     /* 0: P, 1: R, 2: N, 3: D */
} PcmTelemetryMsg;

/* 0x101: Central Compute -> PCM Command */
typedef struct {
    int16_t  target_torque_nm; /* Desired motor torque (-500 to +500 Nm) */
    uint16_t speed_limit_kph;  /* Dynamic speed governor limit */
    uint8_t  inverter_enable;  /* 0: Disabled, 1: Enabled */
    uint8_t  regen_level;      /* 0: None, 1: Low, 2: Medium, 3: High */
    uint16_t reserved;
} PcmCmdMsg;

/* 0x110: Battery Management System (BMS) Telemetry */
typedef struct {
    uint8_t  soc_percent;      /* State of Charge (0 - 100%) */
    uint16_t pack_voltage_x10; /* Pack Voltage * 10 (e.g. 4000 = 400.0 V) */
    int16_t  pack_current_x10; /* Pack Current * 10 (e.g. 150 = 15.0 A discharge, -50 = 5.0 A regen) */
    int8_t   max_cell_temp_c;  /* Max cell temperature in Celsius */
    uint8_t  bms_status;       /* 0: OK, 1: Warning (Derate), 2: Fault (Shutdown) */
    uint8_t  soh_percent;      /* State of Health (0 - 100%) */
} BmsTelemetryMsg;

/* 0x120: ABS & Electronic Brake System Telemetry */
typedef struct {
    uint16_t actual_brake_torque_nm; /* Sum of 4 wheels friction torque */
    uint8_t  abs_active;             /* Bitmask: Bit 0: FL, Bit 1: FR, Bit 2: RL, Bit 3: RR (1 = pulsing) */
    uint8_t  brake_pressure_bar;     /* Master cylinder hydraulic pressure (0 - 150 bar) */
    int8_t   pad_temp_fl_c;          /* Front Left brake rotor temp */
    int8_t   pad_temp_fr_c;          /* Front Right brake rotor temp */
    uint8_t  esc_engaged;            /* 0: Off, 1: Active Stability Intervention */
    uint8_t  brake_status;           /* 0: OK, 1: Pad Wear / Thermal Warning, 2: Fault */
} BrakeTelemetryMsg;

/* 0x121: Central Compute -> Brake Command */
typedef struct {
    uint16_t req_brake_torque_nm;    /* Desired hydraulic brake torque (0 - 3000 Nm) */
    uint8_t  emergency_brake_en;     /* 0: Normal, 1: Full Emergency Brake */
    uint8_t  parking_brake_req;      /* 0: Released, 1: Engaged */
    uint32_t reserved;
} BrakeCmdMsg;

/* 0x130: Electric Power Steering (EPS) Telemetry */
typedef struct {
    int8_t   actual_steer_angle;     /* Pinion steering angle (-120 to +120 deg) */
    int16_t  motor_assist_nm_x10;    /* EPS motor assist torque (Nm * 10) */
    int16_t  driver_hand_torque_x10; /* Torsion bar hand torque (Nm * 10) */
    uint8_t  eps_status;             /* 0: OK, 1: Thermal Derate, 2: Fault */
    uint8_t  lka_active;             /* 0: Inactive, 1: Lane Keeping Assist Active */
    uint8_t  reserved;
} SteerTelemetryMsg;

/* 0x131: Central Compute -> EPS Command */
typedef struct {
    int8_t   driver_steer_angle;     /* Target driver wheel angle (-120 to +120 deg) */
    int8_t   lka_torque_overlay;     /* LKA corrective torque (-50 to +50 Nm * 10) */
    uint8_t  steering_mode;          /* 0: COMFORT, 1: STANDARD, 2: SPORT */
    uint8_t  reserved[5];
} SteerCmdMsg;

/* 0x300: ADAS Radar & Vision Telemetry */
typedef struct {
    uint16_t target_distance_m_x10;  /* Lead target distance in meters * 10 (e.g. 150 = 15.0m) */
    int16_t  relative_speed_kph_x10; /* Relative speed km/h * 10 (e.g. -200 = closing at 20km/h) */
    uint8_t  ttc_seconds_x10;        /* Time-to-Collision in seconds * 10 (255 = no danger) */
    uint8_t  fcw_alert;              /* Forward Collision Warning: 0: None, 1: Caution, 2: Imminent */
    uint8_t  aeb_request;            /* Autonomous Emergency Braking: 0: None, 1: Pre-charge, 2: Full Emergency */
    uint8_t  lane_departure_warning; /* 0: In-Lane, 1: Left Drift, 2: Right Drift */
} AdasTelemetryMsg;

/* 0x301: Central Compute -> ADAS Command */
typedef struct {
    uint8_t  adas_mode;              /* 0: Standby, 1: Active ACC/LKA, 2: Fault */
    uint8_t  emergency_brake_ack;    /* 1 if Central Compute acknowledged AEB request */
    uint8_t  lka_enable;             /* 1 to enable lane keeping assist torque overlay */
    uint8_t  reserved[5];
} AdasCmdMsg;

/* 0x200: Body Control Module (BCM) Telemetry */
typedef struct {
    uint8_t doors_locked;     /* Bit 0: FL, Bit 1: FR, Bit 2: RL, Bit 3: RR (1 = locked) */
    uint8_t lights_active;    /* Bit 0: LowBeam, Bit 1: HighBeam, Bit 2: Hazard, Bit 3: Brake */
    int8_t  cabin_temp_c;     /* Ambient cabin temperature in Celsius */
    uint8_t ambient_lux;      /* Ambient exterior light sensor (0-100) */
    uint8_t wipers_active;    /* 0: Off, 1: Intermittent, 2: High */
    uint8_t reserved[3];
} BcmTelemetryMsg;

/* 0x201: Central Compute -> BCM Command */
typedef struct {
    uint8_t lock_command;     /* 0: No change, 1: Lock All, 2: Unlock All */
    uint8_t light_command;    /* Bit 0: LowBeam, Bit 1: HighBeam, Bit 2: Hazard, Bit 3: Brake */
    uint8_t horn_active;      /* 0: Off, 1: Sound Horn */
    uint8_t reserved[5];
} BcmCmdMsg;

/* 0x210: Driver Cockpit / Human Machine Interface (HMI) */
typedef struct {
    uint8_t throttle_percent; /* Accelerator pedal position (0 - 100%) */
    uint8_t brake_percent;    /* Brake pedal position (0 - 100%) */
    int8_t  steering_angle;   /* Steering angle (-120 to +120 degrees) */
    uint8_t selected_gear;    /* 0: P, 1: R, 2: N, 3: D */
    uint8_t drive_mode;       /* 0: ECO, 1: COMFORT, 2: SPORT */
    uint8_t turn_signal;      /* 0: Off, 1: Left, 2: Right, 3: Hazard */
    uint16_t reserved;
} HmiInputMsg;

/* 0x220: HVAC & Thermal Management Telemetry */
typedef struct {
    int8_t   current_cabin_temp_c;  /* Interior cabin temperature sensor (°C) */
    int8_t   evaporator_temp_c;     /* AC evaporator core temp (°C) */
    int8_t   coolant_loop_temp_c;   /* Battery/inverter liquid coolant temp (°C) */
    uint16_t compressor_power_w;   /* Electrical power consumption of heat pump / compressor (W) */
    uint8_t  blower_rpm_x10;        /* Cabin blower fan RPM / 10 */
    uint8_t  hvac_status;           /* 0: OK, 1: High Thermal Load, 2: Fault */
    uint8_t  reserved;
} HvacTelemetryMsg;

/* 0x221: Central Compute -> HVAC Command */
typedef struct {
    int8_t   target_cabin_temp_c;   /* Desired cabin climate setpoint (°C) */
    uint8_t  fan_speed;             /* 0: Off, 1-7: Fan speed level */
    uint8_t  ac_compressor_enable;  /* 0: Off, 1: On */
    uint8_t  recirc_mode;           /* 0: Fresh Air, 1: Recirculate */
    uint8_t  battery_cooling_req;   /* 0: None, 1: Normal Chilling, 2: Max Thermal Chill */
    uint8_t  reserved[3];
} HvacCmdMsg;

/* 0x400: Telematics Control Unit (TCU) & OTA Status */
typedef struct {
    uint8_t  cellular_csq;      /* Cell signal strength (0 - 31 CSQ) */
    uint8_t  cloud_connected;   /* 0: Disconnected, 1: Authenticating, 2: Cloud Sync Active */
    uint8_t  ota_state;         /* 0: Idle, 1: Downloading, 2: Verifying, 3: Flashing, 4: Complete */
    uint8_t  ota_progress_pct;  /* OTA update percentage (0 - 100%) */
    uint8_t  remote_command;    /* 0: None, 1: Remote Unlock, 2: Remote Pre-heat, 3: Trigger OTA */
    uint8_t  gnss_fix;          /* 0: No Fix, 1: 2D Fix, 2: 3D RTK Fix */
    uint16_t cloud_latency_ms;  /* Round-trip ping to vehicle cloud backend (ms) */
} TelematicsStatusMsg;

/* 0x401: Central Compute -> Telematics Command */
typedef struct {
    uint8_t  ack_command;          /* Acknowledged remote command */
    uint8_t  ecu_firmware_ver;     /* Current firmware version (e.g. 24 = v2.4) */
    uint8_t  diagnostic_dtc_count; /* Active Diagnostic Trouble Codes */
    uint8_t  cloud_sync_rate_hz;   /* Requested cloud upload frequency */
    uint32_t reserved;
} TelematicsCmdMsg;

#pragma pack(pop)

/* Common Enums */
typedef enum {
    GEAR_PARK = 0,
    GEAR_REVERSE = 1,
    GEAR_NEUTRAL = 2,
    GEAR_DRIVE = 3
} VehicleGear;

typedef enum {
    MODE_ECO = 0,
    MODE_COMFORT = 1,
    MODE_SPORT = 2
} VehicleDriveMode;

typedef enum {
    BMS_STATUS_OK = 0,
    BMS_STATUS_WARNING = 1,
    BMS_STATUS_FAULT = 2
} BmsStatus;

typedef enum {
    AEB_NONE = 0,
    AEB_PRECHARGE = 1,
    AEB_FULL_EMERGENCY = 2
} AebState;

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING = 1,
    OTA_STATE_VERIFYING = 2,
    OTA_STATE_FLASHING = 3,
    OTA_STATE_COMPLETE = 4
} OtaState;

#endif /* VEHICLE_PROTOCOL_H */
