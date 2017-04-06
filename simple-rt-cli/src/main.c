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
#include <signal.h>
#include <getopt.h>
#include <libusb.h>
#include <sys/file.h>

#include "accessory.h"
#include "network.h"
#include "utils.h"

#define PID_FILE "/var/run/simple_rt.pid"

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

static bool is_instance_already_running(void)
{
    int pid_file = open(PID_FILE, O_CREAT | O_RDWR, 0666);
    int rc = flock(pid_file, LOCK_EX | LOCK_NB);

    if (rc < 0 && errno == EWOULDBLOCK) {
        /* one instance already running */
        return true;
    }

    /* do not bother of releasing descriptor */

    return false;
}

static volatile sig_atomic_t g_exit_flag = 0;

static void exit_signal_handler(int signo)
{
    g_exit_flag = 1;
    puts("");
}

int main(int argc, char *argv[])
{
    int rc = 0;
    libusb_hotplug_callback_handle callback_handle;

    simple_rt_config_t *config = get_simple_rt_config();

    libusb_init(NULL);

    signal(SIGINT, exit_signal_handler);

    while ((rc = getopt (argc, argv, "hdi:n:")) != -1) {
        switch (rc) {
        case 'h':
            printf("usage: sudo %s [-h] [-i interface] [-n nameserver|\"local\" ]\n"
                    "default params: -i %s -n %s\n",
                    argv[0],
                    config->interface,
                    config->nameserver);
            return EXIT_SUCCESS;
        case 'd':
            puts("debug mode enabled");
            libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
            break;
        case 'i':
            config->interface = optarg;
            break;
        case 'n':
            if (!strcmp(optarg, "local")) {
                config->nameserver = get_system_nameserver();
            } else {
                config->nameserver = optarg;
            }
            break;
        case '?':
        default:
            return EXIT_FAILURE;
        }
    }

    if (is_instance_already_running()) {
        fprintf(stderr, "One instance of SimpleRT is already running!\n");
        return EXIT_FAILURE;
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Run app as root!\n");
        return EXIT_FAILURE;
    }

    if (!start_network()) {
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

    while (!g_exit_flag) {
        libusb_handle_events_completed(NULL, NULL);
    }

    stop_network();

    libusb_hotplug_deregister_callback(NULL, callback_handle);
    libusb_exit(NULL);

    return EXIT_SUCCESS;
}

