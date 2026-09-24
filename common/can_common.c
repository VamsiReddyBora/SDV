#include "can_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

int open_can_socket(const char *ifname) {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("socket(PF_CAN)");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "ioctl(SIOCGIFINDEX) failed for interface %s: %s\n", ifname, strerror(errno));
        close(s);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "bind to %s failed: %s\n", ifname, strerror(errno));
        close(s);
        return -1;
    }

    return s;
}

int set_can_filter(int sock, const struct can_filter *filters, int count) {
    if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, filters, sizeof(struct can_filter) * count) < 0) {
        perror("setsockopt(CAN_RAW_FILTER)");
        return -1;
    }
    return 0;
}

int can_send_msg(int sock, uint32_t can_id, const void *payload, uint8_t len) {
    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));

    frame.can_id = can_id;
    frame.can_dlc = len > 8 ? 8 : len;
    if (payload && len > 0) {
        memcpy(frame.data, payload, frame.can_dlc);
    }

    ssize_t nbytes = write(sock, &frame, sizeof(struct can_frame));
    if (nbytes != sizeof(struct can_frame)) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        perror("can_send write");
        return -1;
    }
    return (int)nbytes;
}

int can_recv_msg(int sock, struct can_frame *frame) {
    ssize_t nbytes = read(sock, frame, sizeof(struct can_frame));
    if (nbytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }
    return (int)nbytes;
}

int set_socket_timeout(int sock, int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        return -1;
    }
    return 0;
}

uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}
