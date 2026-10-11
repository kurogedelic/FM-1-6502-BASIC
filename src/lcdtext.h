#ifndef FM_BASIC_LCDTEXT_H
#define FM_BASIC_LCDTEXT_H
#include <stdint.h>
#include "terminal.h"
/* Monochrome text for the FM-1's 240x240 LCD: 6x8 cells (5x7 glyph + spacing),
 * 40 columns x 30 rows. Rows 0..23 show the Terminal, rows 24..29 are free
 * for a status line. Pure C, no hardware access: the firmware sends the pixel
 * rows this produces to the panel, and the host tests compare them with text. */
#define LCD_W 240
#define LCD_CELL_W 6
#define LCD_CELL_H 8
#define LCD_TEXT_ROWS 30
/* Render one text row (TERM_COLS characters) into LCD_CELL_H * LCD_W pixels.
 * `fg`/`bg` are stored as given (the firmware passes byte-swapped RGB565).
 * `cursor_col` >= 0 draws that cell inverted. */
void lcdtext_row(const char *cells,int cursor_col,uint16_t fg,uint16_t bg,uint16_t *out);
/* Does glyph pixel (gx 0..4, gy 0..6) of ASCII `c` light up? (exposed for tests) */
int lcdtext_glyph_pixel(char c,unsigned gx,unsigned gy);
#endif
