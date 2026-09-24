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
    printf("[BMS Node] Starting on interface %s (PID: %d)...\n", ifname, getpid());

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int sock = open_can_socket(ifname);
    if (sock < 0) {
        fprintf(stderr, "[BMS Node] Failed to bind to %s\n", ifname);
        return 1;
    }

    set_socket_timeout(sock, 10);

    /* Battery State */
    float soc_percent = 88.0f;       /* Starting at 88% */
    float nominal_voltage = 400.0f;  /* 400V architecture */
    float pack_current_a = 0.0f;     /* Discharge > 0, Regen < 0 */
    float cell_temp_c = 26.0f;
    uint8_t soh_percent = 99;
    uint8_t bms_status = BMS_STATUS_OK;

    uint64_t last_tx_ms = 0;
    uint64_t last_calc_ms = get_timestamp_ms();

    while (g_running) {
        uint64_t now_ms = get_timestamp_ms();

        /* Listen for PCM telemetry on vcan0 to calculate load current */
        struct can_frame rx_frame;
        int nbytes = can_recv_msg(sock, &rx_frame);
        if (nbytes > 0 && rx_frame.can_id == CAN_ID_PCM_TELEMETRY && rx_frame.can_dlc >= sizeof(PcmTelemetryMsg)) {
            PcmTelemetryMsg *pcm = (PcmTelemetryMsg *)rx_frame.data;
            float torque = (float)pcm->motor_torque_nm;
            float rpm = (float)pcm->motor_rpm;

            /* Electrical power: P = torque * omega (rad/s) */
            float omega = rpm * (2.0f * 3.14159f / 60.0f);
            float mech_power_w = torque * omega;
            
            /* Inverter efficiency ~ 90% */
            float elec_power_w = (mech_power_w >= 0.0f) ? (mech_power_w / 0.90f) : (mech_power_w * 0.90f);
            /* Add base vehicle low-voltage parasitic load (computers, pumps, fans: ~500W) */
            elec_power_w += 500.0f;

            pack_current_a = elec_power_w / nominal_voltage;
        }

        /* Update Battery State */
        float dt = (now_ms - last_calc_ms) / 1000.0f;
        if (dt >= 0.05f) {
            last_calc_ms = now_ms;

            /* 75 kWh battery capacity = 75000 Wh / 400V = 187.5 Ah = 675,000 Ampere-seconds */
            float amp_seconds = pack_current_a * dt;
            float delta_soc = (amp_seconds / 675000.0f) * 100.0f;
            soc_percent -= delta_soc;
            if (soc_percent < 0.0f) soc_percent = 0.0f;
            if (soc_percent > 100.0f) soc_percent = 100.0f;

            /* Temperature dynamics (Joule heating: I^2 * R) */
            float i2r_heat = (pack_current_a * pack_current_a * 0.05f) * 0.0001f;
            float cooling = (cell_temp_c - 24.0f) * 0.01f;
            cell_temp_c += (i2r_heat - cooling) * dt;

            /* Status evaluation */
            if (cell_temp_c > 55.0f || soc_percent < 10.0f) {
                bms_status = BMS_STATUS_WARNING;
            } else if (cell_temp_c > 65.0f || soc_percent < 2.0f) {
                bms_status = BMS_STATUS_FAULT;
            } else {
                bms_status = BMS_STATUS_OK;
            }
        }

        /* Periodic BMS Telemetry (200ms = 5Hz) */
        if (now_ms - last_tx_ms >= 200) {
            last_tx_ms = now_ms;

            /* Internal resistance voltage drop: V = V_oc - I * R_int */
            float active_voltage = nominal_voltage * (0.85f + 0.15f * (soc_percent / 100.0f)) - (pack_current_a * 0.04f);

            BmsTelemetryMsg bms_msg;
            bms_msg.soc_percent = (uint8_t)soc_percent;
            bms_msg.pack_voltage_x10 = (uint16_t)(active_voltage * 10.0f);
            bms_msg.pack_current_x10 = (int16_t)(pack_current_a * 10.0f);
            bms_msg.max_cell_temp_c = (int8_t)cell_temp_c;
            bms_msg.bms_status = bms_status;
            bms_msg.soh_percent = soh_percent;

            can_send_msg(sock, CAN_ID_BMS_TELEMETRY, &bms_msg, sizeof(bms_msg));
        }

        usleep(5000);
    }

    close(sock);
    printf("[BMS Node] Stopped cleanly.\n");
    return 0;
}
