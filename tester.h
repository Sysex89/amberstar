#ifndef AMBERSTAR_TESTER_H
#define AMBERSTAR_TESTER_H

#include <stddef.h>

void hexdump_amb_header(const char *filename, size_t header_len);
int test_open_amb(void);

#endif
