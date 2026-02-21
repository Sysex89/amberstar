#ifndef AMBERSTAR_TESTER_H
#define AMBERSTAR_TESTER_H

#include <stddef.h>
#include <stdio.h>

/* Terminal colors (no-op if not a TTY; use TEST_GFX when no terminal) */
#define TERM_RED   "\033[31m"
#define TERM_GRN   "\033[32m"
#define TERM_RST   "\033[0m"

/** Prints PASS (green) or FAIL (red) with file:line and reason. Use for normal terminal tests. */
#define TEST(cond, reason) do { \
    if (cond) { \
        fprintf(stderr, TERM_GRN "PASS" TERM_RST " %s:%d: " reason "\n", __FILE__, __LINE__); \
    } else { \
        fprintf(stderr, TERM_RED "FAIL" TERM_RST " %s:%d: " reason "\n", __FILE__, __LINE__); \
    } \
} while (0)

/** Same as TEST but no ANSI codes; use for gfx/headless tests where terminal colors are unavailable. */
#define TEST_GFX(cond, reason) do { \
    if (cond) { \
        fprintf(stderr, "PASS %s:%d: " reason "\n", __FILE__, __LINE__); \
    } else { \
        fprintf(stderr, "FAIL %s:%d: " reason "\n", __FILE__, __LINE__); \
    } \
} while (0)

void hexdump_amb_header(const char *filename, size_t header_len);
int test_open_amb(void);
void test_memory(void);
#endif
