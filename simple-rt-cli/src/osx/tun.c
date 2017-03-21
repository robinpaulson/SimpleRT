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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "tun.h"

#define TUN_NAME "tun0"
#define TUN_DEV "/dev/" TUN_NAME

bool is_tun_present(void)
{
    return access(TUN_DEV, F_OK) == 0;
}

int tun_alloc(char *dev)
{
    int fd;

    if ((fd = open(TUN_DEV, O_RDWR)) < 0 ) {
        perror("error open tun");
        return -1;
    }

    strncpy(dev, TUN_NAME, IFNAMSIZ - 1);

    return fd;
}
