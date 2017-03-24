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

#ifndef _ACCESSORY_H_
#define _ACCESSORY_H_

#include <stdint.h>
#include <libusb.h>

typedef uint32_t accessory_id_t;
typedef struct accessory_t accessory_t;

accessory_t *new_accessory(struct libusb_device_handle *handle);
void free_accessory(accessory_t *acc);

int send_accessory_packet(const uint8_t *data, size_t size,
        accessory_id_t id);

void run_usb_probe_thread_detached(struct libusb_device *dev);

#endif
