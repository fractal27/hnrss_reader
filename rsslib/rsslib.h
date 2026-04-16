#ifndef RSSLIB_H
#define RSSLIB_H

#include <stddef.h>

typedef struct {
    char **items;
    size_t count;
    size_t cap;
} strlist;

void sl_init(strlist *l);
void sl_free(strlist *l);
int sl_add(strlist *l, const char *s, size_t len);
int sl_count(const strlist *l);

int extract_items_incremental(const char *buf, size_t len, strlist *out, char **carry, size_t *carry_len, size_t n);

char *find_child_text(const char *item, const char *childtag);

int sl_array_init(strlist **arr, size_t n);
void sl_array_free(strlist *arr, size_t n);

int filter_items_by_children(const strlist *items, const char **tags, size_t ntags,
                             strlist *out_items, strlist **out_texts_matrix);

#endif
