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
    printf("[PCM Node] Starting on interface %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[PCM Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    /* Configure socket timeout (10ms) to allow physics loop iteration */
    set_socket_timeout(sock, 10);

    /* Vehicle Physics State */
    float speed_kph = 0.0f;
    float motor_rpm = 0.0f;
    float actual_torque_nm = 0.0f;
    float target_torque_nm = 0.0f;
    float motor_temp_c = 28.0f;
    uint8_t gear_state = GEAR_DRIVE;
    uint8_t inverter_enabled = 1;

    uint64_t last_tx_ms = 0;
    uint64_t last_physics_ms = get_timestamp_ms();

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();

        /* 1. Receive commands from Central Compute (CAN_ID_PCM_CMD) */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0) {
            if (rx_frame.can_id == CAN_ID_PCM_CMD && rx_frame.can_dlc >= sizeof(PcmCmdMsg)) {
                PcmCmdMsg *cmd = (PcmCmdMsg *)rx_frame.data;
                target_torque_nm = (float)cmd->target_torque_nm;
                inverter_enabled = cmd->inverter_enable;
            }
        }

        /* 2. Physics & Motor Simulation Step (approx every 20ms) */
        float dt = (now_ms - last_physics_ms) / 1000.0f;
        if (dt >= 0.02f) {
            last_physics_ms = now_ms;

            if (!inverter_enabled) {
                target_torque_nm = 0.0f;
            }

            /* Torque slew rate limit (smoother acceleration) */
            float torque_diff = target_torque_nm - actual_torque_nm;
            actual_torque_nm += torque_diff * 0.15f;

            /* Vehicle mass: ~1600kg, Drag + rolling friction resistance */
            float aero_drag = 0.0035f * speed_kph * speed_kph;
            float roll_drag = (speed_kph > 0.1f) ? 3.0f : 0.0f;
            float net_force = (actual_torque_nm * 4.5f) - (aero_drag + roll_drag);

            /* Acceleration: a = F / m */
            float accel_kph_s = (net_force / 1600.0f) * 3.6f;
            speed_kph += accel_kph_s * dt;
            if (speed_kph < 0.0f) speed_kph = 0.0f;
            if (speed_kph > 220.0f) speed_kph = 220.0f;

            /* Motor RPM based on gear ratio and wheel circumference */
            motor_rpm = speed_kph * 75.0f;

            /* Motor temperature dynamics */
            float heat_gen = fabsf(actual_torque_nm) * 0.008f;
            float cooling = (motor_temp_c - 25.0f) * 0.015f;
            motor_temp_c += (heat_gen - cooling) * dt;
        }

        /* 3. Periodic Telemetry Transmission (100ms = 10Hz) */
        if (now_ms - last_tx_ms >= 100) {
            last_tx_ms = now_ms;

            PcmTelemetryMsg pcm_msg;
            pcm_msg.speed_kph_x10 = (uint16_t)(speed_kph * 10.0f);
            pcm_msg.motor_rpm = (uint16_t)motor_rpm;
            pcm_msg.motor_torque_nm = (int16_t)actual_torque_nm;
            pcm_msg.motor_temp_c = (int8_t)motor_temp_c;
            pcm_msg.gear_state = gear_state;

            can_send_msg(sock, CAN_ID_PCM_TELEMETRY, &pcm_msg, sizeof(pcm_msg));
        }

        usleep(5000); /* 5ms sleep to prevent CPU spinning */
    }

    close(sock);
    printf("[PCM Node] Stopped cleanly.\n");
    return 0;
}
