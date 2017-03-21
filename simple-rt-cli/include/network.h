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

#ifndef _NETWORH_H_
#define _NETWORH_H_

#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_NAMESERVER "8.8.8.8"

typedef struct {
    const char *nameserver;
} network_config_t;

bool start_network(const network_config_t *config);
void stop_network(void);
int send_network_packet(void *data, size_t size);

char *fill_serial_param(char *buf, size_t size, uint32_t acc_id);
char *get_system_nameserver(void);

#endif
