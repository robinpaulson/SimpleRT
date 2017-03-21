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
#include <unistd.h>

#include "linux-adk.h"
#include "network.h"

static int hotplug_callback(struct libusb_context *ctx,
        struct libusb_device *dev,
        libusb_hotplug_event event,
        void * arg)
{
    accessory_t *acc;

    if (event != LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        fprintf(stderr, "Unknown libusb_hotplug_event: %d\n", event);
        return 0;
    }

    if ((acc = new_accessory(dev)) == NULL) {
        fprintf(stderr, "Cannot create new accessory!\n");
        return 0;
    }

    run_accessory_detached(acc);
    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    libusb_hotplug_callback_handle callback_handle;

    network_config_t network_config = {
        .nameserver = DEFAULT_NAMESERVER,
    };

    libusb_init(NULL);

    if (argc > 1) {
        const char *param = argv[1];

        if (strcmp(param, "-n") == 0) {
            if (argc < 3) {
                fprintf(stderr, "nameserver required!\n");
                return EXIT_FAILURE;
            } else if (strcmp(argv[2], "local") == 0) {
                network_config.nameserver = get_system_nameserver();
            } else {
                network_config.nameserver = argv[2];
            }
        } else if (strcmp(param, "-h") == 0) {
            puts("usage: sudo simple-rt [-h | -n [nameserver | local] ]");
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "Unknown param: %s\n", param);
            return EXIT_FAILURE;
        }
    }

    if (getenv("DEBUG") != NULL) {
        puts("debug mode enabled");
        libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Run app as root!\n");
        return EXIT_FAILURE;
    }

    if (!start_network(&network_config)) {
        fprintf(stderr, "Unable to start network!\n");
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0,
            LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            hotplug_callback, NULL, &callback_handle);

    if (rc != LIBUSB_SUCCESS) {
        fprintf(stderr, "Error creating a hotplug callback\n");
        return EXIT_FAILURE;
    }

    puts("SimpleRT started!");

    while (true) {
        libusb_handle_events_completed(NULL, NULL);
        usleep(1);
    }

    stop_network();
    libusb_hotplug_deregister_callback(NULL, callback_handle);
    libusb_exit(NULL);

    return EXIT_SUCCESS;
}

