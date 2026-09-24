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
    printf("[EPS Node] Starting Electric Power Steering (EPS) on %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[EPS Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    set_socket_timeout(sock, 10);

    /* Steering State */
    int8_t target_steer_angle = 0;
    int8_t lka_overlay = 0;
    uint8_t steer_mode = MODE_COMFORT;
    float veh_speed_kph = 0.0f;

    /* Dynamic internal state */
    float actual_angle = 0.0f;
    float hand_torque_nm = 0.0f;
    float motor_assist_nm = 0.0f;
    uint8_t lka_active = 0;
    uint8_t eps_status = 0;

    uint64_t last_tx_ms = 0;
    uint64_t last_sim_ms = get_timestamp_ms();

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();

        /* 1. Receive Steering Commands and PCM Telemetry */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0) {
            if (rx_frame.can_id == CAN_ID_STEER_CMD && rx_frame.can_dlc >= sizeof(SteerCmdMsg)) {
                SteerCmdMsg *cmd = (SteerCmdMsg *)rx_frame.data;
                target_steer_angle = cmd->driver_steer_angle;
                lka_overlay = cmd->lka_torque_overlay;
                steer_mode = cmd->steering_mode;
            } else if (rx_frame.can_id == CAN_ID_PCM_TELEMETRY && rx_frame.can_dlc >= sizeof(PcmTelemetryMsg)) {
                PcmTelemetryMsg *pcm = (PcmTelemetryMsg *)rx_frame.data;
                veh_speed_kph = (float)pcm->speed_kph_x10 / 10.0f;
            }
        }

        /* 2. EPS Motor & Rack Dynamics Step (every 20ms) */
        float dt = (now_ms - last_sim_ms) / 1000.0f;
        if (dt >= 0.02f) {
            last_sim_ms = now_ms;

            /* Rack pinion angle slew toward target with tire self-centering resistance */
            float angle_diff = (float)target_steer_angle - actual_angle;
            actual_angle += angle_diff * 0.25f;

            /* Driver hand torque proportional to steering effort and speed */
            float speed_factor = (veh_speed_kph > 10.0f) ? (veh_speed_kph / 50.0f) : 0.3f;
            hand_torque_nm = (actual_angle * 0.045f) * speed_factor;

            /* Speed-sensitive assist gain calculation:
             * Parking speed (<15 km/h) -> high assist gain (3.8)
             * Highway speed (>80 km/h) -> lower assist gain (1.4)
             */
            float assist_gain = 3.6f - (fminf(veh_speed_kph, 120.0f) / 120.0f) * 2.2f;

            /* Mode modifier */
            if (steer_mode == MODE_COMFORT) assist_gain *= 1.25f;
            else if (steer_mode == MODE_SPORT) assist_gain *= 0.75f;

            motor_assist_nm = hand_torque_nm * assist_gain;

            /* LKA Torque Overlay */
            if (lka_overlay != 0) {
                float lka_nm = (float)lka_overlay / 10.0f;
                motor_assist_nm += lka_nm;
                lka_active = 1;
            } else {
                lka_active = 0;
            }

            eps_status = 0; /* OK */
        }

        /* 3. Transmit EPS Telemetry at 20Hz (every 50ms) on vcan0 */
        if (now_ms - last_tx_ms >= 50) {
            last_tx_ms = now_ms;

            SteerTelemetryMsg msg;
            memset(&msg, 0, sizeof(msg));
            msg.actual_steer_angle = (int8_t)actual_angle;
            msg.motor_assist_nm_x10 = (int16_t)(motor_assist_nm * 10.0f);
            msg.driver_hand_torque_x10 = (int16_t)(hand_torque_nm * 10.0f);
            msg.eps_status = eps_status;
            msg.lka_active = lka_active;

            can_send_msg(sock, CAN_ID_STEER_TELEMETRY, &msg, sizeof(msg));
        }

        usleep(5000);
    }

    close(sock);
    printf("[EPS Node] Stopped cleanly.\n");
    return 0;
}
