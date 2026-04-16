#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rsslib.h"
#include "test_framework.h"

int tests_run = 0;
int tests_passed = 0;

void test_sl_init_free(void) {
    strlist l;
    sl_init(&l);
    TEST_ASSERT(l.items == NULL, "sl_init: items NULL");
    TEST_ASSERT(l.count == 0, "sl_init: count 0");
    TEST_ASSERT(l.cap == 0, "sl_init: cap 0");
    sl_free(&l);
}

void test_sl_add(void) {
    strlist l;
    sl_init(&l);

    TEST_ASSERT_EQUAL(0, sl_add(&l, "hello", 5), "sl_add: first item");
    TEST_ASSERT_EQUAL(1, l.count, "sl_add: count 1");
    TEST_ASSERT_STR_EQUAL("hello", l.items[0], "sl_add: content match");

    TEST_ASSERT_EQUAL(0, sl_add(&l, "world", 5), "sl_add: second item");
    TEST_ASSERT_EQUAL(2, l.count, "sl_add: count 2");

    sl_free(&l);
}

void test_sl_add_growth(void) {
    strlist l;
    sl_init(&l);

    for (int i = 0; i < 20; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "item%d", i);
        TEST_ASSERT_EQUAL(0, sl_add(&l, buf, strlen(buf)), "sl_add: growth test");
    }
    TEST_ASSERT_EQUAL(20, l.count, "sl_add: growth count");

    sl_free(&l);
}

void test_sl_add_empty_string(void) {
    strlist l;
    sl_init(&l);

    TEST_ASSERT_EQUAL(0, sl_add(&l, "", 0), "sl_add: empty string");
    TEST_ASSERT_EQUAL(1, l.count, "sl_add: empty string count");
    TEST_ASSERT_STR_EQUAL("", l.items[0], "sl_add: empty string content");

    sl_free(&l);
}

void test_find_child_text(void) {
    char *result;

    result = find_child_text("<item><title>TestTitle</title></item>", "title");
    TEST_ASSERT_NOT_NULL(result, "find_child_text: found");
    if (result) {
        TEST_ASSERT_STR_EQUAL("TestTitle", result, "find_child_text: value match");
        free(result);
    }

    result = find_child_text("<item><title>TestTitle</title></item>", "nonexistent");
    TEST_ASSERT_NULL(result, "find_child_text: not found returns NULL");

    result = find_child_text("<item><link>http://example.com</link></item>", "link");
    TEST_ASSERT_NOT_NULL(result, "find_child_text: link tag");
    if (result) {
        TEST_ASSERT_STR_EQUAL("http://example.com", result, "find_child_text: link value");
        free(result);
    }
}

void test_find_child_text_with_attributes(void) {
    char *result = find_child_text("<item><a href='url'>text</a></item>", "a");
    TEST_ASSERT_NOT_NULL(result, "find_child_text: with attributes");
    if (result) {
        TEST_ASSERT_STR_EQUAL("text", result, "find_child_text: attribute content");
        free(result);
    }
}

void test_find_child_text_nested(void) {
    char *result = find_child_text("<item><desc><p>Nested</p></desc></item>", "desc");
    TEST_ASSERT_NOT_NULL(result, "find_child_text: nested");
    if (result) {
        TEST_ASSERT_STR_EQUAL("<p>Nested</p>", result, "find_child_text: nested content");
        free(result);
    }
}

void test_extract_items_incremental(void) {
    const char *xml = "<root><item><title>One</title></item><item><title>Two</title></item></root>";
    strlist items;
    char *carry = NULL;
    size_t carry_len = 0;

    sl_init(&items);
    int res = extract_items_incremental(xml, strlen(xml), &items, &carry, &carry_len, 10);
    TEST_ASSERT_EQUAL(0, res, "extract_items_incremental: success");
    TEST_ASSERT_EQUAL(2, items.count, "extract_items_incremental: 2 items");

    sl_free(&items);
    free(carry);
}

void test_extract_items_chunked(void) {
    const char *xml = "<root><item><title>One</title></item></root>";
    size_t chunk = 10;
    strlist items;
    char *carry = NULL;
    size_t carry_len = 0;

    sl_init(&items);
    size_t pos = 0;
    while (pos < strlen(xml)) {
        size_t take = (strlen(xml) - pos) < chunk ? (strlen(xml) - pos) : chunk;
        extract_items_incremental(xml + pos, take, &items, &carry, &carry_len, 10);
        pos += take;
    }

    TEST_ASSERT_EQUAL(1, items.count, "extract_items_chunked: 1 item");
    sl_free(&items);
    free(carry);
}

void test_extract_items_single_byte_chunks(void) {
    const char *xml = "<root><item><title>One</title></item></root>";
    strlist items;
    char *carry = NULL;
    size_t carry_len = 0;

    sl_init(&items);
    for (size_t pos = 0; pos < strlen(xml); pos++) {
        extract_items_incremental(xml + pos, 1, &items, &carry, &carry_len, 10);
    }

    TEST_ASSERT_EQUAL(1, items.count, "extract_items_single_byte: 1 item");
    sl_free(&items);
    free(carry);
}

void test_extract_items_max_n(void) {
    const char *xml = "<root><item><title>One</title></item><item><title>Two</title></item><item><title>Three</title></item></root>";
    strlist items;
    char *carry = NULL;
    size_t carry_len = 0;

    sl_init(&items);
    extract_items_incremental(xml, strlen(xml), &items, &carry, &carry_len, 2);
    TEST_ASSERT_EQUAL(2, items.count, "extract_items_max_n: limit to 2");

    sl_free(&items);
    free(carry);
}

void test_extract_items_partial_tag(void) {
    const char *part1 = "<root><item><title>One</title></item><ite";
    const char *part2 = "m><title>Two</title></item></root>";
    strlist items;
    char *carry = NULL;
    size_t carry_len = 0;

    sl_init(&items);
    extract_items_incremental(part1, strlen(part1), &items, &carry, &carry_len, 10);
    extract_items_incremental(part2, strlen(part2), &items, &carry, &carry_len, 10);

    TEST_ASSERT_EQUAL(2, items.count, "extract_items_partial: 2 items after split");
    sl_free(&items);
    free(carry);
}

void test_filter_items_by_children(void) {
    strlist items, filtered;
    strlist *texts = NULL;
    const char *tags[] = { "title", "id", "cat" };

    sl_init(&items);
    sl_add(&items, "<item><title>One</title><id>1</id><cat>A</cat></item>", strlen("<item><title>One</title><id>1</id><cat>A</cat></item>"));
    sl_add(&items, "<item><title>Two</title><id>2</id></item>", strlen("<item><title>Two</title><id>2</id></item>"));
    sl_add(&items, "<item><title>Three</title><id>3</id><cat>A</cat></item>", strlen("<item><title>Three</title><id>3</id><cat>A</cat></item>"));

    int res = filter_items_by_children(&items, tags, 3, &filtered, &texts);
    TEST_ASSERT_EQUAL(0, res, "filter_items_by_children: success");
    TEST_ASSERT_EQUAL(2, filtered.count, "filter_items_by_children: 2 filtered");

    sl_free(&items);
    sl_free(&filtered);
    sl_array_free(texts, 3);
}

void test_filter_empty_items(void) {
    strlist items, filtered;
    strlist *texts = NULL;
    const char *tags[] = { "title" };

    sl_init(&items);
    int res = filter_items_by_children(&items, tags, 1, &filtered, &texts);
    TEST_ASSERT_EQUAL(0, res, "filter_empty_items: success");
    TEST_ASSERT_EQUAL(0, filtered.count, "filter_empty_items: zero count");

    sl_free(&items);
    sl_free(&filtered);
    sl_array_free(texts, 1);
}

void test_filter_all_match(void) {
    strlist items, filtered;
    strlist *texts = NULL;
    const char *tags[] = { "title" };

    sl_init(&items);
    sl_add(&items, "<item><title>One</title></item>", strlen("<item><title>One</title></item>"));
    sl_add(&items, "<item><title>Two</title></item>", strlen("<item><title>Two</title></item>"));

    int res = filter_items_by_children(&items, tags, 1, &filtered, &texts);
    TEST_ASSERT_EQUAL(0, res, "filter_all_match: success");
    TEST_ASSERT_EQUAL(2, filtered.count, "filter_all_match: all 2 match");

    sl_free(&items);
    sl_free(&filtered);
    sl_array_free(texts, 1);
}

void test_filter_none_match(void) {
    strlist items, filtered;
    strlist *texts = NULL;
    const char *tags[] = { "title", "nonexistent" };

    sl_init(&items);
    sl_add(&items, "<item><title>One</title></item>", strlen("<item><title>One</title></item>"));

    int res = filter_items_by_children(&items, tags, 2, &filtered, &texts);
    TEST_ASSERT_EQUAL(0, res, "filter_none_match: success");
    TEST_ASSERT_EQUAL(0, filtered.count, "filter_none_match: none match");

    sl_free(&items);
    sl_free(&filtered);
    sl_array_free(texts, 2);
}

void test_sl_array_init_free(void) {
    strlist *arr = NULL;
    int res = sl_array_init(&arr, 5);
    TEST_ASSERT_EQUAL(0, res, "sl_array_init: success");
    TEST_ASSERT_NOT_NULL(arr, "sl_array_init: non-null");

    if (arr) {
        for (size_t i = 0; i < 5; i++) {
            TEST_ASSERT_EQUAL(0, arr[i].count, "sl_array_init: empty lists");
        }
        sl_array_free(arr, 5);
    }
}

int main(void) {
    printf("Running tests...\n\n");

    RUN_TEST_GROUP("strlist", {
        RUN_TEST(test_sl_init_free);
        RUN_TEST(test_sl_add);
        RUN_TEST(test_sl_add_growth);
        RUN_TEST(test_sl_add_empty_string);
        RUN_TEST(test_sl_array_init_free);
    });

    RUN_TEST_GROUP("find_child_text", {
        RUN_TEST(test_find_child_text);
        RUN_TEST(test_find_child_text_with_attributes);
        RUN_TEST(test_find_child_text_nested);
    });

    RUN_TEST_GROUP("extract_items", {
        RUN_TEST(test_extract_items_incremental);
        RUN_TEST(test_extract_items_chunked);
        RUN_TEST(test_extract_items_single_byte_chunks);
        RUN_TEST(test_extract_items_max_n);
        RUN_TEST(test_extract_items_partial_tag);
    });

    RUN_TEST_GROUP("filter_items", {
        RUN_TEST(test_filter_items_by_children);
        RUN_TEST(test_filter_empty_items);
        RUN_TEST(test_filter_all_match);
        RUN_TEST(test_filter_none_match);
    });

    printf("\nResults: %d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed != tests_run ? 1 : 0;
}
