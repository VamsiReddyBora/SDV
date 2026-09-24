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
    printf("[ADAS Node] Starting Radar/Vision Object Detection on %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[ADAS Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    set_socket_timeout(sock, 10);

    /* Ego Vehicle State */
    float ego_speed_kph = 0.0f;

    /* Lead Target Simulation Variables */
    float lead_dist_m = 65.0f;
    float relative_speed_kph = 0.0f;
    uint8_t fcw_alert = 0;
    uint8_t aeb_request = AEB_NONE;
    uint8_t lane_departure = 0;
    uint8_t ttc_sec_x10 = 255;

    uint64_t start_time = get_timestamp_ms();
    uint64_t last_tx_ms = 0;
    uint64_t last_sim_ms = get_timestamp_ms();

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();

        /* 1. Receive Ego Vehicle Speed from PCM (CAN_ID_PCM_TELEMETRY) */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0) {
            if (rx_frame.can_id == CAN_ID_PCM_TELEMETRY && rx_frame.can_dlc >= sizeof(PcmTelemetryMsg)) {
                PcmTelemetryMsg *pcm = (PcmTelemetryMsg *)rx_frame.data;
                ego_speed_kph = (float)pcm->speed_kph_x10 / 10.0f;
            }
        }

        /* 2. ADAS Perception & Radar Simulation Step (every 20ms) */
        float dt = (now_ms - last_sim_ms) / 1000.0f;
        if (dt >= 0.02f) {
            last_sim_ms = now_ms;
            float elapsed_sec = (float)(now_ms - start_time) / 1000.0f;
            float cycle = fmodf(elapsed_sec, 36.0f);

            /* Scenario Simulation Timeline:
             * 0-8s: Clear road, lead vehicle distant (~60m), matching speeds.
             * 8-16s: Cruising, lead vehicle decelerates gently.
             * 16-24s: SUDDEN HAZARD: Lead vehicle hard braking! Relative speed drops sharply, distance shrinks.
             * 24-30s: Emergency AEB engaged by vehicle, speed drops, distance stabilizes at ~5m.
             * 30-36s: Clear road resume, lane departure drift check.
             */
            if (cycle < 8.0f) {
                lead_dist_m = 60.0f + (cycle * 0.5f);
                relative_speed_kph = (ego_speed_kph > 5.0f) ? -2.0f : 0.0f;
                fcw_alert = 0;
                aeb_request = AEB_NONE;
                lane_departure = 0;
            } else if (cycle < 16.0f) {
                lead_dist_m = 50.0f - ((cycle - 8.0f) * 1.5f);
                relative_speed_kph = 45.0f - ego_speed_kph; /* Target cruising at 45 km/h */
                fcw_alert = 0;
                aeb_request = AEB_NONE;
                lane_departure = 0;
            } else if (cycle < 22.0f) {
                /* Target braking hard! Target speed down to 5 km/h */
                float target_speed_kph = 5.0f;
                relative_speed_kph = target_speed_kph - ego_speed_kph;
                float closing_mps = (relative_speed_kph < 0.0f) ? (fabsf(relative_speed_kph) / 3.6f) : 0.1f;
                lead_dist_m -= closing_mps * dt;
                if (lead_dist_m < 4.0f) lead_dist_m = 4.0f;

                /* Calculate Time-To-Collision (TTC) */
                if (closing_mps > 0.5f && lead_dist_m > 0.0f) {
                    float ttc = lead_dist_m / closing_mps;
                    if (ttc > 25.0f) ttc_sec_x10 = 255;
                    else ttc_sec_x10 = (uint8_t)(ttc * 10.0f);

                    if (ttc < 2.4f && ttc >= 1.3f) {
                        fcw_alert = 1; /* Forward Collision Warning: Advisory */
                        aeb_request = AEB_PRECHARGE;
                    } else if (ttc < 1.3f) {
                        fcw_alert = 2; /* Imminent Collision Warning */
                        aeb_request = AEB_FULL_EMERGENCY; /* Immediate Emergency Braking */
                    }
                }
            } else if (cycle < 28.0f) {
                /* Emergency stop achieved, target stopped at 5m */
                lead_dist_m = 5.5f;
                relative_speed_kph = 0.0f;
                ttc_sec_x10 = 255;
                fcw_alert = 0;
                aeb_request = AEB_NONE;
                lane_departure = 0;
            } else {
                /* Resume driving & Lane centering test */
                lead_dist_m = 75.0f;
                relative_speed_kph = 5.0f;
                ttc_sec_x10 = 255;
                fcw_alert = 0;
                aeb_request = AEB_NONE;
                /* Simulate slight lane drift at t=32-34s */
                if (cycle >= 32.0f && cycle < 35.0f) {
                    lane_departure = 1; /* Left lane departure */
                } else {
                    lane_departure = 0;
                }
            }
        }

        /* 3. Transmit ADAS Telemetry at 20Hz (every 50ms) on vcan0 */
        if (now_ms - last_tx_ms >= 50) {
            last_tx_ms = now_ms;

            AdasTelemetryMsg msg;
            memset(&msg, 0, sizeof(msg));
            msg.target_distance_m_x10 = (uint16_t)(lead_dist_m * 10.0f);
            msg.relative_speed_kph_x10 = (int16_t)(relative_speed_kph * 10.0f);
            msg.ttc_seconds_x10 = ttc_sec_x10;
            msg.fcw_alert = fcw_alert;
            msg.aeb_request = aeb_request;
            msg.lane_departure_warning = lane_departure;

            can_send_msg(sock, CAN_ID_ADAS_TELEMETRY, &msg, sizeof(msg));
        }

        usleep(5000);
    }

    close(sock);
    printf("[ADAS Node] Stopped cleanly.\n");
    return 0;
}
