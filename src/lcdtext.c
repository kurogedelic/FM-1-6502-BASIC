#include "lcdtext.h"
#include "lcdtext_font.inc"
int lcdtext_glyph_pixel(char c,unsigned gx,unsigned gy){
 if(c<32||c>126||gx>=5||gy>=7)return 0;
 return (font5x7[c-32][gy]>>(4-gx))&1;
}
void lcdtext_row(const char *cells,int cursor_col,uint16_t fg,uint16_t bg,uint16_t *out){
 for(unsigned col=0;col<TERM_COLS;col++){
  char c=cells[col];
  int inv=(int)col==cursor_col;
  uint16_t on=inv?bg:fg,off=inv?fg:bg;
  for(unsigned y=0;y<LCD_CELL_H;y++){
   uint16_t *px=out+y*LCD_W+col*LCD_CELL_W;
   for(unsigned x=0;x<LCD_CELL_W;x++)
    px[x]=(x<5&&y<7&&lcdtext_glyph_pixel(c,x,y))?on:off;
  }
 }
}
