#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "rsslib.h"

void sl_init(strlist *l) { l->items = NULL; l->count = 0; l->cap = 0; }

void sl_free(strlist *l) {
    if (!l) return;
    for (size_t i = 0; i < l->count; ++i) free(l->items[i]);
    free(l->items);
    l->items = NULL; l->count = 0; l->cap = 0;
}

int sl_add(strlist *l, const char *s, size_t len) {
    if (l->count == l->cap) {
        size_t nc = l->cap ? l->cap * 2 : 8;
        char **p = realloc(l->items, nc * sizeof(*p));
        if (!p) return -1;
        l->items = p; l->cap = nc;
    }
    char *copy = malloc(len + 1);
    if (!copy) return -1;
    memcpy(copy, s, len); copy[len] = '\0';
    l->items[l->count++] = copy;
    return 0;
}

int sl_count(const strlist *l) {
    return (int)l->count;
}

int extract_items_incremental(const char *buf, size_t len, strlist *out, char **carry, size_t *carry_len, size_t n) {
    size_t new_len = *carry_len + len;
    char *working = realloc(*carry, new_len + 1);
    if (!working) return -1;
    memcpy(working + *carry_len, buf, len);
    working[new_len] = '\0';
    *carry = working;
    *carry_len = new_len;

    const char *p = working;
    const char *end = working + new_len;
    const char *start_tag = "<item";
    const char *close_tag = "</item>";
    while (out->count < n) {
        const char *st = strstr(p, start_tag);
        if (!st) break;
        const char *gt = strchr(st, '>');
        if (!gt) break;
        const char *content_start = gt + 1;
        const char *close = strstr(content_start, close_tag);
        if (!close) break;
        const char *item_end = close + strlen(close_tag);
        size_t item_len = item_end - st;
        if (sl_add(out, st, item_len) < 0) return -1;
        p = item_end;
    }

    size_t leftover = end - p;
    if (leftover && p != working) memmove(working, p, leftover);
    *carry_len = leftover;
    working = realloc(working, leftover + 1);
    if (!working && leftover) return -1;
    if (working) working[leftover] = '\0';
    *carry = working;
    return 0;
}

char *find_child_text(const char *item, const char *childtag) {
    size_t taglen = strlen(childtag);
    size_t patlen = taglen + 1;
    char *pat = malloc(patlen + 1);
    if (!pat) return NULL;
    sprintf(pat, "<%s", childtag);

    char *start = strstr(item, pat);
    free(pat);
    if (!start) return NULL;
    char *gt = strchr(start, '>');
    if (!gt) return NULL;
    const char *content = gt + 1;

    size_t endpatlen = taglen + 3;
    char *endpat = malloc(endpatlen + 1);
    if (!endpat) return NULL;
    sprintf(endpat, "</%s>", childtag);
    char *end = strstr(content, endpat);
    free(endpat);
    if (!end) return NULL;

    size_t len = end - content;
    const char *cdata_prefix = "<![CDATA[";
    size_t cdata_prefix_len = strlen(cdata_prefix);
    const char *cdata_suffix = "]]>";
    size_t cdata_suffix_len = strlen(cdata_suffix);

    if (len >= cdata_prefix_len + cdata_suffix_len &&
        strncmp(content, cdata_prefix, cdata_prefix_len) == 0 &&
        strncmp(end - cdata_suffix_len, cdata_suffix, cdata_suffix_len) == 0) {
        content += cdata_prefix_len;
        len -= cdata_prefix_len + cdata_suffix_len;
    }

    char *out = malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, content, len);
    out[len] = '\0';
    return out;
}

int sl_array_init(strlist **arr, size_t n) {
    *arr = calloc(n, sizeof(strlist));
    if (!*arr) return -1;
    for (size_t i = 0; i < n; ++i) sl_init(&(*arr)[i]);
    return 0;
}

void sl_array_free(strlist *arr, size_t n) {
    if (!arr) return;
    for (size_t i = 0; i < n; ++i) sl_free(&arr[i]);
    free(arr);
}

int filter_items_by_children(const strlist *items, const char **tags, size_t ntags,
                              strlist *out_items, strlist **out_texts_matrix) {
    sl_init(out_items);
    if (sl_array_init(out_texts_matrix, ntags) < 0) return -1;

    for (size_t i = 0; i < items->count; ++i) {
        char **found_texts = calloc(ntags, sizeof(char*));
        if (!found_texts) { sl_free(out_items); sl_array_free(*out_texts_matrix, ntags); return -1; }

        int ok = 1;
        for (size_t t = 0; t < ntags; ++t) {
            found_texts[t] = find_child_text(items->items[i], tags[t]);
            if (!found_texts[t]) { ok = 0; break; }
        }

        if (ok) {
            if (sl_add(out_items, items->items[i], strlen(items->items[i])) < 0) {
                ok = 0;
            } else {
                for (size_t t = 0; t < ntags; ++t) {
                    if (sl_add(&(*out_texts_matrix)[t], found_texts[t], strlen(found_texts[t])) < 0) {
                        ok = 0;
                        break;
                    }
                }
            }
        }

        for (size_t t = 0; t < ntags; ++t) free(found_texts[t]);
        free(found_texts);
    }
    return 0;
}