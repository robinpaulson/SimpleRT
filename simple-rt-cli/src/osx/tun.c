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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>

#include "utils.h"

#define MY_SC_UNIT 2

bool is_tun_present(void)
{
    /* assume utun always present */
    return true;
}

int tun_alloc(char *dev)
{
    int fd = 0;
    struct sockaddr_ctl sc = { 0 };
    struct ctl_info ctlInfo = { 0 };

    if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >=
            sizeof(ctlInfo.ctl_name))
    {
        fprintf(stderr,"UTUN_CONTROL_NAME too long");
        return -1;
    }

    if ((fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL)) < 0) {
        perror("socket(SYSPROTO_CONTROL)");
        return -1;
    }

    if (ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
        perror("ioctl(CTLIOCGINFO)");
        close(fd);
        return -1;
    }

    sc.sc_id = ctlInfo.ctl_id;
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    sc.sc_unit = MY_SC_UNIT;

    // If the connect is successful, a tun%d device will be created,
    // where "%d" is our unit number -1

    if (connect(fd, (struct sockaddr *) &sc, sizeof(sc)) == -1) {
        perror("connect(AF_SYS_CONTROL)");
        close(fd);
        return -1;
    }

    snprintf(dev, 16, "utun%d", sc.sc_unit - 1);

    return fd;
}

const char *get_system_nameserver(void)
{
    puts("!!! System nameserver is not supported by macOS version, default will be used !!!");
    return DEFAULT_NAMESERVER;
}

