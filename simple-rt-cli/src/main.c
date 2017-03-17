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
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include "linux-adk.h"
#include "tun.h"

#define ACC_BUF_SIZE 4096
#define ACC_TIMEOUT 200

static void *tun_thread_proc(void *param)
{
    int ret;
    ssize_t nread;
    int transferred;
    uint8_t acc_buf[ACC_BUF_SIZE];
    accessory_t *acc = param;

    while (acc->is_running) {
        nread = read(acc->tun_fd, acc_buf, sizeof(acc_buf));
        if (nread > 0) {
            ret = libusb_bulk_transfer(acc->handle,
                    AOA_ACCESSORY_EP_OUT, acc_buf, nread, &transferred, ACC_TIMEOUT);
            if (ret < 0) {
                if (ret == LIBUSB_ERROR_TIMEOUT) {
                    continue;
                } else {
                    fprintf(stderr, "Tun thread: bulk transfer error: %s\n",
                            libusb_strerror(ret));
                    break;
                }
            }
        } else if (nread < 0) {
            fprintf(stderr, "Error reading from tun: %s\n", strerror(errno));
            break;
        } else {
            /* EOF received */
            break;
        }
    }

    acc->is_running = false;

    return NULL;
}

static void *accessory_thread_proc(void *param)
{
    int ret;
    ssize_t nwrite;
    int transferred;
    uint8_t acc_buf[ACC_BUF_SIZE];
    accessory_t *acc = param;

    /* Claiming first (accessory) interface from the opened device */
    ret = libusb_claim_interface(acc->handle, AOA_ACCESSORY_INTERFACE);
    if (ret != 0) {
        fprintf(stderr, "Error claiming interface: %s\n", libusb_strerror(ret));
        goto end;
    }

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
            nwrite = write(acc->tun_fd, acc_buf, transferred);
            if (nwrite < 0) {
                fprintf(stderr, "Error writing into tun: %s\n", strerror(errno));
                break;
            }
        }
    }

end:
    acc->is_running = false;
    return NULL;
}

static void *connection_thread_proc(void *param)
{
    pthread_t tun_thread, accessory_thread;
    accessory_t *acc = param;
    int tun_fd = 0;
    char tun_name[IFNAMSIZ] = { 0 };

    /* FIXME: wait for hotplug_callback released */
    usleep(100);

    if (!is_accessory_present(acc)) {
        if (init_accessory(acc) < 0) {
            /* init failed, possibly usb device not support accessory, it's ok */
        } else {
            /* accessory init success, wait for present */
        }
        return NULL;
    }

    puts("accessory connected!");

    tun_fd = tun_alloc(tun_name);

    if (tun_fd < 0) {
        perror("tun_alloc failed");
        goto end;
    }

    if (!iface_up(tun_name)) {
        fprintf(stderr, "Unable set iface %s up\n", tun_name);
        goto end;
    }

    printf("%s interface configured!\n", tun_name);

    acc->tun_fd = tun_fd;
    acc->is_running = true;

    pthread_create(&tun_thread, NULL, tun_thread_proc, acc);
    pthread_create(&accessory_thread, NULL, accessory_thread_proc, acc);

    pthread_join(tun_thread, NULL);
    pthread_join(accessory_thread, NULL);

end:
    acc->is_running = false;
    close(acc->tun_fd);
    fini_accessory(acc);

    return NULL;
}

static int hotplug_callback(struct libusb_context *ctx,
        struct libusb_device *dev,
        libusb_hotplug_event event,
        void *user_data)
{
    struct libusb_device_descriptor desc;
    accessory_t *acc = user_data;

    if (acc->is_running) {
        /* one accessory already running */
        return 0;
    }

    libusb_get_device_descriptor(dev, &desc);

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        acc->vid = desc.idVendor;
        acc->pid = desc.idProduct;

        pthread_t th;
        pthread_attr_t attrs;
        pthread_attr_init(&attrs);
        pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
        pthread_create(&th, &attrs, connection_thread_proc, acc);
    } else {
        fprintf(stderr, "Unknown libusb_hotplug_event: %d\n", event);
    }

    /* TODO: support device detached event */

    return 0;
}

int main(int argc, char *argv[])
{
    accessory_t *acc;
    int rc = 0, debug_mode = 0;
    libusb_hotplug_callback_handle callback_handle;

    if (argc > 1) {
        const char *param = argv[1];

        if (strcmp(param, "-h") == 0) {
            puts("usage: sudo simple-rt [-h -d]");
            return EXIT_SUCCESS;
        } else if (strcmp(param, "-d") == 0) {
            puts("debug mode enabled");
            debug_mode = 1;
        } else {
            fprintf(stderr, "Unknown param: %s\n", param);
            return EXIT_FAILURE;
        }
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Run app as root!\n");
        return EXIT_FAILURE;
    }

    if (!is_tun_present()) {
        fprintf(stderr, "Tun dev is not present. Is kernel module loaded?\n");
        return EXIT_FAILURE;
    }

    libusb_init(NULL);

    if (debug_mode) {
        libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
    }

    acc = new_accessory();

    rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0,
            LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            hotplug_callback, acc, &callback_handle);

    if (rc != LIBUSB_SUCCESS) {
        fprintf(stderr, "Error creating a hotplug callback\n");
        libusb_exit(NULL);
        exit(1);
    }

    puts("libusb callback registered!");

    while (true) {
        libusb_handle_events_completed(NULL, NULL);
        usleep(1);
    }

    if (callback_handle) {
        libusb_hotplug_deregister_callback(NULL, callback_handle);
        puts("libusb callback deregistered!");
    }

    free_accessory(acc);
    libusb_exit(NULL);

    return EXIT_SUCCESS;
}

