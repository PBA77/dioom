#ifndef PROMPT_FONT_H
#define PROMPT_FONT_H

#include <stdint.h>

static uint8_t prompt_font_glyph(char c, int row)
{
    static const uint8_t glyph_0[7] = {14, 17, 19, 21, 25, 17, 14};
    static const uint8_t glyph_1[7] = {4, 12, 4, 4, 4, 4, 14};
    static const uint8_t glyph_2[7] = {14, 17, 1, 2, 4, 8, 31};
    static const uint8_t glyph_3[7] = {30, 1, 1, 14, 1, 1, 30};
    static const uint8_t glyph_4[7] = {2, 6, 10, 18, 31, 2, 2};
    static const uint8_t glyph_5[7] = {31, 16, 16, 30, 1, 1, 30};
    static const uint8_t glyph_6[7] = {14, 16, 16, 30, 17, 17, 14};
    static const uint8_t glyph_7[7] = {31, 1, 2, 4, 8, 8, 8};
    static const uint8_t glyph_8[7] = {14, 17, 17, 14, 17, 17, 14};
    static const uint8_t glyph_9[7] = {14, 17, 17, 15, 1, 1, 14};
    static const uint8_t glyph_a[7] = {14, 17, 17, 31, 17, 17, 17};
    static const uint8_t glyph_b[7] = {30, 17, 17, 30, 17, 17, 30};
    static const uint8_t glyph_c[7] = {14, 17, 16, 16, 16, 17, 14};
    static const uint8_t glyph_d[7] = {30, 17, 17, 17, 17, 17, 30};
    static const uint8_t glyph_e[7] = {31, 16, 16, 30, 16, 16, 31};
    static const uint8_t glyph_f[7] = {31, 16, 16, 30, 16, 16, 16};
    static const uint8_t glyph_g[7] = {14, 17, 16, 23, 17, 17, 14};
    static const uint8_t glyph_h[7] = {17, 17, 17, 31, 17, 17, 17};
    static const uint8_t glyph_i[7] = {14, 4, 4, 4, 4, 4, 14};
    static const uint8_t glyph_j[7] = {7, 2, 2, 2, 2, 18, 12};
    static const uint8_t glyph_k[7] = {17, 18, 20, 24, 20, 18, 17};
    static const uint8_t glyph_l[7] = {16, 16, 16, 16, 16, 16, 31};
    static const uint8_t glyph_m[7] = {17, 27, 21, 21, 17, 17, 17};
    static const uint8_t glyph_n[7] = {17, 25, 21, 19, 17, 17, 17};
    static const uint8_t glyph_o[7] = {14, 17, 17, 17, 17, 17, 14};
    static const uint8_t glyph_p[7] = {30, 17, 17, 30, 16, 16, 16};
    static const uint8_t glyph_q[7] = {14, 17, 17, 17, 21, 18, 13};
    static const uint8_t glyph_r[7] = {30, 17, 17, 30, 20, 18, 17};
    static const uint8_t glyph_s[7] = {15, 16, 16, 14, 1, 1, 30};
    static const uint8_t glyph_t[7] = {31, 4, 4, 4, 4, 4, 4};
    static const uint8_t glyph_u[7] = {17, 17, 17, 17, 17, 17, 14};
    static const uint8_t glyph_v[7] = {17, 17, 17, 17, 17, 10, 4};
    static const uint8_t glyph_w[7] = {17, 17, 17, 21, 21, 21, 10};
    static const uint8_t glyph_x[7] = {17, 17, 10, 4, 10, 17, 17};
    static const uint8_t glyph_y[7] = {17, 17, 10, 4, 4, 4, 4};
    static const uint8_t glyph_z[7] = {31, 1, 2, 4, 8, 16, 31};
    static const uint8_t glyph_dot[7] = {0, 0, 0, 0, 0, 12, 12};
    static const uint8_t glyph_slash[7] = {1, 1, 2, 4, 8, 16, 16};
    static const uint8_t glyph_dash[7] = {0, 0, 0, 31, 0, 0, 0};
    static const uint8_t glyph_colon[7] = {0, 12, 12, 0, 12, 12, 0};
    static const uint8_t glyph_percent[7] = {24, 25, 2, 4, 8, 19, 3};
    static const uint8_t glyph_bang[7] = {4, 4, 4, 4, 4, 0, 4};

    switch (c) {
    case '0': return glyph_0[row];
    case '1': return glyph_1[row];
    case '2': return glyph_2[row];
    case '3': return glyph_3[row];
    case '4': return glyph_4[row];
    case '5': return glyph_5[row];
    case '6': return glyph_6[row];
    case '7': return glyph_7[row];
    case '8': return glyph_8[row];
    case '9': return glyph_9[row];
    case 'A': return glyph_a[row];
    case 'B': return glyph_b[row];
    case 'C': return glyph_c[row];
    case 'D': return glyph_d[row];
    case 'E': return glyph_e[row];
    case 'F': return glyph_f[row];
    case 'G': return glyph_g[row];
    case 'H': return glyph_h[row];
    case 'I': return glyph_i[row];
    case 'J': return glyph_j[row];
    case 'K': return glyph_k[row];
    case 'L': return glyph_l[row];
    case 'M': return glyph_m[row];
    case 'N': return glyph_n[row];
    case 'O': return glyph_o[row];
    case 'P': return glyph_p[row];
    case 'Q': return glyph_q[row];
    case 'R': return glyph_r[row];
    case 'S': return glyph_s[row];
    case 'T': return glyph_t[row];
    case 'U': return glyph_u[row];
    case 'V': return glyph_v[row];
    case 'W': return glyph_w[row];
    case 'X': return glyph_x[row];
    case 'Y': return glyph_y[row];
    case 'Z': return glyph_z[row];
    case '.': return glyph_dot[row];
    case '/': return glyph_slash[row];
    case '-': return glyph_dash[row];
    case ':': return glyph_colon[row];
    case '%': return glyph_percent[row];
    case '!': return glyph_bang[row];
    default: return 0;
    }
}

#endif
