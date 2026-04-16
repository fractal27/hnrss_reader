#ifndef _RSSP_UTIL_H
#define _RSSP_UTIL_H
#include <stdint.h>


void ssl_init(void);
int connect_ipv4(const char *ip, uint16_t port);
int connect_ipv6(const char *ip, uint16_t port);



#endif // _RSSP_UTIL_H
