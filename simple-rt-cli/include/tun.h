#ifndef _TUN_H_
#define _TUN_H_

#include <stdbool.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

int tun_alloc(char *dev);
bool is_tun_present(void);
bool iface_up(const char *dev);

#endif
