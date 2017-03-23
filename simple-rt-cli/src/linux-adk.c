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
#include <pthread.h>

#include "linux-adk.h"
#include "network.h"

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

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

/* Structures */
typedef struct accessory_t {
    const char *manufacturer;
    const char *model;
    const char *description;
    const char *version;
    const char *url;

    uint32_t id;
    uint16_t vid;
    uint16_t pid;
    uint16_t aoa_version;

    /* FIXME: atomic needed */
    volatile int is_running;

    /*FIXME: using as accessory params, dirty hack? */
    char serial[128];

    struct libusb_device *device;
    struct libusb_device_handle *handle;
} accessory_t;


static const accessory_t acc_default = {
    .manufacturer = "Konstantin Menyaev",
    .model = "SimpleRT",
    .description = "Simple Reverse Tethering",
    .version = "1.0",
    .url = "https://github.com/vvviperrr/SimpleRT",
};

static accessory_t acc_reserved;

/* acc list for 255.255.255.0 network mask */
static accessory_t *acc_list[256] = {
    [0]     = &acc_reserved,    /* reserved, network addr   */
    [1]     = &acc_reserved,    /* reserved, host addr      */
    [255]   = &acc_reserved,    /* reserved, broadcast addr */
};

static uint32_t find_free_accessory_id(void)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(acc_list); i++) {
        if (acc_list[i] == NULL) {
            /* free id found */
            return i;
        }
    }
    return 0;
}

/* check, is arrived device is accessory */
static bool is_accessory_present(accessory_t *acc)
{
    int ret;

    static const uint16_t aoa_pids[] = {
        AOA_ACCESSORY_PID,
        AOA_ACCESSORY_ADB_PID,
        AOA_AUDIO_PID,
        AOA_AUDIO_ADB_PID,
        AOA_ACCESSORY_AUDIO_PID,
        AOA_ACCESSORY_AUDIO_ADB_PID
    };

    /* Trying to open all the AOA IDs possible */
    for (size_t i = 0; i < ARRAY_SIZE(aoa_pids); i++) {
        if (acc->vid == AOA_ACCESSORY_VID && acc->pid == aoa_pids[i]) {
            if ((ret = libusb_open(acc->device, &acc->handle)) == 0) {
                printf("Found accessory %4.4x:%4.4x\n", acc->vid, acc->pid);
                return true;
            } else {
                fprintf(stderr, "Error opening accessory device: %s\n",
                        libusb_strerror(ret));
                return false;
            }
        }
    }

    /* this is not accessory */
    return false;
}

static int init_accessory(accessory_t *acc)
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

    int new_acc_id;
    if ((new_acc_id = find_free_accessory_id()) == 0) {
        fprintf(stderr, "No free accessory id's left\n");
        goto error;
    }

    fill_serial_param(acc->serial, sizeof(acc->serial), new_acc_id);

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

/* detached per-device working thread */
static void *accessory_thread_proc(void *param)
{
    accessory_t *acc = param;
    int ret = 0, transferred = 0;
    uint8_t acc_buf[ACC_BUF_SIZE];
    uint32_t acc_id = 0;

    /* init accessory from new connected device and wait for present */
    if (!is_accessory_present(acc)) {
        init_accessory(acc);
        goto end;
    }

    puts("accessory connected!");

    /* Claiming first (accessory) interface from the opened device */
    ret = libusb_claim_interface(acc->handle, AOA_ACCESSORY_INTERFACE);
    if (ret != 0) {
        fprintf(stderr, "Error claiming interface: %s\n", libusb_strerror(ret));
        goto end;
    }

    acc->is_running = true;

    /* read first packet and map acc->id */
    while (acc->is_running) {
        ret = libusb_bulk_transfer(acc->handle, AOA_ACCESSORY_EP_IN,
                acc_buf, sizeof(acc_buf), &transferred, ACC_TIMEOUT);
        if (ret < 0) {
            if (ret == LIBUSB_ERROR_TIMEOUT) {
                continue;
            } else {
                fprintf(stderr, "Acc thread: bulk transfer error: %s\n",
                        libusb_strerror(ret));
                acc->is_running = false;
                break;
            }
        } else {
            if ((acc_id = get_acc_id_from_packet(acc_buf, transferred, false)) != 0) {
                acc->id = acc_id;
                acc_list[acc->id] = acc;
                /* stored, break this cycle */
                break;
            } else {
                /* invalid packet, waiting next */
                continue;
            }
        }
    }

    /* read rest packets */
    while (acc->is_running) {
        ret = libusb_bulk_transfer(acc->handle, AOA_ACCESSORY_EP_IN,
                acc_buf, sizeof(acc_buf), &transferred, ACC_TIMEOUT);
        if (ret < 0) {
            if (ret == LIBUSB_ERROR_TIMEOUT) {
                continue;
            } else {
                fprintf(stderr, "Acc thread: bulk transfer error: %s\n",
                        libusb_strerror(ret));
                break;
            }
        } else {
            if (send_network_packet(acc_buf, transferred) < 0) {
                fprintf(stderr, "Send network packet faield!\n");
                break;
            }
        }
    }

end:
    acc->is_running = false;
    free_accessory(acc);
    return NULL;
}

void run_accessory_detached(accessory_t *acc)
{
    pthread_t th;
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    pthread_create(&th, &attrs, accessory_thread_proc, acc);
}

/* FIXME: mutex */
accessory_t *new_accessory(struct libusb_device *dev)
{
    accessory_t *acc = NULL;
    struct libusb_device_descriptor desc;

    libusb_get_device_descriptor(dev, &desc);

    acc = malloc(sizeof(accessory_t));
    *acc = acc_default;
    acc->vid = desc.idVendor;
    acc->pid = desc.idProduct;
    acc->device = libusb_ref_device(dev);

    return acc;
}

/* FIXME: mutex */
void free_accessory(accessory_t *acc)
{
    if (!acc) {
        return;
    }

    if (acc->id) {
        acc_list[acc->id] = NULL;
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

/* FIXME: mutex */
int send_accessory_packet(void *data, size_t size, uint32_t acc_id)
{
    accessory_t *acc;

    /* check is id valid */
    if (acc_id >= ARRAY_SIZE(acc_list)) {
        return -1;
    }

    if ((acc = acc_list[acc_id]) != NULL) {
        /* FIXME: error handling */
        int transferred;
        libusb_bulk_transfer(acc->handle,
                AOA_ACCESSORY_EP_OUT, data, size, &transferred, ACC_TIMEOUT);
    } else {
        /* accessory removed? */
    }

    return 0;
}

