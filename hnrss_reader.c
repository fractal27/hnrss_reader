#include "hnrss_reader.h"
#include "util.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <resolv.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>

static volatile int alarm_fired = 0;
static void alarm_handler(int sig) { (void)sig; alarm_fired = 1; }

static int set_timeout(int seconds) {
    alarm_fired = 0;
    struct sigaction sa = { .sa_handler = alarm_handler, .sa_flags = 0 };
    sigaction(SIGALRM, &sa, NULL);
    alarm(seconds);
    return 0;
}

static int check_timeout(void) {
    if (alarm_fired) {
        alarm(0);
        return -1;
    }
    return 0;
}

int dns_resolve(const char *hostname, char *ipbuf, size_t ipbuf_size) {
    set_timeout(5);
    if (res_init() < 0) {
        check_timeout();
        return -1;
    }

    char buf[128];
    int response_len = res_query(hostname, ns_c_in, ns_t_any, (unsigned char*)buf, sizeof(buf));
    if (response_len < 0) {
        check_timeout();
        return -1;
    }
    if (check_timeout() < 0) return -1;

    ns_msg handle;
    if (ns_initparse((unsigned char*)buf, response_len, &handle) < 0) {
        return -1;
    }

    int ancount = ns_msg_count(handle, ns_s_an);
    if (ancount <= 0) {
        return -1;
    }

    ns_rr rr;
    if (ns_parserr(&handle, ns_s_an, 0, &rr) < 0) {
        return -1;
    }

    (void)ns_rr_name(rr);
    int type = ns_rr_type(rr);
    int rdlen = ns_rr_rdlen(rr);
    const u_char *rdata = ns_rr_rdata(rr);

    if (type == ns_t_a && rdlen == 4) {
        inet_ntop(AF_INET, rdata, ipbuf, ipbuf_size);
        return 0;
    } else if (type == ns_t_cname) {
        char cname[256];
        if (dn_expand((unsigned char*)buf, (unsigned char*)(buf + response_len), rdata, cname, sizeof(cname)) >= 0) {
            return dns_resolve(cname, ipbuf, ipbuf_size);
        }
        return -1;
    }

    return -1;
}

static int https_request_impl(const char *host, const char *path, strlist *items_out) {
    int retry_delay = 1;
    int max_retries = 5;

retry:
    int sockfd = connect_ipv4(host, 443);
    if (sockfd < 0) {
        return -1;
    }

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        close(sockfd);
        return -1;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    SSL_set_fd(ssl, sockfd);

    set_timeout(5);
    if (SSL_connect(ssl) <= 0) {
        if (check_timeout() < 0) { SSL_free(ssl); SSL_CTX_free(ctx); close(sockfd); return -1; }
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    if (check_timeout() < 0) { SSL_free(ssl); SSL_CTX_free(ctx); close(sockfd); return -1; }

    char buf_http[512];
    snprintf(buf_http, sizeof(buf_http),
             "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0\r\nConnection: close\r\n\r\n",
             path, host);
    set_timeout(5);
    if (SSL_write(ssl, buf_http, strlen(buf_http)) <= 0) {
        if (check_timeout() < 0) { SSL_free(ssl); SSL_CTX_free(ctx); close(sockfd); return -1; }
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    if (check_timeout() < 0) { SSL_free(ssl); SSL_CTX_free(ctx); close(sockfd); return -1; }

    char recv_buf[4096];
    char *response_body = malloc(65536);
    if (!response_body) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    size_t body_len = 0;
    int headers_done = 0;
    int http_status = 0;

    set_timeout(10);
    int n;
    while ((n = SSL_read(ssl, recv_buf, sizeof(recv_buf) - 1)) > 0) {
        recv_buf[n] = '\0';

        if (!headers_done) {
            char *header_end = strstr(recv_buf, "\r\n\r\n");
            if (header_end) {
                headers_done = 1;
                size_t header_len = header_end - recv_buf;
                if (header_len > 0 && header_len < sizeof(recv_buf)) {
                    recv_buf[header_len] = '\0';
                    sscanf(recv_buf, "HTTP/1.%*d %d", &http_status);
                }
                size_t body_start = (header_end - recv_buf) + 4;
                size_t chunk_size = n - body_start;
                if (body_len + chunk_size < 65536) {
                    memcpy(response_body + body_len, recv_buf + body_start, chunk_size);
                    body_len += chunk_size;
                }
            }
        } else {
            if (body_len + n < 65536) {
                memcpy(response_body + body_len, recv_buf, n);
                body_len += n;
            }
        }
    }
    if (check_timeout() < 0) {
        free(response_body);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }
    response_body[body_len] = '\0';

    if (http_status == 429 && retry_delay <= max_retries) {
        free(response_body);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        sleep(retry_delay);
        retry_delay *= 2;
        goto retry;
    }

    char *carry = NULL;
    size_t carry_len = 0;
    if (extract_items_incremental(response_body, body_len, items_out, &carry, &carry_len, 100) < 0) {
        free(response_body);
        free(carry);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return -1;
    }

    free(response_body);
    free(carry);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sockfd);
    return 0;
}

int https_request(const char *host, const char *path, strlist *items_out) {
    char ipbuf[INET_ADDRSTRLEN];
    if (dns_resolve(host, ipbuf, sizeof(ipbuf)) < 0) {
        return -1;
    }
    return https_request_impl(ipbuf, path, items_out);
}