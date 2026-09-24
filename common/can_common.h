#ifndef CAN_COMMON_H
#define CAN_COMMON_H

#include <stdint.h>
#include <linux/can.h>
#include <linux/can/raw.h>

/**
 * Open and bind a raw CAN socket to the specified interface (e.g. "vcan0").
 * Returns socket file descriptor, or -1 on error.
 */
int open_can_socket(const char *ifname);

/**
 * Configure kernel-level CAN ID filter on socket.
 */
int set_can_filter(int sock, const struct can_filter *filters, int count);

/**
 * Send a standard CAN frame with the specified ID and payload.
 * Returns bytes written, or -1 on error.
 */
int can_send_msg(int sock, uint32_t can_id, const void *payload, uint8_t len);

/**
 * Receive a CAN frame (blocking or with socket timeout).
 * Returns bytes read, 0 on timeout, or -1 on error.
 */
int can_recv_msg(int sock, struct can_frame *frame);

/**
 * Set socket receive timeout in milliseconds.
 */
int set_socket_timeout(int sock, int timeout_ms);

/**
 * Get current monotonic timestamp in milliseconds.
 */
uint64_t get_timestamp_ms(void);

#endif /* CAN_COMMON_H */
