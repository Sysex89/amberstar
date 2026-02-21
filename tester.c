#include "amberstar.h"
#include "tester.h"
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_endian.h>
#include <stdint.h>
#include <stdio.h>

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
    if (!r.ptr || r.len < 6) {
        if (r.ptr)
            SDL_free(r.ptr);
        return AMB_ERR_OPEN_OR_SIZE;
    }
    const uint8_t *p = (const uint8_t *)r.ptr;
    if (!amb_is_ambr(p) && !amb_is_ampc(p)) {
        SDL_free(r.ptr);
        return AMB_ERR_BAD_MAGIC;
    }
    if (!amb_is_ambr(p)) {
        SDL_free(r.ptr);
        return AMB_ERR_NOT_AMBR;
    }
    uint16_t n = SDL_Swap16BE(*(const uint16_t *)(p + 4));
    (void)n;
    SDL_free(r.ptr);

    r = Load_file(data_root, ampc_file);
    if (!r.ptr || r.len < 6) {
        if (r.ptr)
            SDL_free(r.ptr);
        return AMB_ERR_OPEN_AMPC;
    }
    p = (const uint8_t *)r.ptr;
    if (!amb_is_ampc(p)) {
        SDL_free(r.ptr);
        return AMB_ERR_NOT_AMPC;
    }
    SDL_free(r.ptr);
    return AMB_OK;
}
