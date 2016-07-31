#ifndef _LINUX_ADK_H_
#define _LINUX_ADK_H_

#include <stdint.h>

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

/* Structures */
typedef struct {
    libusb_hotplug_callback_handle callback_handle;
    struct libusb_device_handle *handle;
    uint32_t aoa_version;
    uint16_t vid;
    uint16_t pid;
    char *manufacturer;
    char *model;
    char *description;
    char *version;
    char *url;
    char *serial;
    volatile int is_running;
    int tun_fd;
} accessory_t;

int init_accessory(accessory_t *acc);
void fini_accessory(accessory_t *acc);
bool is_accessory_present(accessory_t *acc);
accessory_t new_accessory(void);

#endif /* _LINUX_ADK_H_ */
