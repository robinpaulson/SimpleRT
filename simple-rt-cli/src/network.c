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

#include "tun.h"
#include "network.h"
#include "utils.h"

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

#ifndef IFACE_UP_SH_PATH
 #define IFACE_UP_SH_PATH "./iface_up.sh"
#endif

/* tun stuff */
static int g_tun_fd = 0;
static pthread_t g_tun_thread;
static volatile bool g_tun_is_running = false;

static inline void dump_addr_info(uint32_t addr, size_t size)
{
    uint32_t tmp = htonl(addr);
    printf("packet size = %zu, dest addr = %s, device id = %d\n", size,
            inet_ntoa(*(struct in_addr *) &tmp),
            addr & 0xff);
}

accessory_id_t get_acc_id_from_packet(const uint8_t *data, size_t size, bool dst_addr)
{
    uint32_t addr;
    unsigned int addr_offset = (dst_addr ? 16 : 12);

    /* only ipv4 supported */
    if (size < 20 || ((*data >> 4) & 0xf) != 4) {
        goto end;
    }

    /* dest ip addr in LE */
    addr =
        (uint32_t) (data[addr_offset + 0] << 24) |
        (uint32_t) (data[addr_offset + 1] << 16) |
        (uint32_t) (data[addr_offset + 2] << 8)  |
        (uint32_t) (data[addr_offset + 3] << 0);

    if (NETWORK_ADDRESS(addr) == SIMPLERT_NETWORK_ADDRESS) {
        /* dump_addr_info(addr, size); */
        return ACC_ID_FROM_ADDR(addr);
    }

end:
    return 0;
}

static void *tun_thread_proc(void *arg)
{
    ssize_t nread;
    uint8_t acc_buf[ACC_BUF_SIZE];
    accessory_id_t id = 0;

    g_tun_is_running = true;

    while (g_tun_is_running) {
        if ((nread = tun_read_ip_packet(g_tun_fd, acc_buf, sizeof(acc_buf))) > 0) {
            if ((id = get_acc_id_from_packet(acc_buf, nread, true)) != 0) {
                send_accessory_packet(acc_buf, nread, id);
            } else {
                /* invalid packet received, ignore */
            }
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

    simple_rt_config_t *config = get_simple_rt_config();

    uint32_t net_addr = htonl(SIMPLERT_NETWORK_ADDRESS);
    uint32_t host_addr = htonl(SIMPLERT_NETWORK_ADDRESS | 0x1);
    uint32_t mask = __builtin_popcount(NETWORK_ADDRESS(-1));

    snprintf(net_addr_str, sizeof(net_addr_str), "%s",
            inet_ntoa(*(struct in_addr *) &net_addr));

    snprintf(host_addr_str, sizeof(host_addr_str), "%s",
            inet_ntoa(*(struct in_addr *) &host_addr));

    snprintf(cmd, sizeof(cmd), "%s %s start %s %s %s %u %s %s\n",
            IFACE_UP_SH_PATH, PLATFORM, dev, net_addr_str, host_addr_str, mask,
            config->nameserver,
            config->interface);

    return system(cmd) == 0;
}

static bool iface_down(void)
{
    char cmd[1024] = { 0 };

    snprintf(cmd, sizeof(cmd), "%s %s stop",
            IFACE_UP_SH_PATH, PLATFORM);

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

    if ((tun_fd = tun_alloc(tun_name, sizeof(tun_name))) < 0) {
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

    pthread_create(&g_tun_thread, NULL, tun_thread_proc, NULL);

    return true;
}

void stop_network(void)
{
    if (g_tun_is_running) {
        puts("stopping network");
        g_tun_is_running = false;
        pthread_cancel(g_tun_thread);
        pthread_join(g_tun_thread, NULL);
        iface_down();
    }

    if (g_tun_fd) {
        close(g_tun_fd);
        g_tun_fd = 0;
    }
}

ssize_t send_network_packet(const uint8_t *data, size_t size)
{
    ssize_t nwrite;

    nwrite = tun_write_ip_packet(g_tun_fd, data, size);
    if (nwrite < 0) {
        fprintf(stderr, "Error writing into tun: %s\n",
                strerror(errno));
        return -1;
    }

    return nwrite;
}

char *fill_serial_param(char *buf, size_t size, accessory_id_t id)
{
    simple_rt_config_t *config = get_simple_rt_config();
    uint32_t addr = htonl(SIMPLERT_NETWORK_ADDRESS | id);

    snprintf(buf, size, "%s,%s",
            inet_ntoa(*(struct in_addr *) &addr),
            config->nameserver);

    return buf;
}

