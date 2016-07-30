#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "tun.h"

static const char tundev[] = "/dev/tun0";

bool is_tun_present(void)
{
    return access(tundev, F_OK) == 0;
}

int tun_alloc(char *dev)
{
    int fd;

    if ((fd = open(tundev, O_RDWR)) < 0 ) {
        perror("error open tun");
        return -1;
    }

    return fd;
}
