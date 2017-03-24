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

#include "accessory.h"
#include "linux-adk.h"
#include "network.h"
#include "utils.h"

typedef struct accessory_t {
    uint32_t id;
    /* FIXME: atomic needed */
    volatile int is_running;
    struct libusb_device_handle *handle;
} accessory_t;

static accessory_t acc_reserved;

/* acc list for 255.255.255.0 network mask */
static accessory_t *acc_list[256] = {
    [0]     = &acc_reserved,    /* reserved, network addr   */
    [1]     = &acc_reserved,    /* reserved, host addr      */
    [255]   = &acc_reserved,    /* reserved, broadcast addr */
};

static bool is_accessory_id_valid(uint32_t acc_id)
{
    if (!acc_id || acc_id >= ARRAY_SIZE(acc_list)) {
        return false;
    }

    return true;
}

static uint32_t acquire_accessory_id(void)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(acc_list); i++) {
        if (acc_list[i] == NULL) {
            /* free id found */
            return i;
        }
    }

    return 0;
}

static void release_accessory_id(uint32_t acc_id)
{
    if (!is_accessory_id_valid(acc_id)) {
        return;
    }

    acc_list[acc_id] = NULL;
}

static bool store_accessory_id(accessory_t *acc, uint32_t acc_id)
{
    if (!acc || !is_accessory_id_valid(acc_id)) {
        return false;
    }

    acc->id = acc_id;
    acc_list[acc->id] = acc;
    return true;
}

static accessory_t *find_accessory_by_id(uint32_t acc_id)
{
    if (!is_accessory_id_valid(acc_id)) {
        return NULL;
    }

    return acc_list[acc_id];
}

static void accessory_worker_proc(accessory_t *acc)
{
    uint8_t acc_buf[ACC_BUF_SIZE];
    uint32_t acc_id = 0;
    ssize_t nread;

    puts("accessory connected!");

    acc->is_running = true;

    /* read first packet and map acc->id */
    while (acc->is_running) {
        if ((nread = read_usb_packet(acc->handle, acc_buf, sizeof(acc_buf))) > 0) {
            if ((acc_id = get_acc_id_from_packet(acc_buf, nread, false)) != 0) {
                store_accessory_id(acc, acc_id);
                break;
            }
        } else if (nread < 0) {
            goto end;
        } else {
            continue;
        }
    }

    /* read rest packets */
    while (acc->is_running) {
        if ((nread = read_usb_packet(acc->handle, acc_buf, sizeof(acc_buf))) > 0) {
            if (send_network_packet(acc_buf, nread) < 0) {
                break;
            }
        } else if (nread < 0) {
            break;
        } else {
            continue;
        }
    }

end:
    acc->is_running = false;
    free_accessory(acc);
}

/* FIXME: mutex */
accessory_t *new_accessory(struct libusb_device_handle *handle)
{
    accessory_t *acc = NULL;

    acc = malloc(sizeof(accessory_t));
    acc->id = 0;
    acc->is_running = false;
    acc->handle = handle;

    return acc;
}

/* FIXME: mutex */
void free_accessory(accessory_t *acc)
{
    if (!acc) {
        return;
    }

    if (acc->id) {
        release_accessory_id(acc->id);
    }

    if (acc->handle) {
        printf("Closing accessory device\n");
        libusb_release_interface(acc->handle, 0);
        libusb_close(acc->handle);
    }

    free(acc);
}

/* FIXME: mutex */
int send_accessory_packet(void *data, size_t size, uint32_t acc_id)
{
    accessory_t *acc;

    if ((acc = find_accessory_by_id(acc_id)) != NULL) {
        if (write_usb_packet(acc->handle, data, size) < 0) {
            /* seems like accessory removed, just ignore */
        }
    } else {
        /* accessory not found, removed? */
    }

    return 0;
}

static void *usb_device_thread_proc(void *param)
{
    struct libusb_device *dev = param;
    accessory_t *acc;
    int acc_id;
    char serial[128] = { 0 };

    acc_id = acquire_accessory_id();
    if (acc_id == 0) {
        fprintf(stderr, "No free accessory id's left\n");
        goto end;
    }

    fill_serial_param(serial, sizeof(serial), acc_id);

    acc = probe_usb_device(dev, serial);
    if (acc == NULL) {
        goto end;
    }

    /* block here */
    accessory_worker_proc(acc);

end:
    return NULL;
}

void run_usb_probe_thread_detached(struct libusb_device *dev)
{
    pthread_t th;
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    pthread_create(&th, &attrs, usb_device_thread_proc, dev);
}
