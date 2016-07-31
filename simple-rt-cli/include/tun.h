#ifndef _TUN_H_
#define _TUN_H_

#include <stdbool.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define IFACE_UP_SH_PATH "./iface_up.sh"

int tun_alloc(char *dev);
bool is_tun_present(void);
bool iface_up(const char *dev);

#endif
