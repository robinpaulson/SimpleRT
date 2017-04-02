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
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "utils.h"
#include "linux-adk.h"

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
#define ACC_TIMEOUT 200

static bool is_accessory_present(struct libusb_device *dev)
{
    static const uint16_t aoa_pids[] = {
        AOA_ACCESSORY_PID,
        AOA_ACCESSORY_ADB_PID,
        AOA_AUDIO_PID,
        AOA_AUDIO_ADB_PID,
        AOA_ACCESSORY_AUDIO_PID,
        AOA_ACCESSORY_AUDIO_ADB_PID,
    };

    struct libusb_device_descriptor desc;

    libusb_get_device_descriptor(dev, &desc);

    for (size_t i = 0; i < ARRAY_SIZE(aoa_pids); i++) {
        if (desc.idVendor == AOA_ACCESSORY_VID && desc.idProduct == aoa_pids[i]) {
            printf("Found accessory %4.4x:%4.4x\n", desc.idVendor, desc.idProduct);
            return true;
        }
    }

    return false;
}

accessory_t *probe_usb_device(struct libusb_device *dev,
        gen_new_serial_str_cb gen_new_serial_str)
{
    int ret = 0;
    int is_detached = 0;
    uint16_t aoa_version = 0;
    char serial_str[128] = { 0 };

    struct libusb_device_handle *handle = NULL;

    if ((ret = libusb_open(dev, &handle)) != 0) {
        fprintf(stderr, "Error opening usb device: %s\n",
                libusb_strerror(ret));
        return NULL;
    }

    if (is_accessory_present(dev)) {
        /* Claiming first (accessory) interface from usb device */
        ret = libusb_claim_interface(handle, AOA_ACCESSORY_INTERFACE);
        if (ret != 0) {
            fprintf(stderr, "Error claiming interface: %s\n", libusb_strerror(ret));
            goto error;
        }

        /* create accessory struct */
        return new_accessory(handle);
    }

    /* Check whether a kernel driver is attached. If so, we'll need to detach it. */
    if (libusb_kernel_driver_active(handle, AOA_ACCESSORY_INTERFACE)) {
        puts("Kernel driver is active!");
        ret = libusb_detach_kernel_driver(handle, AOA_ACCESSORY_INTERFACE);
        if (ret == 0) {
            is_detached = 1;
            puts("Kernel driver detached!");
        } else {
            fprintf(stderr, "Error detaching kernel driver: %s\n", libusb_strerror(ret));
            goto error;
        }
    } else {
        puts("Kernel driver is not active!");
    }

    /* Now asking if device supports Android Open Accessory protocol */
    ret = libusb_control_transfer(handle,
            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_GET_PROTOCOL, 0, 0,
            (uint8_t *) &aoa_version, sizeof(aoa_version), 0);
    if (ret < 0) {
        goto error;
    }

    if (!aoa_version) {
        fprintf(stderr, "Device is not support accessory!\n");
        ret = 0;
        goto error;
    }

    printf("Device supports AOA %d.0!\n", aoa_version);

    if (gen_new_serial_str(serial_str, sizeof(serial_str)) == 0) {
        ret = 0;
        goto error;
    }

    struct acc_control_params_t {
        const char *str;
        uint8_t request_type;
        uint8_t bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        const char *data;
        uint32_t timeout;
    } acc_control_params[] = {
        {
            .str = "manufacturer",
            .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            .bRequest = AOA_SEND_IDENT,
            .wIndex = AOA_STRING_MAN_ID,
            .data = "Konstantin Menyaev",
        },
        {
            .str = "model",
            .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            .bRequest = AOA_SEND_IDENT,
            .wIndex = AOA_STRING_MOD_ID,
            .data = "SimpleRT",
        },
        {
            .str = "description",
            .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            .bRequest = AOA_SEND_IDENT,
            .wIndex = AOA_STRING_DSC_ID,
            .data = "Simple Reverse Tethering",
        },
        {
            .str = "version",
            .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            .bRequest = AOA_SEND_IDENT,
            .wIndex = AOA_STRING_VER_ID,
            .data = "1.0",
        },
        {
            .str = "url",
            .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            .bRequest = AOA_SEND_IDENT,
            .wIndex = AOA_STRING_URL_ID,
            .data = "https://github.com/vvviperrr/SimpleRT",
        },
        {
            .str = "serial number",
            .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            .bRequest = AOA_SEND_IDENT,
            .wIndex = AOA_STRING_SER_ID,
            .data = serial_str,
        },
        {
            .str = "command",
            .request_type = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            .bRequest = AOA_START_ACCESSORY,
        },
        { NULL, 0, 0, 0, 0, NULL, 0 },
    };

    printf("Sending identification to the device\n");
    for (struct acc_control_params_t *acp = acc_control_params;
            acp->str != NULL; acp++)
    {
        const char *data = acp->data ? acp->data : "start accessory";
        uint16_t data_len = acp->data ? strlen(acp->data) + 1 : 0;

        printf(" sending %s: %s\n", acp->str, data);

        ret = libusb_control_transfer(handle,
                acp->request_type,
                acp->bRequest,
                acp->wValue,
                acp->wIndex,
                (uint8_t *) acp->data,
                data_len,
                acp->timeout);
        if (ret < 0) {
            goto error;
        }
    }

    puts("Accessory was inited successfully!");

error:
    if (ret < 0) {
        fprintf(stderr, "Accessory init failed: %s\n", libusb_strerror(ret));
    }

    if (is_detached) {
        libusb_attach_kernel_driver(handle, AOA_ACCESSORY_INTERFACE);
    }

    if (handle) {
        libusb_close(handle);
    }

    return NULL;
}

/* FIXME: read_all semantic */
ssize_t read_usb_packet(struct libusb_device_handle *handle,
        uint8_t *data, size_t size)
{
    int ret;
    int transferred;

    while (true) {
        ret = libusb_bulk_transfer(handle, AOA_ACCESSORY_EP_IN,
                data, size, &transferred, ACC_TIMEOUT);
        if (ret < 0) {
            if (ret == LIBUSB_ERROR_TIMEOUT) {
                continue;
            } else {
                fprintf(stderr, "read_usb_packet failed: %s\n",
                        libusb_strerror(ret));
                return -1;
            }
        } else {
            /* success */
            break;
        }
    }

    return transferred;
}

/* FIXME: write_all semantic */
ssize_t write_usb_packet(struct libusb_device_handle *handle,
        const uint8_t *data, size_t size)
{
    int ret;
    int transferred;

    while (true) {
        ret = libusb_bulk_transfer(handle, AOA_ACCESSORY_EP_OUT,
                (uint8_t *) data, size, &transferred, ACC_TIMEOUT);
        if (ret < 0) {
            if (ret == LIBUSB_ERROR_TIMEOUT) {
                continue;
            } else {
                fprintf(stderr, "write_usb_packet failed: %s\n",
                        libusb_strerror(ret));
                return -1;
            }
        } else {
            /* success */
            break;
        }
    }

    return transferred;
}

