#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <math.h>
#include "../common/can_common.h"
#include "../common/vehicle_protocol.h"

static volatile int g_running = 1;

static void sig_handler(int signum) {
    (void)signum;
    g_running = 0;
}

/* Centralized 10-Node Vehicle State Store */
typedef struct {
    /* 1. Powertrain (PCM) */
    float speed_kph;
    uint16_t motor_rpm;
    int16_t actual_torque_nm;
    int8_t motor_temp_c;
    uint8_t pcm_gear;

    /* 2. Battery (BMS) */
    uint8_t battery_soc;
    float pack_voltage;
    float pack_current;
    int8_t cell_temp_c;
    uint8_t bms_status;

    /* 3. ABS / Braking */
    uint16_t actual_brake_torque_nm;
    uint8_t abs_active;
    uint8_t brake_pressure_bar;
    int8_t pad_temp_fl_c;
    int8_t pad_temp_fr_c;
    uint8_t esc_engaged;

    /* 4. EPS Steering */
    int8_t actual_steer_angle;
    float motor_assist_nm;
    float driver_hand_torque_nm;
    uint8_t lka_active;

    /* 5. ADAS Radar / Vision */
    float lead_distance_m;
    float relative_speed_kph;
    float ttc_seconds;
    uint8_t fcw_alert;
    uint8_t aeb_request;
    uint8_t lane_departure;

    /* 6. Body Control (BCM) */
    uint8_t doors_locked;
    uint8_t lights_active;
    int8_t bcm_cabin_temp_c;
    uint8_t ambient_lux;

    /* 7. Cockpit / Driver (HMI) */
    uint8_t throttle_percent;
    uint8_t brake_percent;
    int8_t driver_steer_angle;
    uint8_t selected_gear;
    uint8_t drive_mode;
    uint8_t turn_signal;

    /* 8. HVAC & Thermal */
    int8_t current_cabin_temp_c;
    int8_t evaporator_temp_c;
    int8_t coolant_loop_temp_c;
    uint16_t compressor_power_w;
    uint8_t blower_rpm_x10;
    uint8_t battery_cooling_active;

    /* 9. Telematics & OTA */
    uint8_t cellular_csq;
    uint8_t cloud_connected;
    uint8_t ota_state;
    uint8_t ota_progress_pct;
    uint8_t gnss_fix;
    uint16_t cloud_latency_ms;

    /* 10. Central Supervisory Arbitration Output */
    int16_t commanded_motor_torque_nm;
    uint16_t commanded_friction_brake_nm;
    int8_t commanded_lka_overlay_nm_x10;
    uint8_t auto_lock_engaged;
    uint8_t aeb_active;
} CentralVehicleState;

static void print_dashboard(const CentralVehicleState *st) {
    printf("\033[H\033[J"); /* Clear terminal screen */
    printf("========================================================================================\n");
    printf("           SOFTWARE-DEFINED VEHICLE (SDV) - CENTRAL VEHICLE COMPUTER (CVC)              \n");
    printf("                     10-NODE FULL VEHICLE TOPOLOGY DASHBOARD                            \n");
    printf("========================================================================================\n");
    printf("  [Networks]: vcan0 (Powertrain/Chassis/Safety)  |  vcan1 (Body/Cockpit/HVAC/Telematics)\n");
    printf("----------------------------------------------------------------------------------------\n");

    /* Zone 1: Cockpit / Driver Input */
    const char *gear_str = (st->selected_gear == GEAR_PARK) ? "PARK" :
                           (st->selected_gear == GEAR_REVERSE) ? "REVERSE" :
                           (st->selected_gear == GEAR_NEUTRAL) ? "NEUTRAL" : "DRIVE";
    const char *mode_str = (st->drive_mode == MODE_ECO) ? "ECO" :
                           (st->drive_mode == MODE_COMFORT) ? "COMFORT" : "SPORT";

    printf(" [ZONE 1: COCKPIT & DRIVER HMI (vcan1)]\n");
    printf("    Gear: %-7s | Mode: %-7s | Throttle: %3u%% | Brake: %3u%% | Steering: %+3d°\n",
           gear_str, mode_str, st->throttle_percent, st->brake_percent, st->driver_steer_angle);
    printf("----------------------------------------------------------------------------------------\n");

    /* Zone 2: Powertrain & High-Voltage Battery */
    const char *bms_str = (st->bms_status == BMS_STATUS_OK) ? "NORMAL" :
                          (st->bms_status == BMS_STATUS_WARNING) ? "DERATED" : "FAULT";
    printf(" [ZONE 2: POWERTRAIN & TRACTION BATTERY (vcan0)]\n");
    printf("    Speed: %5.1f km/h | Motor RPM: %5u | Act Torque: %+4d Nm | Cmd: %+4d Nm\n",
           st->speed_kph, st->motor_rpm, st->actual_torque_nm, st->commanded_motor_torque_nm);
    printf("    Battery SoC: %3u%% | Voltage: %5.1f V | Current: %+5.1f A | Cell Temp: %2d°C (%s)\n",
           st->battery_soc, st->pack_voltage, st->pack_current, st->cell_temp_c, bms_str);
    printf("----------------------------------------------------------------------------------------\n");

    /* Zone 3: Active Safety, ADAS, Braking & Steering */
    const char *fcw_str = (st->fcw_alert == 0) ? "CLEAR" :
                          (st->fcw_alert == 1) ? "ADVISORY" : "IMMINENT WARNING";
    const char *aeb_str = (st->aeb_request == AEB_NONE) ? "INACTIVE" :
                          (st->aeb_request == AEB_PRECHARGE) ? "PRE-CHARGE" : "FULL EMERGENCY";
    const char *abs_str = (st->abs_active) ? "PULSING (ACTIVE)" : "STANDBY";
    const char *ldw_str = (st->lane_departure == 1) ? "DRIFT LEFT" :
                          (st->lane_departure == 2) ? "DRIFT RIGHT" : "IN LANE";

    printf(" [ZONE 3: ACTIVE SAFETY, ADAS, BRAKES & EPS (vcan0)]\n");
    printf("    ADAS Radar: Target Dist: %5.1f m | Rel Spd: %+5.1f km/h | TTC: %4.1f s | FCW: %s\n",
           st->lead_distance_m, st->relative_speed_kph, st->ttc_seconds, fcw_str);
    printf("    Safety Action: AEB: %-14s | Lane Status: %-11s | LKA Torque: %+3.1f Nm\n",
           aeb_str, ldw_str, (float)st->commanded_lka_overlay_nm_x10 / 10.0f);
    printf("    Braking (ABS): Hydr Press: %3u bar | Torque: %4u Nm | ABS: %-15s | Rotor: %2d°C\n",
           st->brake_pressure_bar, st->actual_brake_torque_nm, abs_str, st->pad_temp_fl_c);
    printf("    Steering (EPS): Pinion Angle: %+3d° | Assist Torque: %+4.1f Nm | Hand Torque: %+4.1f Nm\n",
           st->actual_steer_angle, st->motor_assist_nm, st->driver_hand_torque_nm);
    printf("----------------------------------------------------------------------------------------\n");

    /* Zone 4: Body, Cabin & Thermal Loop */
    int all_doors_locked = (st->doors_locked == 0x0F);
    int brake_light = (st->lights_active & 0x08) ? 1 : 0;
    int low_beam = (st->lights_active & 0x01) ? 1 : 0;
    const char *chill_str = (st->battery_cooling_active == 2) ? "MAX CHILL" :
                            (st->battery_cooling_active == 1) ? "ACTIVE" : "OFF";

    printf(" [ZONE 4: BODY, CABIN & THERMAL MANAGEMENT (vcan1)]\n");
    printf("    Doors: %-8s | Brake Light: %-3s | Low Beam: %-3s | Ambient: %2u lux\n",
           all_doors_locked ? "LOCKED" : "UNLOCKED",
           brake_light ? "ON" : "OFF",
           low_beam ? "ON" : "OFF",
           st->ambient_lux);
    printf("    HVAC / Thermal: Cabin: %2d°C | Evap: %2d°C | Coolant Loop: %2d°C | Battery Chill: %s\n",
           st->current_cabin_temp_c, st->evaporator_temp_c, st->coolant_loop_temp_c, chill_str);
    printf("    Heat Pump Load: %4u W | Blower: %4u RPM\n",
           st->compressor_power_w, (uint16_t)st->blower_rpm_x10 * 10);
    printf("----------------------------------------------------------------------------------------\n");

    /* Zone 5: Telematics, 5G Cloud & OTA */
    const char *ota_str = (st->ota_state == OTA_STATE_IDLE) ? "IDLE (v2.4.0)" :
                          (st->ota_state == OTA_STATE_DOWNLOADING) ? "DOWNLOADING PKG" :
                          (st->ota_state == OTA_STATE_VERIFYING) ? "VERIFYING SIGNATURE" :
                          (st->ota_state == OTA_STATE_FLASHING) ? "FLASHING A/B PARTITION" : "UPDATE COMPLETE (v2.4.1)";

    printf(" [ZONE 5: TELEMATICS & OTA CLOUD GATEWAY (vcan1)]\n");
    printf("    Cellular: 5G Signal: %2u/31 CSQ | GNSS: RTK 3D Fix | Cloud Latency: %2u ms\n",
           st->cellular_csq, st->cloud_latency_ms);
    printf("    OTA Status: %-26s | Progress: [%3u%%]\n", ota_str, st->ota_progress_pct);
    printf("----------------------------------------------------------------------------------------\n");

    /* Zone 6: Central Supervisory Arbitration Engine */
    printf(" [CENTRAL VEHICLE ARBITRATION ACTIONS]\n");
    if (st->aeb_active) {
        printf("   [!CRITICAL DANGER!] >> AEB EMERGENCY BRAKING ACTIVE: Full Friction + Regen Clamped!\n");
    } else if (st->brake_percent > 0) {
        printf("   >> Brake Blending: Regen Torque %d Nm + Hydraulic Friction %u Nm\n",
               st->commanded_motor_torque_nm, st->commanded_friction_brake_nm);
    }
    if (st->speed_kph > 15.0f && st->auto_lock_engaged) {
        printf("   >> Auto Door Lock: ENGAGED (>15 km/h safety threshold)\n");
    }
    if (st->lka_active) {
        printf("   >> Lane Keeping Assist (LKA): Corrective Steering Torque Overlay Active\n");
    }
    if (st->battery_cooling_active) {
        printf("   >> Thermal Supervisor: Battery liquid coolant plate chilling enabled\n");
    }
    printf("========================================================================================\n");
    printf("  Press Ctrl+C to cleanly terminate all 10 simulation nodes.\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    const char *if_powertrain = (argc > 1) ? argv[1] : CAN_BUS_POWERTRAIN;
    const char *if_body       = (argc > 2) ? argv[2] : CAN_BUS_BODY;

    printf("[Central Compute] Initializing 10-Node SDV Central Controller on %s & %s (PID: %d)...\n",
           if_powertrain, if_body, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock_pt = open_can_socket(if_powertrain);
    if (sock_pt < 0) {
        fprintf(stderr, "Failed to open %s\n", if_powertrain);
        return 1;
    }

    int sock_body = open_can_socket(if_body);
    if (sock_body < 0) {
        fprintf(stderr, "Failed to open %s\n", if_body);
        close(sock_pt);
        return 1;
    }

    CentralVehicleState state;
    memset(&state, 0, sizeof(state));
    state.battery_soc = 88;
    state.pack_voltage = 400.0f;
    state.motor_temp_c = 25;
    state.cell_temp_c = 25;
    state.bcm_cabin_temp_c = 22;
    state.current_cabin_temp_c = 22;
    state.evaporator_temp_c = 5;
    state.coolant_loop_temp_c = 25;
    state.cellular_csq = 28;
    state.cloud_connected = 2;
    state.cloud_latency_ms = 35;
    state.lead_distance_m = 65.0f;
    state.ttc_seconds = 25.0f;

    uint64_t last_ctrl_loop_ms = 0;
    uint64_t last_dash_ms = 0;
    uint64_t last_tcu_cmd_ms = 0;

    int max_fd = (sock_pt > sock_body) ? sock_pt : sock_body;

    while (g_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_pt, &read_fds);
        FD_SET(sock_body, &read_fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000; /* 10ms select timeout */

        int ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ret > 0) {
            /* 1. Process Powertrain & Chassis Bus (vcan0) */
            if (FD_ISSET(sock_pt, &read_fds)) {
                struct can_frame frame;
                if (can_recv_msg(sock_pt, &frame) > 0) {
                    if (frame.can_id == CAN_ID_PCM_TELEMETRY && frame.can_dlc >= sizeof(PcmTelemetryMsg)) {
                        PcmTelemetryMsg *pcm = (PcmTelemetryMsg *)frame.data;
                        state.speed_kph = (float)pcm->speed_kph_x10 / 10.0f;
                        state.motor_rpm = pcm->motor_rpm;
                        state.actual_torque_nm = pcm->motor_torque_nm;
                        state.motor_temp_c = pcm->motor_temp_c;
                        state.pcm_gear = pcm->gear_state;
                    } else if (frame.can_id == CAN_ID_BMS_TELEMETRY && frame.can_dlc >= sizeof(BmsTelemetryMsg)) {
                        BmsTelemetryMsg *bms = (BmsTelemetryMsg *)frame.data;
                        state.battery_soc = bms->soc_percent;
                        state.pack_voltage = (float)bms->pack_voltage_x10 / 10.0f;
                        state.pack_current = (float)bms->pack_current_x10 / 10.0f;
                        state.cell_temp_c = bms->max_cell_temp_c;
                        state.bms_status = bms->bms_status;
                    } else if (frame.can_id == CAN_ID_BRAKE_TELEMETRY && frame.can_dlc >= sizeof(BrakeTelemetryMsg)) {
                        BrakeTelemetryMsg *brk = (BrakeTelemetryMsg *)frame.data;
                        state.actual_brake_torque_nm = brk->actual_brake_torque_nm;
                        state.abs_active = brk->abs_active;
                        state.brake_pressure_bar = brk->brake_pressure_bar;
                        state.pad_temp_fl_c = brk->pad_temp_fl_c;
                        state.pad_temp_fr_c = brk->pad_temp_fr_c;
                        state.esc_engaged = brk->esc_engaged;
                    } else if (frame.can_id == CAN_ID_STEER_TELEMETRY && frame.can_dlc >= sizeof(SteerTelemetryMsg)) {
                        SteerTelemetryMsg *str = (SteerTelemetryMsg *)frame.data;
                        state.actual_steer_angle = str->actual_steer_angle;
                        state.motor_assist_nm = (float)str->motor_assist_nm_x10 / 10.0f;
                        state.driver_hand_torque_nm = (float)str->driver_hand_torque_x10 / 10.0f;
                        state.lka_active = str->lka_active;
                    } else if (frame.can_id == CAN_ID_ADAS_TELEMETRY && frame.can_dlc >= sizeof(AdasTelemetryMsg)) {
                        AdasTelemetryMsg *adas = (AdasTelemetryMsg *)frame.data;
                        state.lead_distance_m = (float)adas->target_distance_m_x10 / 10.0f;
                        state.relative_speed_kph = (float)adas->relative_speed_kph_x10 / 10.0f;
                        state.ttc_seconds = (adas->ttc_seconds_x10 == 255) ? 25.0f : ((float)adas->ttc_seconds_x10 / 10.0f);
                        state.fcw_alert = adas->fcw_alert;
                        state.aeb_request = adas->aeb_request;
                        state.lane_departure = adas->lane_departure_warning;
                    }
                }
            }

            /* 2. Process Body & Cockpit Bus (vcan1) */
            if (FD_ISSET(sock_body, &read_fds)) {
                struct can_frame frame;
                if (can_recv_msg(sock_body, &frame) > 0) {
                    if (frame.can_id == CAN_ID_HMI_INPUT && frame.can_dlc >= sizeof(HmiInputMsg)) {
                        HmiInputMsg *hmi = (HmiInputMsg *)frame.data;
                        state.throttle_percent = hmi->throttle_percent;
                        state.brake_percent = hmi->brake_percent;
                        state.driver_steer_angle = hmi->steering_angle;
                        state.selected_gear = hmi->selected_gear;
                        state.drive_mode = hmi->drive_mode;
                        state.turn_signal = hmi->turn_signal;
                    } else if (frame.can_id == CAN_ID_BCM_TELEMETRY && frame.can_dlc >= sizeof(BcmTelemetryMsg)) {
                        BcmTelemetryMsg *bcm = (BcmTelemetryMsg *)frame.data;
                        state.doors_locked = bcm->doors_locked;
                        state.lights_active = bcm->lights_active;
                        state.bcm_cabin_temp_c = bcm->cabin_temp_c;
                        state.ambient_lux = bcm->ambient_lux;
                    } else if (frame.can_id == CAN_ID_HVAC_TELEMETRY && frame.can_dlc >= sizeof(HvacTelemetryMsg)) {
                        HvacTelemetryMsg *hvac = (HvacTelemetryMsg *)frame.data;
                        state.current_cabin_temp_c = hvac->current_cabin_temp_c;
                        state.evaporator_temp_c = hvac->evaporator_temp_c;
                        state.coolant_loop_temp_c = hvac->coolant_loop_temp_c;
                        state.compressor_power_w = hvac->compressor_power_w;
                        state.blower_rpm_x10 = hvac->blower_rpm_x10;
                    } else if (frame.can_id == CAN_ID_TELEMATICS_STATUS && frame.can_dlc >= sizeof(TelematicsStatusMsg)) {
                        TelematicsStatusMsg *tcu = (TelematicsStatusMsg *)frame.data;
                        state.cellular_csq = tcu->cellular_csq;
                        state.cloud_connected = tcu->cloud_connected;
                        state.ota_state = tcu->ota_state;
                        state.ota_progress_pct = tcu->ota_progress_pct;
                        state.gnss_fix = tcu->gnss_fix;
                        state.cloud_latency_ms = tcu->cloud_latency_ms;
                    }
                }
            }
        }

        uint64_t now_ms = get_timestamp_ms();

        /* Central Arbitration & Control Loop (Every 20ms = 50Hz) */
        if (now_ms - last_ctrl_loop_ms >= 20) {
            last_ctrl_loop_ms = now_ms;

            /* ============================================================= */
            /* A. Active Safety & Autonomous Emergency Braking (AEB)         */
            /* ============================================================= */
            int16_t requested_motor_torque = 0;
            uint16_t requested_friction_brake = 0;
            uint8_t aeb_in_progress = 0;

            if (state.aeb_request == AEB_FULL_EMERGENCY) {
                /* Imminent collision! Override driver inputs completely */
                aeb_in_progress = 1;
                requested_motor_torque = -180;        /* Max motor regen deceleration */
                requested_friction_brake = 2400;      /* Full hydraulic emergency pressure */
            } else if (state.aeb_request == AEB_PRECHARGE) {
                /* Pre-charge hydraulic lines for instant bite */
                requested_friction_brake = 350;
            } else {
                /* Normal Driver Pedal Arbitration */
                if (state.selected_gear == GEAR_DRIVE) {
                    float max_mode_torque = 280.0f; /* Comfort */
                    if (state.drive_mode == MODE_ECO) max_mode_torque = 180.0f;
                    else if (state.drive_mode == MODE_SPORT) max_mode_torque = 420.0f;

                    requested_motor_torque = (int16_t)((state.throttle_percent / 100.0f) * max_mode_torque);

                    /* Brake Blending: Distribute braking between motor regen and hydraulic friction */
                    if (state.brake_percent > 0) {
                        /* 1. Electric Motor Regen (up to -160 Nm) */
                        requested_motor_torque = (int16_t)(-(state.brake_percent / 100.0f) * 160.0f);

                        /* 2. Hydraulic Friction Brake (blended smoothly above 10% pedal) */
                        if (state.brake_percent > 10) {
                            requested_friction_brake = (uint16_t)(((state.brake_percent - 10) / 90.0f) * 2200.0f);
                        }
                    }

                    /* BMS derating protection */
                    if (state.bms_status == BMS_STATUS_WARNING) {
                        if (requested_motor_torque > 0) requested_motor_torque /= 2;
                    } else if (state.bms_status == BMS_STATUS_FAULT) {
                        requested_motor_torque = 0;
                    }
                } else if (state.selected_gear == GEAR_REVERSE) {
                    requested_motor_torque = (int16_t)(-((state.throttle_percent / 100.0f) * 80.0f));
                    if (state.brake_percent > 0) {
                        requested_friction_brake = (uint16_t)((state.brake_percent / 100.0f) * 1200.0f);
                    }
                } else {
                    /* Park or Neutral */
                    requested_motor_torque = 0;
                    if (state.selected_gear == GEAR_PARK) {
                        requested_friction_brake = 800; /* Parking hold */
                    }
                }
            }

            state.commanded_motor_torque_nm = requested_motor_torque;
            state.commanded_friction_brake_nm = requested_friction_brake;
            state.aeb_active = aeb_in_progress;

            /* Send PCM Command (vcan0) */
            PcmCmdMsg pcm_cmd;
            memset(&pcm_cmd, 0, sizeof(pcm_cmd));
            pcm_cmd.target_torque_nm = requested_motor_torque;
            pcm_cmd.speed_limit_kph = 200;
            pcm_cmd.inverter_enable = (state.selected_gear == GEAR_PARK) ? 0 : 1;
            pcm_cmd.regen_level = 1;
            can_send_msg(sock_pt, CAN_ID_PCM_CMD, &pcm_cmd, sizeof(pcm_cmd));

            /* Send Brake Command (vcan0) */
            BrakeCmdMsg brk_cmd;
            memset(&brk_cmd, 0, sizeof(brk_cmd));
            brk_cmd.req_brake_torque_nm = requested_friction_brake;
            brk_cmd.emergency_brake_en = aeb_in_progress;
            brk_cmd.parking_brake_req = (state.selected_gear == GEAR_PARK) ? 1 : 0;
            can_send_msg(sock_pt, CAN_ID_BRAKE_CMD, &brk_cmd, sizeof(brk_cmd));

            /* ============================================================= */
            /* B. Steering & Lane Keeping Assist (LKA) Arbitration           */
            /* ============================================================= */
            int8_t lka_overlay = 0;
            if (state.lane_departure == 1 && state.turn_signal == 0) {
                lka_overlay = 18;  /* Drift left -> steer right */
            } else if (state.lane_departure == 2 && state.turn_signal == 0) {
                lka_overlay = -18; /* Drift right -> steer left */
            }
            state.commanded_lka_overlay_nm_x10 = lka_overlay;

            SteerCmdMsg steer_cmd;
            memset(&steer_cmd, 0, sizeof(steer_cmd));
            steer_cmd.driver_steer_angle = state.driver_steer_angle;
            steer_cmd.lka_torque_overlay = lka_overlay;
            steer_cmd.steering_mode = state.drive_mode;
            can_send_msg(sock_pt, CAN_ID_STEER_CMD, &steer_cmd, sizeof(steer_cmd));

            /* ============================================================= */
            /* C. Body Supervisory Logic (Auto-lock, Auto-lights, Hazards)   */
            /* ============================================================= */
            BcmCmdMsg bcm_cmd;
            memset(&bcm_cmd, 0, sizeof(bcm_cmd));

            /* Auto-lock doors above 15 km/h */
            if (state.speed_kph > 15.0f && state.doors_locked != 0x0F) {
                bcm_cmd.lock_command = 1; /* Lock All */
                state.auto_lock_engaged = 1;
            }

            /* Lighting arbitration:
             * Bit 0: LowBeam (auto on if dark)
             * Bit 2: Hazard (flash on AEB emergency)
             * Bit 3: Brake Light (on if brake pedal > 5% or AEB active)
             */
            uint8_t lights = 0;
            if (state.ambient_lux < 30) {
                lights |= 0x01;
            }
            if (state.brake_percent > 5 || aeb_in_progress) {
                lights |= 0x08;
            }
            if (aeb_in_progress) {
                lights |= 0x04; /* Hazard lights flash during emergency stop */
                bcm_cmd.horn_active = 1;
            }
            bcm_cmd.light_command = lights;
            can_send_msg(sock_body, CAN_ID_BCM_CMD, &bcm_cmd, sizeof(bcm_cmd));

            /* ============================================================= */
            /* D. HVAC & Thermal Supervisory Control                         */
            /* ============================================================= */
            HvacCmdMsg hvac_cmd;
            memset(&hvac_cmd, 0, sizeof(hvac_cmd));
            hvac_cmd.target_cabin_temp_c = 21; /* Desired cabin setpoint 21°C */
            hvac_cmd.fan_speed = 3;
            hvac_cmd.ac_compressor_enable = 1;

            /* Battery Thermal Management:
             * If battery cell temp > 35°C, trigger active battery coolant chilling
             */
            if (state.cell_temp_c > 45 || state.motor_temp_c > 75) {
                hvac_cmd.battery_cooling_req = 2; /* Max chill */
                state.battery_cooling_active = 2;
            } else if (state.cell_temp_c > 32) {
                hvac_cmd.battery_cooling_req = 1; /* Normal chilling */
                state.battery_cooling_active = 1;
            } else {
                hvac_cmd.battery_cooling_req = 0;
                state.battery_cooling_active = 0;
            }
            can_send_msg(sock_body, CAN_ID_HVAC_CMD, &hvac_cmd, sizeof(hvac_cmd));
        }

        /* Forward Telematics Status / Commands (every 500ms) */
        if (now_ms - last_tcu_cmd_ms >= 500) {
            last_tcu_cmd_ms = now_ms;

            TelematicsCmdMsg tcu_cmd;
            memset(&tcu_cmd, 0, sizeof(tcu_cmd));
            tcu_cmd.ack_command = 1;
            tcu_cmd.ecu_firmware_ver = 24; /* v2.4 */
            tcu_cmd.diagnostic_dtc_count = (state.bms_status == BMS_STATUS_FAULT) ? 1 : 0;
            tcu_cmd.cloud_sync_rate_hz = 2;
            can_send_msg(sock_body, CAN_ID_TELEMATICS_CMD, &tcu_cmd, sizeof(tcu_cmd));
        }

        /* Update Dashboard (every 250ms) */
        if (now_ms - last_dash_ms >= 250) {
            last_dash_ms = now_ms;
            print_dashboard(&state);
        }
    }

    close(sock_pt);
    close(sock_body);
    printf("\n[Central Compute] Stopped cleanly.\n");
    return 0;
}
