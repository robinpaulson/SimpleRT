#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <libusb.h>

#include "linux-adk.h"
#include "tun.h"

#define ACC_BUF_SIZE 4096
#define ACC_TIMEOUT 200

static void *tun_thread_proc(void *param)
{
    ssize_t nread;
    int ret;
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
                    /* FIXME */
                    continue;
                } else {
                    fprintf(stderr, "bulk transfer error: %s\n", libusb_strerror(ret));
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
                fprintf(stderr, "bulk transfer error: %s\n", libusb_strerror(ret));
                break;
            }
        }

        write(acc->tun_fd, acc_buf, transferred);
    }

end:
    acc->is_running = false;
    return NULL;
}

static void *connection_thread_proc(void *param)
{
    pthread_t tun_thread, accessory_thread;
    accessory_t *acc = param;

    char tun_name[IFNAMSIZ] = { 0 };
    int tun_fd = tun_alloc(tun_name);

    if (tun_fd < 0) {
        perror("tun_alloc failed");
        return NULL;
    }

    if (!iface_up(tun_name)) {
        fprintf(stderr, "Unable set iface %s up\n", tun_name);
        return NULL;
    }

    acc->tun_fd = tun_fd;
    acc->is_running = true;

    pthread_create(&tun_thread, NULL, tun_thread_proc, acc);
    pthread_create(&accessory_thread, NULL, accessory_thread_proc, acc);

    pthread_join(tun_thread, NULL);
    pthread_join(accessory_thread, NULL);

    acc->is_running = false;
    close(acc->tun_fd);
    fini_accessory(acc);

    return NULL;
}

static void on_device_connected(struct libusb_context *ctx,
        struct libusb_device *dev,
        const struct libusb_device_descriptor *desc,
        accessory_t *acc)
{
    acc->vid = desc->idVendor;
    acc->pid = desc->idProduct;

    if (!is_accessory_present(acc)) {
        if (init_accessory(acc) < 0) {
            /* init failed, possibly usb device not support accessory, it's ok */
        } else {
            /* accessory init success, wait for present */
        }
        return;
    }

    puts("accessory connected!");

    pthread_t conn_thread;
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    pthread_create(&conn_thread, &attrs, connection_thread_proc, acc);
}

int hotplug_callback(struct libusb_context *ctx,
        struct libusb_device *dev,
        libusb_hotplug_event event,
        void *user_data)
{
    struct libusb_device_descriptor desc;
    accessory_t *acc = user_data;

    libusb_get_device_descriptor(dev, &desc);

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        on_device_connected(ctx, dev, &desc, acc);
    } else {
        fprintf(stderr, "Unknown libusb_hotplug_event: %d\n", event);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int rc;
    accessory_t acc;
    libusb_hotplug_callback_handle handle;

    if (geteuid() != 0) {
        fprintf(stderr, "Run app as root!\n");
        return EXIT_FAILURE;
    }

    if (!is_tun_present()) {
        fprintf(stderr, "Tun dev is not present. Is kernel module loaded?\n");
        return EXIT_FAILURE;
    }

    libusb_init(NULL);
    /* libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG); */

    acc = new_accessory();

    rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0,
            LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            hotplug_callback, &acc, &handle);

    if (rc != LIBUSB_SUCCESS) {
        fprintf(stderr, "Error creating a hotplug callback\n");
        libusb_exit(NULL);
        return EXIT_FAILURE;
    }

    while (true) {
        libusb_handle_events_completed(NULL, NULL);
        sleep(1);
    }

    libusb_hotplug_deregister_callback(NULL, handle);
    libusb_exit(NULL);

    return EXIT_SUCCESS;
}

