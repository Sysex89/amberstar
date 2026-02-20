//reimplementation of the engine in c

#include <stdint.h>

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