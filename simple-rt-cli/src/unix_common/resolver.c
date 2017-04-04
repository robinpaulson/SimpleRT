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

#include <string.h>
#include <resolv.h>
#include <arpa/inet.h>

#include "utils.h"

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

