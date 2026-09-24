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
    const char *ifname = (argc > 1) ? argv[1] : CAN_BUS_BODY;
    printf("[HMI Node] Starting driver input simulation on %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[HMI Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    uint64_t start_time = get_timestamp_ms();
    uint64_t last_tx_ms = 0;

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();
        float elapsed_sec = (float)(now_ms - start_time) / 1000.0f;

        /* Simulated Driving Cycle (30 second loop) */
        float cycle_time = fmodf(elapsed_sec, 32.0f);

        uint8_t throttle = 0;
        uint8_t brake = 0;
        int8_t steer = 0;
        uint8_t gear = GEAR_DRIVE;
        uint8_t mode = MODE_COMFORT;

        if (cycle_time < 3.0f) {
            /* Parked & initializing */
            gear = GEAR_PARK;
            throttle = 0;
            brake = 20; /* Foot on brake */
            mode = MODE_COMFORT;
        } else if (cycle_time < 7.0f) {
            /* Shifting to Drive, gentle initial acceleration */
            gear = GEAR_DRIVE;
            throttle = 25;
            brake = 0;
            mode = MODE_COMFORT;
        } else if (cycle_time < 14.0f) {
            /* Acceleration to cruising speed */
            gear = GEAR_DRIVE;
            throttle = 55;
            brake = 0;
            steer = 2; /* Slight right curve */
            mode = MODE_COMFORT;
        } else if (cycle_time < 20.0f) {
            /* Aggressive acceleration in SPORT mode */
            gear = GEAR_DRIVE;
            throttle = 90;
            brake = 0;
            steer = -3;
            mode = MODE_SPORT;
        } else if (cycle_time < 25.0f) {
            /* Coasting and light braking */
            gear = GEAR_DRIVE;
            throttle = 0;
            brake = 30;
            mode = MODE_ECO;
        } else {
            /* Braking to a stop */
            gear = GEAR_DRIVE;
            throttle = 0;
            brake = 65;
            mode = MODE_ECO;
        }

        /* Periodic Transmission (50ms = 20Hz) */
        if (now_ms - last_tx_ms >= 50) {
            last_tx_ms = now_ms;

            HmiInputMsg hmi_msg;
            memset(&hmi_msg, 0, sizeof(hmi_msg));
            hmi_msg.throttle_percent = throttle;
            hmi_msg.brake_percent = brake;
            hmi_msg.steering_angle = steer;
            hmi_msg.selected_gear = gear;
            hmi_msg.drive_mode = mode;

            can_send_msg(sock, CAN_ID_HMI_INPUT, &hmi_msg, sizeof(hmi_msg));
        }

        usleep(10000);
    }

    close(sock);
    printf("[HMI Node] Stopped cleanly.\n");
    return 0;
}
