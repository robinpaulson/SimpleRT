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
#include <stdbool.h>

/* Android Open Accessory protocol defines */
#define AOA_GET_PROTOCOL            51
#define AOA_SEND_IDENT              52
#define AOA_START_ACCESSORY         53
#define AOA_REGISTER_HID            54
#define AOA_UNREGISTER_HID          55
#define AOA_SET_HID_REPORT_DESC     56
#define AOA_SEND_HID_EVENT          57
#define AOA_AUDIO_SUPPORT           58

/* String IDs */
#define AOA_STRING_MAN_ID           0
#define AOA_STRING_MOD_ID           1
#define AOA_STRING_DSC_ID           2
#define AOA_STRING_VER_ID           3
#define AOA_STRING_URL_ID           4
#define AOA_STRING_SER_ID           5

/* Product IDs / Vendor IDs */
#define AOA_ACCESSORY_VID           0x18D1	/* Google */
#define AOA_ACCESSORY_PID           0x2D00	/* accessory */
#define AOA_ACCESSORY_ADB_PID       0x2D01	/* accessory + adb */
#define AOA_AUDIO_PID               0x2D02	/* audio */
#define AOA_AUDIO_ADB_PID           0x2D03	/* audio + adb */
#define AOA_ACCESSORY_AUDIO_PID     0x2D04	/* accessory + audio */
#define AOA_ACCESSORY_AUDIO_ADB_PID 0x2D05	/* accessory + audio + adb */

/* Endpoint Addresses TODO get from interface descriptor */
#define AOA_ACCESSORY_EP_IN         0x81
#define AOA_ACCESSORY_EP_OUT        0x02
#define AOA_ACCESSORY_INTERFACE     0x00

/* ACC params */
#define ACC_BUF_SIZE 4096
#define ACC_TIMEOUT 200

/* Structures */
typedef struct {
    const char *manufacturer;
    const char *model;
    const char *description;
    const char *version;
    const char *url;
    const char *serial;
    uint16_t vid;
    uint16_t pid;
    uint16_t aoa_version;
    /* FIXME: atomic needed */
    volatile int is_running;
    struct libusb_device *device;
    struct libusb_device_handle *handle;
} accessory_t;

int init_accessory(accessory_t *acc);
bool is_accessory_present(accessory_t *acc);

accessory_t *new_accessory(struct libusb_device *dev);
void free_accessory(accessory_t *acc);

#endif /* _LINUX_ADK_H_ */
