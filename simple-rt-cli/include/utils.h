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

#ifndef _UTILS_H_
#define _UTILS_H_

#define DEFAULT_NAMESERVER "8.8.8.8"

#define ACC_BUF_SIZE 4096

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

typedef struct simple_rt_config_t {
    const char *interface;
    const char *nameserver;
} simple_rt_config_t;

extern simple_rt_config_t *get_simple_rt_config(void);
extern const char *get_system_nameserver(void);

#endif
