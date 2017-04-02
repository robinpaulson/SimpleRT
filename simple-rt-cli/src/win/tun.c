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

#include "utils.h"

bool is_tun_present(void)
{
    return false;
}

int tun_alloc(char *dev_name, size_t dev_name_size)
{
    return -1;
}

ssize_t tun_read_ip_packet(int fd, uint8_t *packet, size_t size)
{
    return 0;
}

ssize_t tun_write_ip_packet(int fd, const uint8_t *packet, size_t size)
{
    return 0;
}

const char *get_system_nameserver(void)
{
    puts("!!! System nameserver is not supported by Windows version, default will be used !!!");
    return DEFAULT_NAMESERVER;
}

