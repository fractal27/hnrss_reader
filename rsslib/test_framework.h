#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
#define OK_STRING "\e[42m OK \e[0m "
#define TEST_ASSERT(condition, message) do { \
    tests_run++; \
    if (condition) { \
		printf(OK_STRING "condition " #condition "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s\e[0m\n", __FILE__, __LINE__, message); \
    } \
} while(0)

#define TEST_ASSERT_EQUAL(expected, actual, message) do { \
    tests_run++; \
    if ((expected) == (actual)) { \
		printf(OK_STRING "equals " #expected " == " #actual "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s (expected %d, got %d)\e[0m\n", __FILE__, __LINE__, message, (int)(expected), (int)(actual)); \
    } \
} while(0)

#define TEST_ASSERT_MEM_EQUAL(expected, actual, size, message) do { \
    tests_run++; \
    if (memcmp((expected), (actual), (size)) == 0) { \
		printf(OK_STRING "mem_equal " #expected " == " #actual "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s\e[0m\n", __FILE__, __LINE__, message); \
    } \
} while(0)

#define TEST_ASSERT_STR_EQUAL(expected, actual, message) do { \
    tests_run++; \
    if ((expected) && (actual) && strcmp((expected), (actual)) == 0) { \
		printf(OK_STRING "str_equals " #expected " == " #actual "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s (expected '%s', got '%s')\e[0m\n", __FILE__, __LINE__, message, (expected) ? (expected) : "NULL", (actual) ? (actual) : "NULL"); \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr, message) do { \
    tests_run++; \
    if ((ptr) == NULL) { \
		printf(OK_STRING "null " #ptr "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s (expected NULL, got non-NULL)\e[0m\n", __FILE__, __LINE__, message); \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr, message) do { \
    tests_run++; \
    if ((ptr) != NULL) { \
		printf(OK_STRING "not_null " #ptr "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s (expected non-NULL)\e[0m\n", __FILE__, __LINE__, message); \
    } \
} while(0)

// #define TEST_ASSERT_TRUE(value, message) TEST_ASSERT_EQUAL(1, (value), message)
// #define TEST_ASSERT_FALSE(value, message) TEST_ASSERT_EQUAL(0, (value), message)

#define TEST_ASSERT_TRUE(value, message) do { \
    tests_run++; \
    if (1 == (value)) { \
		printf(OK_STRING "assert_true " #value "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s (expected true(1), got %d)\e[0m\n", __FILE__, __LINE__, message, (int)(value)); \
    } \
} while(0)

#define TEST_ASSERT_FALSE(value, message) do { \
    tests_run++; \
    if (0 == (value)) { \
		printf(OK_STRING "assert_false " #value "\n"); \
        tests_passed++; \
    } else { \
        printf("\e[31mFAIL: %s:%d: %s (expected true(1), got %d)\e[0m\n", __FILE__, __LINE__, (message), (int)(value)); \
    } \
} while(0)


#define RUN_TEST(test_func) do { \
    printf("\e[2m- Running %s...\e[0m\n", #test_func); \
    test_func(); \
} while(0)

#define RUN_TEST_GROUP(group_name, tests) do { \
    printf("\n## %s\n", group_name); \
    tests; \
} while(0)

#endif
