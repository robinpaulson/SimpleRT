/*
 * SimpleRT: Reverse tethering utility for Android
 * Copyright (C) 2016 Konstantin Menyaev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "network.h"
#include "linux-adk.h"

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define SIMPLERT_NETWORK_ADDRESS_BUILDER(a,b,c,d) ( \
        (uint32_t) ((a) << 24) |                    \
        (uint32_t) ((b) << 16) |                    \
        (uint32_t) ((c) << 8)  |                    \
        (uint32_t) ((d) << 0)                       \
)

#define SIMPLERT_NETWORK_ADDRESS \
    SIMPLERT_NETWORK_ADDRESS_BUILDER(10,1,1,0)

#define NETWORK_ADDRESS(addr) \
    ((addr) & 0xffffff00)

#define ACC_ID_FROM_ADDR(addr) \
    ((addr) & 0xff)

#define DNS_ADDRESS "8.8.8.8"

#define IFACE_UP_SH_PATH "./iface_up.sh"

extern int tun_alloc(char *dev);
extern bool is_tun_present(void);

static int g_tun_fd = 0;
static pthread_t g_tun_thread = 0;
static volatile int g_tun_is_running = 0;

static inline void dump_addr_info(uint32_t addr, ssize_t size)
{
    uint32_t tmp = htonl(addr);
    printf("packet size = %3zu, dest addr = %s, device id = %d\n", size,
            inet_ntoa(*(struct in_addr *) &tmp),
            addr & 0xff);
}

static inline void process_network_packet(uint8_t *data, ssize_t size)
{
    uint32_t dst_addr;

    /* only ipv4 supported */
    if (size < 20 || ((*data >> 4) & 0xf) != 4) {
        return;
    }

    /* dest ip addr in LE */
    dst_addr =
        (uint32_t) (data[16] << 24) |
        (uint32_t) (data[17] << 16) |
        (uint32_t) (data[18] << 8)  |
        (uint32_t) (data[19] << 0);

    if (NETWORK_ADDRESS(dst_addr) == SIMPLERT_NETWORK_ADDRESS) {
        /* dump_addr_info(dst_addr, size); */
        send_accessory_packet(data, size, ACC_ID_FROM_ADDR(dst_addr));
    }
}

static void *tun_thread_proc(void *arg)
{
    ssize_t nread;
    uint8_t acc_buf[ACC_BUF_SIZE];

    while (g_tun_is_running) {
        if ((nread = read(g_tun_fd, acc_buf, sizeof(acc_buf))) > 0) {
            process_network_packet(acc_buf, nread);
        } else if (nread < 0) {
            fprintf(stderr, "Error reading from tun: %s\n", strerror(errno));
            break;
        } else {
            /* EOF received */
            break;
        }
    }

    g_tun_is_running = false;
    return NULL;
}

/* FIXME */
static bool iface_up(const char *dev)
{
    char cmd[1024] = { 0 };
    char net_addr_str[32] = { 0 };
    char host_addr_str[32] = { 0 };

    uint32_t net_addr = htonl(SIMPLERT_NETWORK_ADDRESS);
    uint32_t host_addr = htonl(SIMPLERT_NETWORK_ADDRESS | 0x1);
    uint32_t mask = __builtin_popcount(NETWORK_ADDRESS(-1));

    snprintf(net_addr_str, sizeof(net_addr_str), "%s",
            inet_ntoa(*(struct in_addr *) &net_addr));

    snprintf(host_addr_str, sizeof(host_addr_str), "%s",
            inet_ntoa(*(struct in_addr *) &host_addr));

    snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %u\n",
            IFACE_UP_SH_PATH, PLATFORM, dev, net_addr_str, host_addr_str, mask);

    return system(cmd) == 0;
}

bool start_network(void)
{
    int tun_fd = 0;
    char tun_name[IFNAMSIZ] = { 0 };

    if (g_tun_is_running) {
        fprintf(stderr, "Network already started!\n");
        return false;
    }

    puts("starting network");

    if (!is_tun_present()) {
        fprintf(stderr, "Tun dev is not present. Is kernel module loaded?\n");
        return false;
    }

    if ((tun_fd = tun_alloc(tun_name)) < 0) {
        perror("tun_alloc failed");
        return false;
    }

    if (!iface_up(tun_name)) {
        fprintf(stderr, "Unable set iface %s up\n", tun_name);
        close(tun_fd);
        return false;
    }

    g_tun_fd = tun_fd;
    printf("%s interface configured!\n", tun_name);

    g_tun_is_running = 1;
    pthread_create(&g_tun_thread, NULL, tun_thread_proc, NULL);

    return true;
}

void stop_network(void)
{
    g_tun_is_running = 0;

    if (g_tun_thread) {
        puts("stopping network");
        pthread_join(g_tun_thread, NULL);
        g_tun_thread = 0;
    }

    if (g_tun_fd) {
        close(g_tun_fd);
        g_tun_fd = 0;
    }
}

int send_network_packet(void *data, size_t size)
{
    ssize_t nwrite;

    nwrite = write(g_tun_fd, data, size);
    if (nwrite < 0) {
        fprintf(stderr, "Error writing into tun: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

char *fill_serial_param(char *buf, size_t size, uint32_t acc_id)
{
    uint32_t addr = htonl(SIMPLERT_NETWORK_ADDRESS | acc_id);
    snprintf(buf, size, "%s,%s", inet_ntoa(*(struct in_addr *) &addr), DNS_ADDRESS);
    return buf;
}

