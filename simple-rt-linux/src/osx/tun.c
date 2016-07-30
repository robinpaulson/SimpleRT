#include <unistd.h>
#include "tun.h"

static const char tundev[] = "/dev/tun0";

bool is_tun_present(void)
{
    return access(tundev, F_OK) == 0;
}

int tun_alloc(char *dev)
{
    return 0;
}
