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
    printf("[Telematics Node] Starting Cellular Cloud Gateway & OTA Manager on %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[Telematics Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    set_socket_timeout(sock, 10);

    /* Telematics State */
    uint8_t cellular_csq = 28;      /* 28/31 ~ Excellent 5G signal */
    uint8_t cloud_connected = 2;    /* 2: Cloud Sync Active */
    uint8_t ota_state = OTA_STATE_IDLE;
    uint8_t ota_progress = 0;
    uint8_t remote_cmd = 0;
    uint8_t gnss_fix = 2;           /* 3D RTK Fix */
    uint16_t cloud_latency_ms = 35;

    uint64_t start_time = get_timestamp_ms();
    uint64_t last_tx_ms = 0;

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();
        float elapsed_sec = (float)(now_ms - start_time) / 1000.0f;
        float cycle = fmodf(elapsed_sec, 40.0f);

        /* 1. Receive Telematics Commands from Central Compute */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0) {
            if (rx_frame.can_id == CAN_ID_TELEMATICS_CMD && rx_frame.can_dlc >= sizeof(TelematicsCmdMsg)) {
                /* Central compute acknowledged remote command */
                remote_cmd = 0;
            }
        }

        /* 2. OTA State Machine & Cloud Simulation Cycle (40s demonstration cycle) */
        if (cycle < 8.0f) {
            ota_state = OTA_STATE_IDLE;
            ota_progress = 0;
            remote_cmd = 0;
            cloud_latency_ms = 32 + ((int)cycle % 5);
        } else if (cycle < 18.0f) {
            /* Cloud pushes an OTA firmware update campaign */
            ota_state = OTA_STATE_DOWNLOADING;
            float p = ((cycle - 8.0f) / 10.0f) * 100.0f;
            ota_progress = (uint8_t)fminf(p, 100.0f);
            cloud_latency_ms = 42 + ((int)cycle % 8);
        } else if (cycle < 24.0f) {
            /* Package downloaded, cryptographic verification */
            ota_state = OTA_STATE_VERIFYING;
            ota_progress = 100;
            cloud_latency_ms = 36;
        } else if (cycle < 32.0f) {
            /* Flashing firmware to secondary A/B storage partition */
            ota_state = OTA_STATE_FLASHING;
            float p = ((cycle - 24.0f) / 8.0f) * 100.0f;
            ota_progress = (uint8_t)fminf(p, 100.0f);
            cloud_latency_ms = 35;
        } else {
            /* OTA Complete! */
            ota_state = OTA_STATE_COMPLETE;
            ota_progress = 100;
            cloud_latency_ms = 30;
        }

        /* 3. Transmit Telematics Status at 2Hz (every 500ms) on vcan1 */
        if (now_ms - last_tx_ms >= 500) {
            last_tx_ms = now_ms;

            TelematicsStatusMsg msg;
            memset(&msg, 0, sizeof(msg));
            msg.cellular_csq = cellular_csq;
            msg.cloud_connected = cloud_connected;
            msg.ota_state = ota_state;
            msg.ota_progress_pct = ota_progress;
            msg.remote_command = remote_cmd;
            msg.gnss_fix = gnss_fix;
            msg.cloud_latency_ms = cloud_latency_ms;

            can_send_msg(sock, CAN_ID_TELEMATICS_STATUS, &msg, sizeof(msg));
        }

        usleep(10000);
    }

    close(sock);
    printf("[Telematics Node] Stopped cleanly.\n");
    return 0;
}
