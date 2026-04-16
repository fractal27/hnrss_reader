#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void ssl_init(void) {
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    SSL_library_init();
}



int connect_ipv4(const char *ip, uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        close(s);
        errno = EINVAL;
        return -1;
    }

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }
    return s; /* connected socket */
}

int connect_ipv6(const char *ip, uint16_t port) {
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    if (s < 0) return -1;

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip, &addr6.sin6_addr) != 1) {
        close(s);
        errno = EINVAL;
        return -1;
    }

    if (connect(s, (struct sockaddr *)&addr6, sizeof(addr6)) < 0) {
        close(s);
        return -1;
    }
    return s;
}

