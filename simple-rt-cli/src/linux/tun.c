#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include "tun.h"

static const char clonedev[] = "/dev/net/tun";
static const char script_path[] = "./iface_up.sh";

bool is_tun_present(void)
{
    return access(clonedev, F_OK) == 0;
}

bool iface_up(const char *dev)
{
    char cmd[1024];

    sprintf(cmd, "%s %s %s\n", script_path, "linux", dev);
    return system(cmd) == 0;
}

int tun_alloc(char *dev)
{
    int fd;
    int err;
    struct ifreq ifr;

    if ((fd = open(clonedev, O_RDWR)) < 0 ) {
        perror("error open tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        perror("error create tun");
        return err;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}
