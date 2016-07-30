#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "tun.h"

#define TUN_NAME "tun0"
#define TUN_DEV "/dev/" TUN_NAME

bool is_tun_present(void)
{
    return access(TUN_DEV, F_OK) == 0;
}

bool iface_up(const char *dev)
{
    char cmd[1024];

    sprintf(cmd, "ifconfig %s 10.10.10.1 10.10.10.2 up", dev);
    system(cmd);

    return true;
}

int tun_alloc(char *dev)
{
    int fd;

    if ((fd = open(TUN_DEV, O_RDWR)) < 0 ) {
        perror("error open tun");
        return -1;
    }

    strncpy(dev, TUN_NAME, IFNAMSIZ - 1);

    return fd;
}
