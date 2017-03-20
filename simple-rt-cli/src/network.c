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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "network.h"
#include "tun.h"
#include "linux-adk.h"

static int g_tun_fd = 0;
static pthread_t g_tun_thread = 0;
static volatile int g_tun_is_running = 0;

#define TUN_NET_ADDR_HELPER(a,b,c) (    \
        (uint32_t)((0) << 24) |         \
        (uint32_t)((c) << 16) |         \
        (uint32_t)((b) << 8)  |         \
        (uint32_t)((a) << 0)            \
)

#define TUN_NET_ADDR TUN_NET_ADDR_HELPER(10,1,1)
#define NETWORK_ADDRESS(addr) ((addr) & 0x00FFFFFF)

static inline void process_network_packet(uint8_t *data, ssize_t size)
{
    uint32_t dst_addr;
    uint8_t *p = data + 16; /* ptr to dst ip */

    /* strange packet, ignore */
    if (size < 20) {
        return;
    }

    /* only ipv4 supported */
    if (((*data >> 4) & 0xf) != 4) {
        return;
    }

    dst_addr =
        (uint32_t) (p[3] << 24) |
        (uint32_t) (p[2] << 16) |
        (uint32_t) (p[1] << 8)  |
        (uint32_t) (p[0] << 0);

    if (NETWORK_ADDRESS(dst_addr) == TUN_NET_ADDR) {

        printf("packet size = %zu, dest addr =  %s, device id = %d\n",
                size,
                inet_ntoa(*(struct in_addr *) &dst_addr),
                (dst_addr >> 24) & 0xFF);

        /* int transferred; */
        /* libusb_bulk_transfer(acc_global_handle, */
        /*         AOA_ACCESSORY_EP_OUT, data, size, &transferred, ACC_TIMEOUT); */
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

bool start_network(void)
{
    int tun_fd = 0;
    char tun_name[IFNAMSIZ] = { 0 };

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
    if (g_tun_thread) {
        puts("stopping network");
        pthread_join(g_tun_thread, NULL);
        g_tun_thread = 0;
    }

    g_tun_is_running = 0;
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

