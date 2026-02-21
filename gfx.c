#include "gfx.h"
#include <SDL3/SDL_stdinc.h>

static t_xentry X_conv_tab[320];
static uint16_t Y_conv_tab[200];

void init_gfx(void)
{
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 16; j++) {
            X_conv_tab[i * 16 + j].rsa = i * 8;
            X_conv_tab[i * 16 + j].pixel = j;
        }
    }
    for (int i = 0; i < 200; i++)
        Y_conv_tab[i] = i * 160;
}

t_xentry coord_convert(uint16_t x, uint16_t y)
{
    t_xentry out;
    uint16_t y_offset = Y_conv_tab[y];
    out.rsa = y_offset + X_conv_tab[x].rsa;
    out.pixel = X_conv_tab[x].pixel;
    return out;
}

static const uint16_t StartEnd_tab[32] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
    0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff
};

static const uint16_t Colour_tab[16][4] = {
    {0x0000, 0x0000, 0x0000, 0x0000}, {0xffff, 0x0000, 0x0000, 0x0000},
    {0x0000, 0xffff, 0x0000, 0x0000}, {0xffff, 0xffff, 0x0000, 0x0000},
    {0x0000, 0x0000, 0xffff, 0x0000}, {0xffff, 0x0000, 0xffff, 0x0000},
    {0x0000, 0xffff, 0xffff, 0x0000}, {0xffff, 0xffff, 0xffff, 0x0000},
    {0x0000, 0x0000, 0x0000, 0xffff}, {0xffff, 0x0000, 0x0000, 0xffff},
    {0x0000, 0xffff, 0x0000, 0xffff}, {0xffff, 0xffff, 0x0000, 0xffff},
    {0x0000, 0x0000, 0xffff, 0xffff}, {0xffff, 0x0000, 0xffff, 0xffff},
    {0x0000, 0xffff, 0xffff, 0xffff}, {0xffff, 0xffff, 0xffff, 0xffff}
};

static void write_plane(uint16_t *screen_ptr, uint16_t mask_set, uint16_t mask_clear, uint16_t color_pattern)
{
    *screen_ptr |= (mask_set & color_pattern);
    screen_ptr++;
    *screen_ptr &= ~(mask_clear & ~color_pattern);
}

void draw_box(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
              uint16_t color, uint8_t *screen_base)
{
    uint16_t num_lines = y2 - y1;
    if (num_lines == 0) return;

    uint8_t *screen_ptr = screen_base + (y1 * 160);
    uint16_t start_pixel = x1 & 0x0F;
    uint16_t end_pixel = x2 & 0x0F;
    uint16_t start_col = x1 & 0xFFF0;
    uint16_t end_col = x2 & 0xFFF0;
    uint16_t start_mask = StartEnd_tab[start_pixel];
    uint16_t end_mask = StartEnd_tab[16 + end_pixel];
    uint16_t start_mask_inv = ~start_mask;
    uint16_t end_mask_inv = ~end_mask;

    screen_ptr += (start_col >> 1);

    const uint16_t *color_patterns = Colour_tab[color & 0x0F];
    uint16_t color_plane0 = color_patterns[0];
    uint16_t color_plane1 = color_patterns[1];
    uint16_t color_plane2 = color_patterns[2];
    uint16_t color_plane3 = color_patterns[3];
    uint16_t width_cols = ((end_col - start_col) >> 4);

    if (width_cols == 0) {
        uint16_t combined_mask = start_mask & end_mask;
        uint16_t combined_mask_inv = start_mask_inv | end_mask_inv;
        uint8_t *line_ptr = screen_ptr;
        for (uint16_t line = 0; line <= num_lines; line++) {
            uint16_t *p = (uint16_t *)line_ptr;
            p[0] |= (combined_mask & color_plane0);
            p[0] &= ~(combined_mask_inv & ~color_plane0);
            p[1] |= (combined_mask & color_plane1);
            p[1] &= ~(combined_mask_inv & ~color_plane1);
            p[2] |= (combined_mask & color_plane2);
            p[2] &= ~(combined_mask_inv & ~color_plane2);
            p[3] |= (combined_mask & color_plane3);
            p[3] &= ~(combined_mask_inv & ~color_plane3);
            line_ptr += 160;
        }
    } else {
        uint8_t *line_ptr = screen_ptr;
        for (uint16_t line = 0; line <= num_lines; line++) {
            uint16_t *p = (uint16_t *)line_ptr;
            p[0] |= (start_mask & color_plane0);
            p[1] |= (start_mask & color_plane1);
            p[2] &= ~(start_mask_inv & ~color_plane2);
            p[3] &= ~(start_mask_inv & ~color_plane3);
            p += 4;
            for (uint16_t col = 1; col < width_cols; col++) {
                p[0] = color_plane0;
                p[1] = color_plane1;
                p[2] = color_plane2;
                p[3] = color_plane3;
                p += 4;
            }
            p[0] |= (end_mask & color_plane0);
            p[1] |= (end_mask & color_plane1);
            p[2] &= ~(end_mask_inv & ~color_plane2);
            p[3] &= ~(end_mask_inv & ~color_plane3);
            line_ptr += 160;
        }
    }
}

bool duplicate_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                     SDL_Surface *src_screen, SDL_Surface *dst_screen)
{
    if (!src_screen || !dst_screen) return false;
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    SDL_Rect srcrect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = width_pixels,
        .h = height_pixels
    };
    SDL_Rect dstrect = { .x = srcrect.x, .y = srcrect.y, .w = srcrect.w, .h = srcrect.h };
    return SDL_BlitSurface(src_screen, &srcrect, dst_screen, &dstrect);
}

bool get2_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                SDL_Surface *screen, void *buffer, size_t buffer_pitch)
{
    if (!screen || !buffer) return false;
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    SDL_Rect rect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = width_pixels,
        .h = height_pixels
    };
    if (SDL_MUSTLOCK(screen)) {
        if (SDL_LockSurface(screen) != 0) return false;
    }
    int src_pitch = screen->pitch;
    uint8_t *src_pixels = (uint8_t *)screen->pixels;
    uint8_t *dst_pixels = (uint8_t *)buffer;
    for (int row = 0; row < rect.h; row++) {
        uint8_t *src_row = src_pixels + (rect.y + row) * src_pitch + rect.x * screen->format;
        uint8_t *dst_row = dst_pixels + row * buffer_pitch;
        SDL_memcpy(dst_row, src_row, rect.w * screen->format);
    }
    if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);
    return true;
}

bool put_unmasked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen)
{
    if (!block_surface || !screen) return false;
    t_xentry coord = coord_convert(x, y);
    SDL_Rect dstrect = { .x = coord.rsa % 160, .y = coord.rsa / 160, .w = 16, .h = 16 };
    if (block_surface->w != 16 || block_surface->h != 16) return false;
    return SDL_BlitSurface(block_surface, NULL, screen, &dstrect);
}

bool put_masked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen)
{
    if (!block_surface || !screen) return false;
    t_xentry coord = coord_convert(x, y);
    SDL_Rect dstrect = { .x = coord.rsa % 160, .y = coord.rsa / 160, .w = 16, .h = 16 };
    if (block_surface->w != 16 || block_surface->h != 16) return false;
    SDL_BlendMode old_blend;
    SDL_GetSurfaceBlendMode(block_surface, &old_blend);
    SDL_SetSurfaceBlendMode(block_surface, SDL_BLENDMODE_BLEND);
    bool result = SDL_BlitSurface(block_surface, NULL, screen, &dstrect);
    SDL_SetSurfaceBlendMode(block_surface, old_blend);
    return result;
}

bool blit_unmasked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen)
{
    return put_unmasked_block(x, y, block_surface, screen);
}

bool blit_masked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen)
{
    return put_masked_block(x, y, block_surface, screen);
}

bool blot_unmasked_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                         SDL_Surface *block_surface, SDL_Surface *screen)
{
    if (!block_surface || !screen) return false;
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    SDL_Rect srcrect = { .x = 0, .y = 0, .w = width_pixels, .h = height_pixels };
    SDL_Rect dstrect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = width_pixels,
        .h = height_pixels
    };
    if (block_surface->w < width_pixels || block_surface->h < height_pixels) return false;
    return SDL_BlitSurface(block_surface, &srcrect, screen, &dstrect);
}

bool blot_masked_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                       SDL_Surface *block_surface, SDL_Surface *screen)
{
    if (!block_surface || !screen) return false;
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    SDL_Rect srcrect = { .x = 0, .y = 0, .w = width_pixels, .h = height_pixels };
    SDL_Rect dstrect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = width_pixels,
        .h = height_pixels
    };
    if (block_surface->w < width_pixels || block_surface->h < height_pixels) return false;
    SDL_BlendMode old_blend;
    SDL_GetSurfaceBlendMode(block_surface, &old_blend);
    SDL_SetSurfaceBlendMode(block_surface, SDL_BLENDMODE_BLEND);
    bool result = SDL_BlitSurface(block_surface, &srcrect, screen, &dstrect);
    SDL_SetSurfaceBlendMode(block_surface, old_blend);
    return result;
}
