#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include "../common/can_common.h"
#include "../common/vehicle_protocol.h"

static volatile int g_running = 1;

static void sig_handler(int signum) {
    (void)signum;
    g_running = 0;
}

int main(int argc, char *argv[]) {
    const char *ifname = (argc > 1) ? argv[1] : CAN_BUS_POWERTRAIN;
    printf("[Brake Node] Starting ABS / Electronic Stability Control on %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[Brake Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    set_socket_timeout(sock, 10);

    /* Brake System State */
    uint16_t req_brake_torque = 0;
    uint8_t  emergency_brake = 0;
    uint8_t  parking_brake = 0;
    float    veh_speed_mps = 0.0f;

    /* Dynamic internal state */
    float hydraulic_pressure_bar = 0.0f;
    float actual_friction_torque = 0.0f;
    float pad_temp_fl = 30.0f;
    float pad_temp_fr = 30.0f;
    uint8_t abs_active_mask = 0;
    uint8_t esc_active = 0;

    uint64_t last_tx_ms = 0;
    uint64_t last_sim_ms = get_timestamp_ms();

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();

        /* 1. Receive Brake Commands and PCM Telemetry */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0) {
            if (rx_frame.can_id == CAN_ID_BRAKE_CMD && rx_frame.can_dlc >= sizeof(BrakeCmdMsg)) {
                BrakeCmdMsg *cmd = (BrakeCmdMsg *)rx_frame.data;
                req_brake_torque = cmd->req_brake_torque_nm;
                emergency_brake = cmd->emergency_brake_en;
                parking_brake = cmd->parking_brake_req;
            } else if (rx_frame.can_id == CAN_ID_PCM_TELEMETRY && rx_frame.can_dlc >= sizeof(PcmTelemetryMsg)) {
                PcmTelemetryMsg *pcm = (PcmTelemetryMsg *)rx_frame.data;
                veh_speed_mps = ((float)pcm->speed_kph_x10 / 10.0f) / 3.6f;
            }
        }

        /* 2. ABS & Hydraulic Actuator Simulation Step (every 20ms) */
        float dt = (now_ms - last_sim_ms) / 1000.0f;
        if (dt >= 0.02f) {
            last_sim_ms = now_ms;

            /* Target hydraulic pressure: ~20 Nm per bar */
            float target_pressure = (float)req_brake_torque / 22.0f;
            if (parking_brake) target_pressure = 45.0f;
            if (target_pressure > 140.0f) target_pressure = 140.0f;

            /* Hydraulic slew rate (pressure rise/fall response) */
            float p_diff = target_pressure - hydraulic_pressure_bar;
            hydraulic_pressure_bar += p_diff * 0.35f;

            /* Wheel Slip & ABS Intervention Logic:
             * High deceleration and pressure (> 65 bar) at speed triggers ABS slip modulation.
             */
            abs_active_mask = 0;
            esc_active = 0;

            if (hydraulic_pressure_bar > 65.0f && veh_speed_mps > 2.0f) {
                /* Simulate ABS modulation pulsing (high frequency pressure dumping) */
                int pulse_phase = (int)(now_ms / 60) % 2;
                if (pulse_phase == 1) {
                    /* Dump pressure to prevent wheel lockup */
                    hydraulic_pressure_bar *= 0.88f;
                    abs_active_mask = 0x0F; /* All 4 wheels modulating */
                } else {
                    abs_active_mask = 0x05; /* Front wheels modulating */
                }
            }

            if (emergency_brake) {
                esc_active = 1;
            }

            actual_friction_torque = hydraulic_pressure_bar * 21.5f;

            /* Thermal dynamics of brake rotors */
            float heat_input = (actual_friction_torque * veh_speed_mps) * 0.00015f;
            float cooling = (pad_temp_fl - 25.0f) * 0.008f;
            pad_temp_fl += (heat_input - cooling) * dt;
            pad_temp_fr = pad_temp_fl + 1.2f; /* slight natural variance */

            if (pad_temp_fl > 450.0f) pad_temp_fl = 450.0f;
            if (pad_temp_fl < 20.0f) pad_temp_fl = 20.0f;
        }

        /* 3. Transmit Brake Telemetry at 10Hz (every 100ms) on vcan0 */
        if (now_ms - last_tx_ms >= 100) {
            last_tx_ms = now_ms;

            BrakeTelemetryMsg msg;
            memset(&msg, 0, sizeof(msg));
            msg.actual_brake_torque_nm = (uint16_t)actual_friction_torque;
            msg.abs_active = abs_active_mask;
            msg.brake_pressure_bar = (uint8_t)hydraulic_pressure_bar;
            msg.pad_temp_fl_c = (int8_t)pad_temp_fl;
            msg.pad_temp_fr_c = (int8_t)pad_temp_fr;
            msg.esc_engaged = esc_active;
            msg.brake_status = (pad_temp_fl > 300.0f) ? 1 : 0;

            can_send_msg(sock, CAN_ID_BRAKE_TELEMETRY, &msg, sizeof(msg));
        }

        usleep(5000);
    }

    close(sock);
    printf("[Brake Node] Stopped cleanly.\n");
    return 0;
}
