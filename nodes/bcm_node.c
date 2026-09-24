#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../common/can_common.h"
#include "../common/vehicle_protocol.h"

static volatile int g_running = 1;

static void sig_handler(int signum) {
    (void)signum;
    g_running = 0;
}

int main(int argc, char *argv[]) {
    const char *ifname = (argc > 1) ? argv[1] : CAN_BUS_BODY;
    printf("[BCM Node] Starting on interface %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[BCM Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    set_socket_timeout(sock, 10);

    /* Body State */
    uint8_t doors_locked = 0x00;   /* Bitmask: 0x0F = all 4 doors locked, 0x00 = unlocked */
    uint8_t lights_active = 0x00;  /* Bit 0: LowBeam, Bit 1: HighBeam, Bit 2: Hazard, Bit 3: Brake */
    int8_t cabin_temp_c = 22;
    uint8_t ambient_lux = 75;      /* Daytime */
    uint8_t wipers_active = 0;

    uint64_t last_tx_ms = 0;

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();

        /* Receive commands from Central Compute (CAN_ID_BCM_CMD) */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0) {
            if (rx_frame.can_id == CAN_ID_BCM_CMD && rx_frame.can_dlc >= sizeof(BcmCmdMsg)) {
                BcmCmdMsg *cmd = (BcmCmdMsg *)rx_frame.data;
                if (cmd->lock_command == 1) {
                    doors_locked = 0x0F; /* All doors locked */
                } else if (cmd->lock_command == 2) {
                    doors_locked = 0x00; /* All doors unlocked */
                }

                lights_active = cmd->light_command;
            }
        }

        /* Periodic Telemetry (250ms = 4Hz) */
        if (now_ms - last_tx_ms >= 250) {
            last_tx_ms = now_ms;

            BcmTelemetryMsg bcm_msg;
            memset(&bcm_msg, 0, sizeof(bcm_msg));
            bcm_msg.doors_locked = doors_locked;
            bcm_msg.lights_active = lights_active;
            bcm_msg.cabin_temp_c = cabin_temp_c;
            bcm_msg.ambient_lux = ambient_lux;
            bcm_msg.wipers_active = wipers_active;

            can_send_msg(sock, CAN_ID_BCM_TELEMETRY, &bcm_msg, sizeof(bcm_msg));
        }

        usleep(10000);
    }

    close(sock);
    printf("[BCM Node] Stopped cleanly.\n");
    return 0;
}
