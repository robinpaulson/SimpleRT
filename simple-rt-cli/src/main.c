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
#include <getopt.h>
#include <libusb.h>

#include "accessory.h"
#include "network.h"
#include "utils.h"

static int hotplug_callback(struct libusb_context *ctx,
        struct libusb_device *dev,
        libusb_hotplug_event event,
        void * arg)
{
    if (event != LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        fprintf(stderr, "Unknown libusb_hotplug_event: %d\n", event);
        return 0;
    }

    run_usb_probe_thread_detached(dev);

    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    libusb_hotplug_callback_handle callback_handle;

    network_config_t network_config = {
        .interface = DEFAULT_INTERFACE,
        .nameserver = DEFAULT_NAMESERVER,
    };

    libusb_init(NULL);

    while ((rc = getopt (argc, argv, "hdi:n:")) != -1) {
        switch (rc) {
        case 'h':
            printf("usage: sudo %s [-h] [-i interface] [-n nameserver|\"local\" ]\n"
                    "default params: -i %s -n %s\n",
                    argv[0],
                    network_config.interface,
                    network_config.nameserver);
            return EXIT_SUCCESS;
        case 'd':
            puts("debug mode enabled");
            libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
            break;
        case 'i':
            network_config.interface = optarg;
            break;
        case 'n':
            if (!strcmp(optarg, "local")) {
                network_config.nameserver = get_system_nameserver();
            } else {
                network_config.nameserver = optarg;
            }
            break;
        case '?':
        default:
            return EXIT_FAILURE;
        }
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

