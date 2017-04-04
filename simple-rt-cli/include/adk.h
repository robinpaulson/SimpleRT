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

#include "accessory.h"

typedef accessory_id_t (*gen_new_serial_str_cb)(char *str, size_t size);

accessory_t *probe_usb_device(struct libusb_device *dev,
        gen_new_serial_str_cb gen_new_serial_str);

ssize_t read_usb_packet(struct libusb_device_handle *handle,
        uint8_t *data, size_t size);

ssize_t write_usb_packet(struct libusb_device_handle *handle,
        const uint8_t *data, size_t size);

#endif /* _LINUX_ADK_H_ */
