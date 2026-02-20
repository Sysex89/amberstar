//reimplementation of the engine in c

#include <SDL3/SDL_endian.h>
#include <stdint.h>
#include <SDL3/SDL.h>

typedef struct XEntry {
    uint16_t rsa;
    uint16_t pixel;
} t_xentry;

static t_xentry X_conv_tab[320];
static uint16_t Y_conv_tab[200];

void init_gfx(){

    //init x
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 16; j++) {
            X_conv_tab[i * 16 + j].rsa = i * 8;      // RSA = column * 8 bytes
            X_conv_tab[i * 16 + j].pixel = j;         // Pixel = 0-15 within column
        }
    }
    
    //init y
    for (int i = 0; i < 200; i++) {
        Y_conv_tab[i] = i * 160;                      // Byte offset = row * 160 bytes per scanline
    }
}

t_xentry coord_convert(uint16_t x, uint16_t y){
    t_xentry out;
    
    // Convert Y-coordinate: get byte offset for this row
    // Assembly: add.w d1,d1 (multiply by 2 for word indexing)
    //           move.w 0(a0,d1.w),d2 (load Y_conv_tab[Y])
    uint16_t y_offset = Y_conv_tab[y];
    
    // Convert X-coordinate: get column RSA and pixel number
    // Assembly: lsl.w #2,d0 (multiply by 4 for byte indexing, 4 bytes per entry)
    //           add.w 0(a0,d0.w),d2 (add X_conv_tab[X].rsa to Y offset)
    //           move.w 2(a0,d0.w),d3 (load X_conv_tab[X].pixel)
    out.rsa = y_offset + X_conv_tab[x].rsa;    // Final RSA = row offset + column offset
    out.pixel = X_conv_tab[x].pixel;            // Pixel number within the column
    
    return out;
}

// Lookup tables for box drawing
static const uint16_t StartEnd_tab[32] = {
    // Start masks (left edge, pixels 0-15)
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
    // End masks (right edge, pixels 0-15)
    0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
    0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff
};

static const uint16_t Colour_tab[16][4] = {
    {0x0000, 0x0000, 0x0000, 0x0000},  // Color 0
    {0xffff, 0x0000, 0x0000, 0x0000},  // Color 1
    {0x0000, 0xffff, 0x0000, 0x0000},  // Color 2
    {0xffff, 0xffff, 0x0000, 0x0000},  // Color 3
    {0x0000, 0x0000, 0xffff, 0x0000},  // Color 4
    {0xffff, 0x0000, 0xffff, 0x0000},  // Color 5
    {0x0000, 0xffff, 0xffff, 0x0000},  // Color 6
    {0xffff, 0xffff, 0xffff, 0x0000},  // Color 7
    {0x0000, 0x0000, 0x0000, 0xffff}, // Color 8
    {0xffff, 0x0000, 0x0000, 0xffff}, // Color 9
    {0x0000, 0xffff, 0x0000, 0xffff}, // Color 10
    {0xffff, 0xffff, 0x0000, 0xffff}, // Color 11
    {0x0000, 0x0000, 0xffff, 0xffff}, // Color 12
    {0xffff, 0x0000, 0xffff, 0xffff}, // Color 13
    {0x0000, 0xffff, 0xffff, 0xffff}, // Color 14
    {0xffff, 0xffff, 0xffff, 0xffff}  // Color 15
};

// Helper function to write to planar screen memory
static void write_plane(uint16_t *screen_ptr, uint16_t mask_set, uint16_t mask_clear, uint16_t color_pattern) {
    // Set bits: OR with mask_set
    *screen_ptr |= (mask_set & color_pattern);
    screen_ptr++;
    // Clear bits: AND with inverted mask_clear
    *screen_ptr &= ~(mask_clear & ~color_pattern);
    screen_ptr++;
}

void draw_box(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, 
              uint16_t color, uint8_t *screen_base) {
    // Calculate number of lines
    uint16_t num_lines = y2 - y1;
    if (num_lines == 0) return;
    
    // Calculate screen address for start of box (Y coordinate)
    uint8_t *screen_ptr = screen_base + (y1 * 160);
    
    // Get pixel positions within 16-pixel columns (low nibbles)
    uint16_t start_pixel = x1 & 0x0F;  // Pixel position in start column (0-15)
    uint16_t end_pixel = x2 & 0x0F;    // Pixel position in end column (0-15)
    
    // Align to column boundaries
    uint16_t start_col = x1 & 0xFFF0;  // Clear low nibble
    uint16_t end_col = x2 & 0xFFF0;    // Clear low nibble
    
    // Get masks for partial pixels at edges
    uint16_t start_mask = StartEnd_tab[start_pixel];
    uint16_t end_mask = StartEnd_tab[16 + end_pixel];
    uint16_t start_mask_inv = ~start_mask;
    uint16_t end_mask_inv = ~end_mask;
    
    // Calculate byte offset for X coordinate (divide by 2, then add to screen_ptr)
    screen_ptr += (start_col >> 1);
    
    // Get color patterns for 4 bitplanes
    const uint16_t *color_patterns = Colour_tab[color & 0x0F];
    uint16_t color_plane0 = color_patterns[0];
    uint16_t color_plane1 = color_patterns[1];
    uint16_t color_plane2 = color_patterns[2];
    uint16_t color_plane3 = color_patterns[3];
    
    // Calculate width in columns (truncs)
    uint16_t width_cols = ((end_col - start_col) >> 4);  // Number of 16-pixel columns
    
    // Check if box is only one column wide
    if (width_cols == 0) {
        // Single column case: combine masks
        uint16_t combined_mask = start_mask & end_mask;
        uint16_t combined_mask_inv = start_mask_inv | end_mask_inv;
        
        // Draw each scanline
        uint8_t *line_ptr = screen_ptr;
        for (uint16_t line = 0; line <= num_lines; line++) {
            uint16_t *p = (uint16_t *)line_ptr;
            
            // Atari ST planar format: each column is 8 bytes (4 planes × 2 bytes)
            // Apply mask to each plane
            p[0] |= (combined_mask & color_plane0);
            p[0] &= ~(combined_mask_inv & ~color_plane0);
            
            p[1] |= (combined_mask & color_plane1);
            p[1] &= ~(combined_mask_inv & ~color_plane1);
            
            p[2] |= (combined_mask & color_plane2);
            p[2] &= ~(combined_mask_inv & ~color_plane2);
            
            p[3] |= (combined_mask & color_plane3);
            p[3] &= ~(combined_mask_inv & ~color_plane3);
            
            line_ptr += 160;  // Next scanline (160 bytes per line)
        }
    } else {
        // Multiple columns: draw start, middle, end
        uint8_t *line_ptr = screen_ptr;
        
        for (uint16_t line = 0; line <= num_lines; line++) {
            uint16_t *p = (uint16_t *)line_ptr;
            
            // Draw start partial column (first trunc) - 4 planes, 2 bytes each
            p[0] |= (start_mask & color_plane0);
            p[1] |= (start_mask & color_plane1);
            p[2] &= ~(start_mask_inv & ~color_plane2);
            p[3] &= ~(start_mask_inv & ~color_plane3);
            p += 4;  // Move to next column (8 bytes = 4 words)
            
            // Draw middle full columns (if any) - unrolled for speed
            for (uint16_t col = 1; col < width_cols; col++) {
                // Full column: just set all bits
                p[0] = color_plane0;
                p[1] = color_plane1;
                p[2] = color_plane2;
                p[3] = color_plane3;
                p += 4;  // Next column
            }
            
            // Draw end partial column (last trunc)
            p[0] |= (end_mask & color_plane0);
            p[1] |= (end_mask & color_plane1);
            p[2] &= ~(end_mask_inv & ~color_plane2);
            p[3] &= ~(end_mask_inv & ~color_plane3);
            
            line_ptr += 160;  // Next scanline
        }
    }
}

// SDL3 Block Function Implementations
// These map the Atari ST planar graphics block operations to SDL3 surfaces

// [ Copy any-sized block from one screen to another ]
// Equivalent to: Duplicate_block
// Copies a rectangular region from source screen to destination screen at same position
bool duplicate_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                     SDL_Surface *src_screen, SDL_Surface *dst_screen) {
    if (!src_screen || !dst_screen) return false;
    
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    
    SDL_Rect srcrect = {
        .x = coord.rsa % 160,  // X byte offset within scanline
        .y = coord.rsa / 160,  // Y scanline
        .w = width_pixels,
        .h = height_pixels
    };
    
    SDL_Rect dstrect = {
        .x = srcrect.x,
        .y = srcrect.y,
        .w = srcrect.w,
        .h = srcrect.h
    };
    
    return SDL_BlitSurface(src_screen, &srcrect, dst_screen, &dstrect);
}

// [ Any size block-get routine ]
// Equivalent to: Get2_block
// Reads a rectangular region from screen into a buffer
bool get2_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                 SDL_Surface *screen, void *buffer, size_t buffer_pitch) {
    if (!screen || !buffer) return false;
    
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    
    SDL_Rect rect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = width_pixels,
        .h = height_pixels
    };
    
    // Lock surface to read pixels
    if (SDL_MUSTLOCK(screen)) {
        if (SDL_LockSurface(screen) != 0) return false;
    }
    
    // Calculate source pitch
    int src_pitch = screen->pitch;
    uint8_t *src_pixels = (uint8_t *)screen->pixels;
    uint8_t *dst_pixels = (uint8_t *)buffer;
    
    // Copy row by row
    for (int row = 0; row < rect.h; row++) {
        uint8_t *src_row = src_pixels + (rect.y + row) * src_pitch + rect.x * screen->format;
        uint8_t *dst_row = dst_pixels + row * buffer_pitch;
        SDL_memcpy(dst_row, src_row, rect.w * screen->format);
    }
    
    if (SDL_MUSTLOCK(screen)) {
        SDL_UnlockSurface(screen);
    }
    
    return true;
}

// [ 16x16 unmasked block-put routine ]
// Equivalent to: Put_unmasked_block
// Draws a 16x16 opaque sprite from buffer to screen
bool put_unmasked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen) {
    if (!block_surface || !screen) return false;
    
    t_xentry coord = coord_convert(x, y);
    
    SDL_Rect dstrect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = 16,
        .h = 16
    };
    
    // Ensure block surface is 16x16
    if (block_surface->w != 16 || block_surface->h != 16) {
        return false;
    }
    
    return SDL_BlitSurface(block_surface, NULL, screen, &dstrect);
}

// [ 16x16 masked block-put routine ]
// Equivalent to: Put_masked_block
// Draws a 16x16 sprite with transparency from buffer to screen
bool put_masked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen) {
    if (!block_surface || !screen) return false;
    
    t_xentry coord = coord_convert(x, y);
    
    SDL_Rect dstrect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = 16,
        .h = 16
    };
    
    // Ensure block surface is 16x16
    if (block_surface->w != 16 || block_surface->h != 16) {
        return false;
    }
    
    // Set blend mode for transparency (if using alpha)
    SDL_BlendMode old_blend;
    SDL_GetSurfaceBlendMode(block_surface, &old_blend);
    SDL_SetSurfaceBlendMode(block_surface, SDL_BLENDMODE_BLEND);
    
    bool result = SDL_BlitSurface(block_surface, NULL, screen, &dstrect);
    
    SDL_SetSurfaceBlendMode(block_surface, old_blend);
    
    return result;
}

// [ 16x16 any coordinate unmasked block-put routine ]
// Equivalent to: Blit_unmasked_block
// Draws a 16x16 opaque sprite at any pixel position (handles sub-column alignment)
bool blit_unmasked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen) {
    // For SDL3, we can use the same as Put_unmasked_block since SDL handles sub-pixel positioning
    // The original assembly used bit rotation for sub-column alignment, but SDL handles this automatically
    return put_unmasked_block(x, y, block_surface, screen);
}

// [ 16x16 any coordinate masked block-put routine ]
// Equivalent to: Blit_masked_block
// Draws a 16x16 sprite with transparency at any pixel position
bool blit_masked_block(uint16_t x, uint16_t y, SDL_Surface *block_surface, SDL_Surface *screen) {
    // Same as Blit_unmasked_block but with transparency
    return put_masked_block(x, y, block_surface, screen);
}

// [ Any size unmasked block-put routine ]
// Equivalent to: Blot_unmasked_block
// Draws an arbitrary-sized opaque rectangle from buffer to screen
bool blot_unmasked_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                         SDL_Surface *block_surface, SDL_Surface *screen) {
    if (!block_surface || !screen) return false;
    
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    
    SDL_Rect srcrect = {
        .x = 0,
        .y = 0,
        .w = width_pixels,
        .h = height_pixels
    };
    
    SDL_Rect dstrect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = width_pixels,
        .h = height_pixels
    };
    
    // Ensure block surface is large enough
    if (block_surface->w < width_pixels || block_surface->h < height_pixels) {
        return false;
    }
    
    return SDL_BlitSurface(block_surface, &srcrect, screen, &dstrect);
}

// [ Any size masked block-put routine ]
// Equivalent to: Blot_masked_block
// Draws an arbitrary-sized rectangle with transparency from buffer to screen
bool blot_masked_block(uint16_t x, uint16_t y, uint16_t width_cols, uint16_t height_pixels,
                       SDL_Surface *block_surface, SDL_Surface *screen) {
    if (!block_surface || !screen) return false;
    
    t_xentry coord = coord_convert(x, y);
    uint16_t width_pixels = width_cols * 16;
    
    SDL_Rect srcrect = {
        .x = 0,
        .y = 0,
        .w = width_pixels,
        .h = height_pixels
    };
    
    SDL_Rect dstrect = {
        .x = coord.rsa % 160,
        .y = coord.rsa / 160,
        .w = width_pixels,
        .h = height_pixels
    };
    
    // Ensure block surface is large enough
    if (block_surface->w < width_pixels || block_surface->h < height_pixels) {
        return false;
    }
    
    // Set blend mode for transparency
    SDL_BlendMode old_blend;
    SDL_GetSurfaceBlendMode(block_surface, &old_blend);
    SDL_SetSurfaceBlendMode(block_surface, SDL_BLENDMODE_BLEND);
    
    bool result = SDL_BlitSurface(block_surface, &srcrect, screen, &dstrect);
    
    SDL_SetSurfaceBlendMode(block_surface, old_blend);
    
    return result;
}


const char *amberfile_path = "/AMBRFILE/";
//SDL_Swap16BE(x);// we should use this when reading
typedef struct amberfile_header {
    uint8_t magic[4];   /* "AMBR" or "AMPC" */
    uint16_t sfilenm;
    uint16_t padding;
} t_amberfile_header;