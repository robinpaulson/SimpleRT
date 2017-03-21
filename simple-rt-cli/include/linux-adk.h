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

#ifndef _LINUX_ADK_H_
#define _LINUX_ADK_H_

#include <stdint.h>
#include <libusb.h>

#define ACC_BUF_SIZE 4096

typedef struct accessory_t accessory_t;

accessory_t *new_accessory(struct libusb_device *dev);
void free_accessory(accessory_t *acc);

void run_accessory_detached(accessory_t *acc);
int send_accessory_packet(void *data, size_t size, uint32_t acc_id);

#endif /* _LINUX_ADK_H_ */
