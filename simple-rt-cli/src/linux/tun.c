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
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "utils.h"

static const char clonedev[] = "/dev/net/tun";

bool is_tun_present(void)
{
    return access(clonedev, F_OK) == 0;
}

int tun_alloc(char *dev)
{
    int fd;
    int err;
    struct ifreq ifr;

    if ((fd = open(clonedev, O_RDWR)) < 0 ) {
        perror("error open tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        perror("error create tun");
        return err;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}

const char *get_system_nameserver(void)
{
    struct __res_state rs;
    static char buf[128];

    if (res_ninit(&rs) < 0) {
        goto end;
    }

    if (!rs.nscount) {
        goto end;
    }

    /* using first nameserver */
    memset(buf, 0, sizeof(buf));
    strncpy(buf, inet_ntoa(rs.nsaddr_list[0].sin_addr), sizeof(buf) - 1);
    return buf;

end:
    fprintf(stderr, "Cannot find system nameserver. Default one will be used.\n");
    return DEFAULT_NAMESERVER;
}

