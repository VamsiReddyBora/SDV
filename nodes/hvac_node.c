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
    printf("[HVAC Node] Starting Cabin & Battery Thermal Loop on %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[HVAC Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    set_socket_timeout(sock, 10);

    /* Command settings from Central Compute */
    int8_t target_temp_c = 21;
    uint8_t fan_speed = 3;
    uint8_t compressor_en = 1;
    uint8_t battery_chill_req = 0;

    /* Thermal Loop State Variables */
    float cabin_temp_c = 26.5f;       /* Starts slightly warm */
    float evaporator_temp_c = 18.0f;
    float coolant_loop_temp_c = 27.0f;
    float compressor_power_w = 450.0f;
    uint8_t blower_rpm = 180;          /* 1800 RPM / 10 */

    uint64_t last_tx_ms = 0;
    uint64_t last_sim_ms = get_timestamp_ms();

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();

        /* 1. Receive Commands from Central Compute */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0) {
            if (rx_frame.can_id == CAN_ID_HVAC_CMD && rx_frame.can_dlc >= sizeof(HvacCmdMsg)) {
                HvacCmdMsg *cmd = (HvacCmdMsg *)rx_frame.data;
                target_temp_c = cmd->target_cabin_temp_c;
                fan_speed = cmd->fan_speed;
                compressor_en = cmd->ac_compressor_enable;
                battery_chill_req = cmd->battery_cooling_req;
            }
        }

        /* 2. Thermodynamic Simulation Step (every 50ms) */
        float dt = (now_ms - last_sim_ms) / 1000.0f;
        if (dt >= 0.05f) {
            last_sim_ms = now_ms;

            /* Evaporator cooling */
            if (compressor_en) {
                float target_evap = 4.0f;
                evaporator_temp_c += (target_evap - evaporator_temp_c) * 0.08f;
            } else {
                evaporator_temp_c += (25.0f - evaporator_temp_c) * 0.02f;
            }

            /* Cabin cooling/heating towards target */
            float air_cooling_rate = (fan_speed / 7.0f) * 0.12f;
            cabin_temp_c += ((float)target_temp_c - cabin_temp_c) * air_cooling_rate * dt;

            /* Battery liquid coolant chilling loop */
            float target_coolant = 25.0f;
            if (battery_chill_req == 1) {
                target_coolant = 20.0f; /* Active cooling */
            } else if (battery_chill_req == 2) {
                target_coolant = 15.0f; /* Max chill */
            }
            coolant_loop_temp_c += (target_coolant - coolant_loop_temp_c) * 0.05f * dt;

            /* Compressor electrical power draw */
            blower_rpm = fan_speed * 30; /* up to 210 (2100 RPM) */
            float fan_w = fan_speed * 25.0f;
            float ac_w = 0.0f;
            if (compressor_en) {
                ac_w = 600.0f + (battery_chill_req * 850.0f) + (fabsf(cabin_temp_c - target_temp_c) * 150.0f);
            }
            compressor_power_w = fan_w + ac_w;
        }

        /* 3. Transmit HVAC Telemetry at 4Hz (every 250ms) on vcan1 */
        if (now_ms - last_tx_ms >= 250) {
            last_tx_ms = now_ms;

            HvacTelemetryMsg msg;
            memset(&msg, 0, sizeof(msg));
            msg.current_cabin_temp_c = (int8_t)roundf(cabin_temp_c);
            msg.evaporator_temp_c = (int8_t)roundf(evaporator_temp_c);
            msg.coolant_loop_temp_c = (int8_t)roundf(coolant_loop_temp_c);
            msg.compressor_power_w = (uint16_t)compressor_power_w;
            msg.blower_rpm_x10 = blower_rpm;
            msg.hvac_status = (battery_chill_req > 0) ? 1 : 0;

            can_send_msg(sock, CAN_ID_HVAC_TELEMETRY, &msg, sizeof(msg));
        }

        usleep(10000);
    }

    close(sock);
    printf("[HVAC Node] Stopped cleanly.\n");
    return 0;
}
