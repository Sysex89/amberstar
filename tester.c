#include "amberstar.h"
#include "memory.h"
#include "tester.h"
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_endian.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_FREE    (16u * 1024u * 1024u)
#define MEMLIST_ENTRY   12
#define MAX_MEMBLOCKS   1000

static uint32_t read_be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static uint16_t read_be16(const uint8_t *p)
{
    return (uint16_t)p[0] << 8 | p[1];
}

static size_t memory_list_count(void)
{
    return (size_t)((const uint8_t *)Memlist_end - (const uint8_t *)Memory_list) / MEMLIST_ENTRY;
}

static void hexdump_bytes(const uint8_t *p, size_t len)
{
    for (size_t i = 0; i < len; i += 16) {
        printf("%04zx  ", i);
        for (size_t j = 0; j < 16 && i + j < len; j++)
            printf("%02x ", p[i + j]);
        printf("\n");
    }
}

void hexdump_amb_header(const char *filename, size_t header_len)
{
    t_file_res r = Load_file(data_root, filename);
    if (!r.ptr || r.len < 6) {
        if (r.ptr)
            SDL_free(r.ptr);
        printf("hexdump: could not load %s\n", filename);
        return;
    }
    size_t n = (size_t)header_len;
    if (n > r.len)
        n = r.len;
    printf("--- %s (first %zu bytes) ---\n", filename, n);
    hexdump_bytes((const uint8_t *)r.ptr, n);
    SDL_free(r.ptr);
}

int test_open_amb(void)
{
    /* AUTOMAP.AMB is AMBR (unpacked); PICS80.AMB is AMPC (packed). */
    const char *ambr_file = "AUTOMAP.AMB";
    const char *ampc_file = "PICS80.AMB";

    t_file_res r = Load_file(data_root, ambr_file);
    TEST(r.ptr && r.len >= 6, "load AMBR file");
    if (!r.ptr || r.len < 6) {
        if (r.ptr)
            SDL_free(r.ptr);
        return AMB_ERR_OPEN_OR_SIZE;
    }
    const uint8_t *p = (const uint8_t *)r.ptr;
    TEST(amb_is_ambr(p) || amb_is_ampc(p), "AMBR/AMPC magic");
    if (!amb_is_ambr(p) && !amb_is_ampc(p)) {
        SDL_free(r.ptr);
        return AMB_ERR_BAD_MAGIC;
    }
    TEST(amb_is_ambr(p), "file is AMBR");
    if (!amb_is_ambr(p)) {
        SDL_free(r.ptr);
        return AMB_ERR_NOT_AMBR;
    }
    uint16_t n = SDL_Swap16BE(*(const uint16_t *)(p + 4));
    (void)n;
    SDL_free(r.ptr);

    r = Load_file(data_root, ampc_file);
    TEST(r.ptr && r.len >= 6, "load AMPC file");
    if (!r.ptr || r.len < 6) {
        if (r.ptr)
            SDL_free(r.ptr);
        return AMB_ERR_OPEN_AMPC;
    }
    p = (const uint8_t *)r.ptr;
    TEST(amb_is_ampc(p), "file is AMPC");
    if (!amb_is_ampc(p)) {
        SDL_free(r.ptr);
        return AMB_ERR_NOT_AMPC;
    }
    SDL_free(r.ptr);
    return AMB_OK;
}

void test_memory(void)
{
    void *p;
    uint32_t f;
    size_t n;
    const uint8_t *list;

    Init_memory(NULL);

    TEST(Size_of_free_memory == INITIAL_FREE, "Init_memory sets Size_of_free_memory to INITIAL_FREE");

    TEST(Memlist_end == Memory_list, "Init_memory leaves Memlist_end at start (no entries)");

    p = Allocate_memory(1);
    TEST(p != NULL, "Allocate_memory(1) returns non-null");
    if (p) {
        n = memory_list_count();
        list = (const uint8_t *)Memory_list;
        TEST(n >= 1 && read_be32(list + 4) == 2, "size aligned to even (1 -> 2)");
        *(unsigned char *)p = 0xab;
        TEST(*(unsigned char *)p == 0xab, "allocated pointer is writable");
        Kill_memory(p);
    }

    Init_memory(NULL);
    p = Allocate_memory(INITIAL_FREE);
    TEST(p != NULL, "drain free for next test");
    if (p) {
        void *q = Allocate_memory(1);
        TEST(q == NULL, "Allocate_memory returns NULL when size > Size_of_free_memory");
        Kill_memory(p);
    }

    Init_memory(NULL);
    f = Size_of_free_memory;
    p = Allocate_memory(100);
    TEST(p != NULL && Size_of_free_memory == f - 100, "Allocate_memory decrements Size_of_free_memory by aligned size");
    if (p) Kill_memory(p);

    Init_memory(NULL);
    p = Allocate_memory(10);
    TEST(p != NULL, "alloc for list check");
    if (p) {
        n = memory_list_count();
        list = (const uint8_t *)Memory_list;
        TEST(n == 1 && (list[8] & 0x7f) == 0 && read_be16(list + 10) == 0, "Allocate_memory entry has type 0, subfile 0");
        Kill_memory(p);
    }

    Init_memory(NULL);
    p = File_allocate(3, 100, 5);
    TEST(p != NULL && Reallocate_memory(3, 5) == p, "File_allocate stores type/subfile; Reallocate_memory returns block");
    if (p) Kill_memory(p);

    Init_memory(NULL);
    TEST(Reallocate_memory(99, 999) == NULL, "Reallocate_memory returns NULL for unknown (type, subfile)");

    Init_memory(NULL);
    p = Allocate_memory(INITIAL_FREE);
    TEST(p != NULL, "drain free for File_allocate NULL test");
    if (p) {
        void *q = File_allocate(1, 1, 0);
        TEST(q == NULL, "File_allocate returns NULL when size > Size_of_free_memory");
        Kill_memory(p);
    }

    Init_memory(NULL);
    p = Allocate_memory(1000);
    TEST(p != NULL, "alloc for Kill test");
    if (p) {
        f = Size_of_free_memory;
        Kill_memory(p);
        TEST(Size_of_free_memory == f + 1000, "Kill_memory increases Size_of_free_memory by block size");
    }

    Init_memory(NULL);
    p = Allocate_memory(8);
    n = memory_list_count();
    Kill_memory(p);
    TEST(n == 1 && memory_list_count() == 0, "Kill_memory removes entry from Memory_list");

    Init_memory(NULL);
    f = Size_of_free_memory;
    Kill_memory((void *)(uintptr_t)0x1234);
    TEST(Size_of_free_memory == f, "Kill_memory invalid pointer is no-op");

    Init_memory(NULL);
    p = Allocate_memory(50);
    f = Size_of_free_memory;
    Free_memory(p);
    TEST(Size_of_free_memory == f + 50, "Free_memory frees like Kill_memory");

    Init_memory(NULL);
    p = Allocate_memory(100);
    TEST(p != NULL, "alloc for Shrink test");
    if (p) {
        f = Size_of_free_memory;
        Shrink_memory(40, p);
        TEST(Size_of_free_memory == f + 60, "Shrink_memory adds freed amount to Size_of_free_memory");
        Kill_memory(p);
    }

    Init_memory(NULL);
    p = Allocate_memory(20);
    TEST(p != NULL, "alloc for Shrink no-op test");
    if (p) {
        f = Size_of_free_memory;
        Shrink_memory(20, p);
        TEST(Size_of_free_memory == f, "Shrink_memory same size no-op");
        Shrink_memory(100, p);
        TEST(Size_of_free_memory == f, "Shrink_memory larger size no-op");
        Kill_memory(p);
    }

    Init_memory(NULL);
    p = Allocate_memory(100);
    TEST(p != NULL, "alloc for Shrink align test");
    if (p) {
        Shrink_memory(3, p);
        n = memory_list_count();
        list = (const uint8_t *)Memory_list;
        TEST(n == 1 && read_be32(list + 4) == 4, "Shrink_memory new size aligned (3 -> 4)");
        Kill_memory(p);
    }

    Init_memory(NULL);
    f = Size_of_free_memory;
    Shrink_memory(10, (void *)(uintptr_t)0xdead);
    TEST(Size_of_free_memory == f, "Shrink_memory invalid pointer is no-op");

    Init_memory(NULL);
    {
        void *ptrs[MAX_MEMBLOCKS];
        for (n = 0; n < MAX_MEMBLOCKS; n++) {
            ptrs[n] = Allocate_memory(1);
            if (!ptrs[n]) break;
        }
        TEST(n == MAX_MEMBLOCKS, "MAX_MEMBLOCKS allocations succeed");
        p = Allocate_memory(1);
        TEST(p == NULL, "Allocate_memory returns NULL when slots exhausted");
        for (n = 0; n < MAX_MEMBLOCKS; n++)
            Kill_memory(ptrs[n]);
    }

    Init_memory(NULL);
    p = Allocate_memory(8);
    TEST(p != NULL, "alloc for list format test");
    if (p) {
        n = memory_list_count();
        list = (const uint8_t *)Memory_list;
        TEST(n == 1 && (const uint8_t *)Memlist_end == list + MEMLIST_ENTRY, "one entry; Memlist_end past it");
        TEST((list[8] & 0x80) == 0, "type has bit 7 clear in list");
        Kill_memory(p);
    }
}

void test_data_loading(void) {}

void test_gfx(void)
{
    /* Use TEST_GFX here: no terminal colors (e.g. headless or after gfx init). */
    TEST_GFX(1, "placeholder");
}

