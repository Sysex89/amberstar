#ifndef AMBERSTAR_GFX_H
#define AMBERSTAR_GFX_H

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

#endif
