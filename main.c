#include <stdio.h>
#include <stdlib.h>

#include "hnrss_reader.h"
#include "util.h"
#include "rsslib/rsslib.h"

#define FLAG_IMPLEMENTATION
#include "flag.h"

int main(int argc, char **argv) {
    size_t *max_items = flag_size("n", 10, "Maximum number of items to display");
    bool *verbose = flag_bool("v", false, "Enable verbose output");
    bool *show_help = flag_bool("h", false, "Show this help message");

    if (!flag_parse(argc, argv)) {
        flag_print_error(stderr);
        fprintf(stderr, "\nUsage: %s [options] [host]\n\nOptions:\n", argv[0]);
        flag_print_options(stderr);
        return 1;
    }

    if (*show_help) {
        fprintf(stderr, "Usage: %s [options] [host]\n\nOptions:\n", argv[0]);
        flag_print_options(stderr);
        return 0;
    }

    int rest_argc = flag_rest_argc();
    char **rest_argv = flag_rest_argv();

    const char *host = "hnrss.org";
    if (rest_argc > 0) {
        host = rest_argv[0];
    }

    if (*verbose) {
        fprintf(stderr, "Fetching RSS feed from %s\n", host);
    }

    ssl_init();
    strlist items;
    sl_init(&items);

    if (https_request(host, "/newest", &items) < 0) {
        fprintf(stderr, "Failed to fetch RSS feed\n");
        sl_free(&items);
        return 1;
    }

    if (*verbose) {
        fprintf(stderr, "Found %zu items\n", items.count);
    }

    const char *filter_tags[] = { "title", "link" };
    strlist filtered;
    strlist *texts_by_tag = NULL;
    if (filter_items_by_children(&items, filter_tags, 2, &filtered, &texts_by_tag) < 0) {
        fprintf(stderr, "Failed to filter items\n");
        sl_free(&items);
        return 1;
    }

    size_t display_count = (*max_items < filtered.count) ? *max_items : filtered.count;
    for (size_t i = 0; i < display_count; ++i) {
        printf("%s | %s\n", texts_by_tag[0].items[i], texts_by_tag[1].items[i]);
    }

    if (*verbose && filtered.count > display_count) {
        fprintf(stderr, "... and %zu more items\n", filtered.count - display_count);
    }

    sl_free(&items);
    sl_free(&filtered);
    sl_array_free(texts_by_tag, 2);
    return 0;
}
