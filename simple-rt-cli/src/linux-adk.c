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
#include <stdbool.h>
#include <unistd.h>
#include "linux-adk.h"

static const accessory_t acc_default = {
    .manufacturer = "Konstantin Menyaev",
    .model = "SimpleRT",
    .description = "Simple Reverse Tethering",
    .version = "1.0",
    .url = "https://github.com/vvviperrr/SimpleRT",
    .serial = "10.1.1.2,8.8.8.8",
};

/* check, is arrived device is accessory */
bool is_accessory_present(accessory_t *acc)
{
    static const uint16_t aoa_pids[] = {
        AOA_ACCESSORY_PID,
        AOA_ACCESSORY_ADB_PID,
        AOA_AUDIO_PID,
        AOA_AUDIO_ADB_PID,
        AOA_ACCESSORY_AUDIO_PID,
        AOA_ACCESSORY_AUDIO_ADB_PID
    };

    /* Trying to open all the AOA IDs possible */
    for (size_t i = 0; i < sizeof(aoa_pids) / sizeof(aoa_pids[0]); i++) {
        uint16_t vid = AOA_ACCESSORY_VID;
        uint16_t pid = aoa_pids[i];

        if (acc->vid == vid && acc->pid == pid) {
            int ret = libusb_open(acc->device, &acc->handle);
            if (ret == 0) {
                printf("Found accessory %4.4x:%4.4x\n", vid, pid);
                return true;
            } else {
                fprintf(stderr, "Error opening accessory device: %s\n", libusb_strerror(ret));
                return false;
            }
        }
    }

    /* this is not accessory */
    return false;
}

int init_accessory(accessory_t *acc)
{
    int ret = 0, is_detached = 0;

    /* Check if device is not already in accessory mode */
    if (is_accessory_present(acc)) {
        return 0;
    }

    /* FIXME */
    /* Trying to open supplied device */
    acc->handle = libusb_open_device_with_vid_pid(NULL, acc->vid, acc->pid);
    if (acc->handle == NULL) {
        printf("Unable to open device...\n");
        return -1;
    }

    /* Check whether a kernel driver is attached. If so, we'll need to detach it. */
    if (libusb_kernel_driver_active(acc->handle, AOA_ACCESSORY_INTERFACE)) {
        puts("Kernel driver is active!");
        ret = libusb_detach_kernel_driver(acc->handle, AOA_ACCESSORY_INTERFACE);
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
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_GET_PROTOCOL, 0, 0,
            (uint8_t *) &acc->aoa_version,
            sizeof(acc->aoa_version), 0);
    if (ret < 0) {
        printf("Device %4.4x:%4.4x is not support accessory! Reason: %s\n",
                acc->vid, acc->pid, libusb_strerror(ret));
        goto error;
    }

    if (!acc->aoa_version) {
        puts("Device is not support accessory");
        goto error;
    }

    printf("Device supports AOA %d.0!\n", acc->aoa_version);

    /* Some Android devices require a waiting period between transfer calls */
    usleep(10000);

    /* In case of a no_app accessory, the version must be >= 2 */
    if ((acc->aoa_version < 2) && !acc->manufacturer) {
        printf("Connecting without an Android App only for AOA 2.0\n");
        goto error;
    }

    printf("Sending identification to the device\n");

    printf(" sending manufacturer: %s\n", acc->manufacturer);
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_SEND_IDENT, 0,
            AOA_STRING_MAN_ID,
            (uint8_t *) acc->manufacturer,
            strlen(acc->manufacturer) + 1, 0);
    if (ret < 0)
        goto error;

    printf(" sending model: %s\n", acc->model);
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_SEND_IDENT, 0,
            AOA_STRING_MOD_ID,
            (uint8_t *) acc->model,
            strlen(acc->model) + 1, 0);
    if (ret < 0)
        goto error;

    printf(" sending description: %s\n", acc->description);
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_SEND_IDENT, 0, AOA_STRING_DSC_ID,
            (uint8_t *) acc->description,
            strlen(acc->description) + 1, 0);
    if (ret < 0)
        goto error;

    printf(" sending version: %s\n", acc->version);
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_SEND_IDENT, 0, AOA_STRING_VER_ID,
            (uint8_t *) acc->version,
            strlen(acc->version) + 1, 0);
    if (ret < 0)
        goto error;

    printf(" sending url: %s\n", acc->url);
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_SEND_IDENT, 0, AOA_STRING_URL_ID,
            (uint8_t *) acc->url,
            strlen(acc->url) + 1, 0);
    if (ret < 0)
        goto error;

    printf(" sending serial number: %s\n", acc->serial);
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_SEND_IDENT, 0, AOA_STRING_SER_ID,
            (uint8_t *) acc->serial,
            strlen(acc->serial) + 1, 0);
    if (ret < 0)
        goto error;

    printf("Turning the device in Accessory mode\n");
    ret = libusb_control_transfer(acc->handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
            AOA_START_ACCESSORY, 0, 0, NULL, 0, 0);
    if (ret < 0)
        goto error;

    puts("Accessory was inited");

error:
    if (ret < 0) {
        printf("Accessory init failed: %d\n", ret);

        if (is_detached) {
            libusb_attach_kernel_driver(acc->handle, AOA_ACCESSORY_INTERFACE);
        }
    }

    return ret < 0 ? -1 : 0;
}

accessory_t *new_accessory(struct libusb_device *dev)
{
    accessory_t *acc;
    struct libusb_device_descriptor desc;

    libusb_get_device_descriptor(dev, &desc);

    acc = malloc(sizeof(accessory_t));
    *acc = acc_default;

    acc->vid = desc.idVendor;
    acc->pid = desc.idProduct;
    acc->device = libusb_ref_device(dev);

    return acc;
}

void free_accessory(accessory_t *acc)
{
    if (!acc) {
        return;
    }

    if (acc->device) {
        libusb_unref_device(acc->device);
    }

    if (acc->handle) {
        printf("Closing USB device\n");
        libusb_release_interface(acc->handle, 0);
        libusb_close(acc->handle);
    }

    free(acc);
}

