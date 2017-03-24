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
    accessory_id_t id;
    /* FIXME: atomic needed */
    volatile int is_running;
    struct libusb_device_handle *handle;
} accessory_t;

static struct {
    bool used;
    accessory_t *acc;
} acc_list[256] = {
    [0]     =   { .used = true }, /* reserved, network addr   */
    [1]     =   { .used = true }, /* reserved, host addr      */
    [255]   =   { .used = true }, /* reserved, broadcast addr */
};

static bool is_accessory_id_valid(accessory_id_t id)
{
    return id && id < ARRAY_SIZE(acc_list);
}

static accessory_id_t acquire_accessory_id(void)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(acc_list); i++) {
        if (!acc_list[i].used) {
            acc_list[i].used = true;
            return i;
        }
    }

    return 0;
}

static void release_accessory_id(accessory_id_t id)
{
    if (!is_accessory_id_valid(id)) {
        return;
    }

    acc_list[id].acc = NULL;
    acc_list[id].used = false;
}

static bool store_accessory_id(accessory_t *acc, accessory_id_t id)
{
    if (!acc || !is_accessory_id_valid(id)) {
        return false;
    }

    acc->id = id;
    acc_list[acc->id].acc = acc;
    return true;
}

static accessory_t *find_accessory_by_id(accessory_id_t id)
{
    if (!is_accessory_id_valid(id)) {
        return NULL;
    }

    return acc_list[id].acc;
}

static void accessory_worker_proc(accessory_t *acc)
{
    uint8_t acc_buf[ACC_BUF_SIZE];
    accessory_id_t id = 0;
    ssize_t nread;

    puts("accessory connected!");

    acc->is_running = true;

    /* read first packet and map acc->id */
    while (acc->is_running) {
        if ((nread = read_usb_packet(acc->handle, acc_buf, sizeof(acc_buf))) > 0) {
            if ((id = get_acc_id_from_packet(acc_buf, nread, false)) != 0) {
                store_accessory_id(acc, id);
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
int send_accessory_packet(const uint8_t *data, size_t size,
        accessory_id_t id)
{
    accessory_t *acc;

    if ((acc = find_accessory_by_id(id)) != NULL) {
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
    accessory_id_t id;
    char serial[128] = { 0 };

    if ((id = acquire_accessory_id()) == 0) {
        fprintf(stderr, "No free accessory id's left\n");
        goto end;
    }

    fill_serial_param(serial, sizeof(serial), id);

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
