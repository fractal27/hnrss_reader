#ifndef HNRSS_READER_H
#define HNRSS_READER_H

#include "rsslib/rsslib.h"

int https_request(const char *host, const char *path, strlist *items_out);

#endif
