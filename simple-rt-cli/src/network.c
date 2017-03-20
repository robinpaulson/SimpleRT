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
#include "network.h"
#include "tun.h"
#include "linux-adk.h"

static int g_tun_fd = 0;

#if 0
static void *tun_thread_proc(void *param)
{
    int ret;
    ssize_t nread;
    int transferred;
    uint8_t acc_buf[ACC_BUF_SIZE];
    accessory_t *acc = param;

    while (acc->is_running) {
        nread = read(g_tun_fd, acc_buf, sizeof(acc_buf));
        if (nread > 0) {
            ret = libusb_bulk_transfer(acc->handle,
                    AOA_ACCESSORY_EP_OUT, acc_buf, nread, &transferred, ACC_TIMEOUT);
            if (ret < 0) {
                if (ret == LIBUSB_ERROR_TIMEOUT) {
                    continue;
                } else {
                    fprintf(stderr, "Tun thread: bulk transfer error: %s\n",
                            libusb_strerror(ret));
                    break;
                }
            }
        } else if (nread < 0) {
            fprintf(stderr, "Error reading from tun: %s\n", strerror(errno));
            break;
        } else {
            /* EOF received */
            break;
        }
    }

    acc->is_running = false;

    return NULL;
}
#endif

bool start_network(void)
{
    int tun_fd = 0;
    char tun_name[IFNAMSIZ] = { 0 };

    tun_fd = tun_alloc(tun_name);

    if (tun_fd < 0) {
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

    return true;
}

void stop_network(void)
{
}

int send_network(void *data, size_t count)
{
    ssize_t nwrite;

    nwrite = write(g_tun_fd, data, count);
    if (nwrite < 0) {
        fprintf(stderr, "Error writing into tun: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

