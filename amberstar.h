#ifndef AMBERSTAR_AMBERSTAR_H
#define AMBERSTAR_AMBERSTAR_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct XEntry {
    uint16_t rsa;
    uint16_t pixel;
} t_xentry;

void init_gfx(void);
t_xentry coord_convert(uint16_t x, uint16_t y);

void draw_box(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
              uint16_t color, uint8_t *screen_base);

bool duplicate_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                     SDL_Surface *src_screen, SDL_Surface *dst_screen);
bool get2_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                SDL_Surface *screen, void *buffer, size_t buffer_pitch);
bool put_unmasked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen);
bool put_masked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen);
bool blit_unmasked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen);
bool blit_masked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen);
bool blot_unmasked_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                         SDL_Surface *block_surface, SDL_Surface *screen);
bool blot_masked_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                       SDL_Surface *block_surface, SDL_Surface *screen);

extern const char *data_root;

typedef struct amberfile_header {
    uint8_t magic[4];
    uint16_t num_subfiles;
} t_amberfile_header;

#define AMB_MAGIC_AMBR 0x414D4252u
#define AMB_MAGIC_AMPC 0x414D5043u

static inline int amb_is_ambr(const uint8_t *magic4) {
    return magic4[0] == 'A' && magic4[1] == 'M' && magic4[2] == 'B' && magic4[3] == 'R';
}
static inline int amb_is_ampc(const uint8_t *magic4) {
    return magic4[0] == 'A' && magic4[1] == 'M' && magic4[2] == 'P' && magic4[3] == 'C';
}

char *data_path(const char *data_root, const char *filename);

typedef enum {
    AMB_OK = 0,
    AMB_ERR_OPEN_OR_SIZE = -1,
    AMB_ERR_BAD_MAGIC     = -2,
    AMB_ERR_NOT_AMBR      = -3,
    AMB_ERR_OPEN_AMPC     = -4,
    AMB_ERR_NOT_AMPC      = -5,
} amb_error_t;

const char *amb_error_str(int code);

typedef struct file_res {
    void *ptr;
    uint32_t len;
} t_file_res;

typedef t_file_res t_subfile_res;

t_file_res Load_file(const char *data_root, const char *filename);
t_subfile_res Load_subfile(const char *data_root, const char *filename, int subfile_idx);

#endif
