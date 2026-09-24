#ifndef VEHICLE_PROTOCOL_H
#define VEHICLE_PROTOCOL_H

#include <stdint.h>

/* ========================================================================= */
/* Virtual CAN Network Topology                                              */
/* ========================================================================= */
#define CAN_BUS_POWERTRAIN "vcan0"  /* Powertrain / Chassis / High-Voltage bus */
#define CAN_BUS_BODY       "vcan1"  /* Body / Comfort / Cabin / HMI bus        */

/* ========================================================================= */
/* Standard CAN Identifiers (11-bit standard IDs)                            */
/* ========================================================================= */

/* --- Bus 0: Powertrain & Battery --- */
#define CAN_ID_PCM_TELEMETRY  0x100  /* Powertrain -> Central Compute */
#define CAN_ID_PCM_CMD        0x101  /* Central Compute -> Powertrain */
#define CAN_ID_BMS_TELEMETRY  0x110  /* BMS -> Central Compute        */

/* --- Bus 1: Body & Cockpit (HMI) --- */
#define CAN_ID_BCM_TELEMETRY  0x200  /* BCM -> Central Compute        */
#define CAN_ID_BCM_CMD        0x201  /* Central Compute -> BCM        */
#define CAN_ID_HMI_INPUT      0x210  /* HMI Cockpit -> Central Compute*/

/* --- Future Expansion CAN IDs --- */
#define CAN_ID_BRAKE_TELEMETRY 0x120 /* ABS/Brake Node */
#define CAN_ID_STEER_TELEMETRY 0x130 /* Electric Power Steering (EPS) */
#define CAN_ID_ADAS_TELEMETRY  0x300 /* Radar/Camera ADAS Node */
#define CAN_ID_TELEMATICS_OTA  0x400 /* Telematics / Cloud Gateway */

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

#endif /* VEHICLE_PROTOCOL_H */
