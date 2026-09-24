#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include "../common/can_common.h"
#include "../common/vehicle_protocol.h"

static volatile int g_running = 1;

static void sig_handler(int signum) {
    (void)signum;
    g_running = 0;
}

/* Centralized Vehicle State Store */
typedef struct {
    /* Powertrain */
    float speed_kph;
    uint16_t motor_rpm;
    int16_t actual_torque_nm;
    int8_t motor_temp_c;
    uint8_t pcm_gear;

    /* Battery */
    uint8_t battery_soc;
    float pack_voltage;
    float pack_current;
    int8_t cell_temp_c;
    uint8_t bms_status;

    /* Body */
    uint8_t doors_locked;
    uint8_t lights_active;
    int8_t cabin_temp_c;
    uint8_t ambient_lux;

    /* Cockpit / Driver */
    uint8_t throttle_percent;
    uint8_t brake_percent;
    int8_t steering_angle;
    uint8_t selected_gear;
    uint8_t drive_mode;

    /* Supervisory flags */
    uint8_t auto_lock_engaged;
    int16_t commanded_torque_nm;
} CentralVehicleState;

static void print_dashboard(const CentralVehicleState *st) {
    printf("\033[H\033[J"); /* Clear terminal screen */
    printf("===================================================================================\n");
    printf("          SOFTWARE-DEFINED VEHICLE (SDV) - CENTRAL VEHICLE COMPUTER (CVC)          \n");
    printf("===================================================================================\n");
    printf("  [Buses]: vcan0 (Powertrain/High-Voltage) | vcan1 (Body/Cockpit HMI)              \n");
    printf("-----------------------------------------------------------------------------------\n");

    /* Driver Input & Mode */
    const char *gear_str = (st->selected_gear == GEAR_PARK) ? "PARK" :
                           (st->selected_gear == GEAR_REVERSE) ? "REVERSE" :
                           (st->selected_gear == GEAR_NEUTRAL) ? "NEUTRAL" : "DRIVE";
    const char *mode_str = (st->drive_mode == MODE_ECO) ? "ECO" :
                           (st->drive_mode == MODE_COMFORT) ? "COMFORT" : "SPORT";

    printf("  [COCKPIT / HMI]\n");
    printf("    Gear: %-8s | Mode: %-8s | Throttle: %3u%% | Brake: %3u%% | Steer: %3d°\n",
           gear_str, mode_str, st->throttle_percent, st->brake_percent, st->steering_angle);
    printf("-----------------------------------------------------------------------------------\n");

    /* Powertrain Dynamics */
    printf("  [POWERTRAIN (vcan0)]\n");
    printf("    Speed: %5.1f km/h | Motor RPM: %5u | Act Torque: %+4d Nm | Cmd Torque: %+4d Nm\n",
           st->speed_kph, st->motor_rpm, st->actual_torque_nm, st->commanded_torque_nm);
    printf("    Motor Temp: %3d°C\n", st->motor_temp_c);
    printf("-----------------------------------------------------------------------------------\n");

    /* Battery / Energy */
    const char *bms_str = (st->bms_status == BMS_STATUS_OK) ? "NORMAL" :
                          (st->bms_status == BMS_STATUS_WARNING) ? "DERATED" : "FAULT";
    printf("  [BATTERY MANAGEMENT SYSTEM (vcan0)]\n");
    printf("    SoC: %3u%% | Voltage: %5.1f V | Current: %+5.1f A | Temp: %2d°C | Status: %s\n",
           st->battery_soc, st->pack_voltage, st->pack_current, st->cell_temp_c, bms_str);
    printf("-----------------------------------------------------------------------------------\n");

    /* Body & Comfort */
    int all_doors_locked = (st->doors_locked == 0x0F);
    int brake_light = (st->lights_active & 0x08) ? 1 : 0;
    int low_beam = (st->lights_active & 0x01) ? 1 : 0;
    printf("  [BODY CONTROL MODULE (vcan1)]\n");
    printf("    Doors: %-8s | Brake Lights: %-3s | Low Beam: %-3s | Ambient: %2u lux | Cabin: %2d°C\n",
           all_doors_locked ? "LOCKED" : "UNLOCKED",
           brake_light ? "ON" : "OFF",
           low_beam ? "ON" : "OFF",
           st->ambient_lux, st->cabin_temp_c);
    printf("-----------------------------------------------------------------------------------\n");
    printf("  [CENTRAL SUPERVISORY ACTIONS]\n");
    if (st->speed_kph > 15.0f && st->auto_lock_engaged) {
        printf("   >> Auto-Speed Door Lock: ENGAGED (>15 km/h)\n");
    }
    if (st->brake_percent > 5) {
        printf("   >> Regenerative / Friction Braking: ACTIVE (Regen Torque: %d Nm)\n", st->commanded_torque_nm);
    }
    if (st->bms_status == BMS_STATUS_WARNING) {
        printf("   >> Torque Derating Active: Thermal or Battery preservation active\n");
    }
    printf("===================================================================================\n");
    printf("  Press Ctrl+C to stop simulation.\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    const char *if_powertrain = (argc > 1) ? argv[1] : CAN_BUS_POWERTRAIN;
    const char *if_body       = (argc > 2) ? argv[2] : CAN_BUS_BODY;

    printf("[Central Compute] Initializing SDV Central Controller on %s and %s (PID: %d)...\n",
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
    state.cabin_temp_c = 22;

    uint64_t last_ctrl_loop_ms = 0;
    uint64_t last_dash_ms = 0;

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
            /* 1. Process Powertrain Bus Frames (vcan0) */
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
                    }
                }
            }

            /* 2. Process Body & Cockpit Bus Frames (vcan1) */
            if (FD_ISSET(sock_body, &read_fds)) {
                struct can_frame frame;
                if (can_recv_msg(sock_body, &frame) > 0) {
                    if (frame.can_id == CAN_ID_HMI_INPUT && frame.can_dlc >= sizeof(HmiInputMsg)) {
                        HmiInputMsg *hmi = (HmiInputMsg *)frame.data;
                        state.throttle_percent = hmi->throttle_percent;
                        state.brake_percent = hmi->brake_percent;
                        state.steering_angle = hmi->steering_angle;
                        state.selected_gear = hmi->selected_gear;
                        state.drive_mode = hmi->drive_mode;
                    } else if (frame.can_id == CAN_ID_BCM_TELEMETRY && frame.can_dlc >= sizeof(BcmTelemetryMsg)) {
                        BcmTelemetryMsg *bcm = (BcmTelemetryMsg *)frame.data;
                        state.doors_locked = bcm->doors_locked;
                        state.lights_active = bcm->lights_active;
                        state.cabin_temp_c = bcm->cabin_temp_c;
                        state.ambient_lux = bcm->ambient_lux;
                    }
                }
            }
        }

        uint64_t now_ms = get_timestamp_ms();

        /* Central Arbitration & Control Loop (Every 20ms = 50Hz) */
        if (now_ms - last_ctrl_loop_ms >= 20) {
            last_ctrl_loop_ms = now_ms;

            /* A. Powertrain Arbitration Logic */
            int16_t requested_torque = 0;
            if (state.selected_gear == GEAR_DRIVE) {
                float max_mode_torque = 280.0f; /* Comfort default */
                if (state.drive_mode == MODE_ECO) max_mode_torque = 180.0f;
                else if (state.drive_mode == MODE_SPORT) max_mode_torque = 420.0f;

                requested_torque = (int16_t)((state.throttle_percent / 100.0f) * max_mode_torque);

                /* If brake is applied, prioritize braking (Regenerative braking) */
                if (state.brake_percent > 0) {
                    int16_t regen_torque = (int16_t)(-(state.brake_percent / 100.0f) * 160.0f);
                    requested_torque = regen_torque;
                }

                /* BMS derating protection */
                if (state.bms_status == BMS_STATUS_WARNING) {
                    if (requested_torque > 0) requested_torque /= 2;
                } else if (state.bms_status == BMS_STATUS_FAULT) {
                    requested_torque = 0;
                }
            } else if (state.selected_gear == GEAR_REVERSE) {
                requested_torque = (int16_t)(-((state.throttle_percent / 100.0f) * 80.0f));
            } else {
                /* Park or Neutral */
                requested_torque = 0;
            }

            state.commanded_torque_nm = requested_torque;

            /* Send command to PCM on vcan0 */
            PcmCmdMsg pcm_cmd;
            memset(&pcm_cmd, 0, sizeof(pcm_cmd));
            pcm_cmd.target_torque_nm = requested_torque;
            pcm_cmd.speed_limit_kph = 200;
            pcm_cmd.inverter_enable = (state.selected_gear == GEAR_PARK) ? 0 : 1;
            pcm_cmd.regen_level = 1;
            can_send_msg(sock_pt, CAN_ID_PCM_CMD, &pcm_cmd, sizeof(pcm_cmd));

            /* B. Body Supervisory Logic (Auto-lock, Auto-lights) */
            BcmCmdMsg bcm_cmd;
            memset(&bcm_cmd, 0, sizeof(bcm_cmd));

            /* Auto-lock above 15 km/h */
            if (state.speed_kph > 15.0f && state.doors_locked != 0x0F) {
                bcm_cmd.lock_command = 1; /* Lock All */
                state.auto_lock_engaged = 1;
            }

            /* Light command arbitration:
               Bit 0: LowBeam (auto on if dark or manually on)
               Bit 3: Brake Light (on if brake pedal > 5%)
            */
            uint8_t lights = 0;
            if (state.ambient_lux < 30) {
                lights |= 0x01; /* Auto LowBeam in dark */
            }
            if (state.brake_percent > 5) {
                lights |= 0x08; /* Brake light active */
            }
            bcm_cmd.light_command = lights;

            can_send_msg(sock_body, CAN_ID_BCM_CMD, &bcm_cmd, sizeof(bcm_cmd));
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
