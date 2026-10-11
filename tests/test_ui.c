/* Host tests for the user-interface pieces that the FM-1 firmware reuses:
 * terminal line endings, the on-device keyboard map, and the LCD text renderer. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/keyboard.h"
#include "../src/lcdtext.h"
#include "../src/terminal.h"

/* Every character a typical BASIC program needs must be typeable without a computer. */
static int reachable(char want){
 Keyboard k;
 for(int layer=0;layer<2;layer++)for(int shift=0;shift<2;shift++){
  keyboard_init(&k);k.layer=layer?KEY_SYMBOLS:KEY_ALPHA;k.shift=(uint8_t)shift;
  for(unsigned n=0;n<27;n++)if(keyboard_note(&k,n)==want)return 1;
 }
 keyboard_init(&k);
 for(unsigned p=0;p<14;p++){keyboard_init(&k);if(keyboard_panel(&k,p)==(unsigned char)want)return 1;}
 return 0;
}

static void test_keyboard(void){
 const char *needed="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \"$()+-*/^=<>,.:;?\r\b";
 for(const char *c=needed;*c;c++){if(!reachable(*c)){fprintf(stderr,"cannot type 0x%02X (%c)\n",(unsigned)*c,*c);assert(0);}}
 assert(reachable(3));  /* Ctrl-C / BREAK */
 Keyboard k;keyboard_init(&k);
 assert(keyboard_note(&k,0)=='Q');assert(keyboard_note(&k,25)=='M');assert(keyboard_note(&k,26)==' ');
 assert(keyboard_panel(&k,7)==-1);assert(keyboard_note(&k,0)=='1');assert(keyboard_note(&k,10)=='"');
 assert(keyboard_note(&k,27)==-1);
}

static void test_terminal(void){
 Terminal t;term_init(&t);
 term_write(&t,"AB\r\nCD");
 assert(t.y==1&&t.x==2&&!memcmp(t.cells[0],"AB",2)&&!memcmp(t.cells[1],"CD",2)); /* CR LF is one newline */
 term_put(&t,'\r');term_put(&t,'\r');assert(t.y==3);                               /* two CRs are two newlines */
 term_init(&t);
 for(int i=0;i<TERM_COLS;i++)term_put(&t,'X');
 term_put(&t,'\r');term_put(&t,'\n');
 assert(t.y==1);                                                                   /* 40 chars + CR LF: no blank line */
 term_init(&t);
 for(int i=0;i<TERM_COLS+3;i++)term_put(&t,'Y');
 assert(t.y==1&&t.x==3);                                                           /* wraps on the next character */
 term_init(&t);term_write(&t,"AB\b");assert(t.x==1&&t.cells[0][1]==' ');
 term_init(&t);for(int i=0;i<TERM_ROWS+2;i++){term_write(&t,"L");term_put(&t,'\r');term_put(&t,'\n');}
 assert(t.y==TERM_ROWS-1);                                                         /* scrolls */
}

static void dump_glyph(char c,char art[7][6]){
 for(unsigned y=0;y<7;y++){for(unsigned x=0;x<5;x++)art[y][x]=lcdtext_glyph_pixel(c,x,y)?'#':'.';art[y][5]=0;}
}

static void test_lcd(void){
 char art[7][6];
 dump_glyph('H',art);
 assert(!strcmp(art[0],"#...#")&&!strcmp(art[3],"#####")&&!strcmp(art[6],"#...#"));
 dump_glyph('2',art);
 assert(!strcmp(art[0],".###.")&&!strcmp(art[6],"#####"));
 dump_glyph('"',art);
 assert(!strcmp(art[0],".#.#.")&&!strcmp(art[3],"....."));
 /* every printable glyph is non-empty except space, and none is drawn outside 5x7 */
 for(char c=33;c<127;c++){int n=0;for(unsigned y=0;y<7;y++)for(unsigned x=0;x<5;x++)n+=lcdtext_glyph_pixel(c,x,y);assert(n>0);}
 assert(lcdtext_glyph_pixel(' ',2,3)==0);assert(lcdtext_glyph_pixel(1,0,0)==0);
 /* all glyphs are distinct */
 for(char a=32;a<127;a++)for(char b=(char)(a+1);b<127;b++){
  int same=1;for(unsigned y=0;y<7&&same;y++)for(unsigned x=0;x<5;x++)if(lcdtext_glyph_pixel(a,x,y)!=lcdtext_glyph_pixel(b,x,y)){same=0;break;}
  if(same){fprintf(stderr,"glyphs %c and %c are identical\n",a,b);assert(0);}
 }
 /* row rendering */
 static uint16_t px[LCD_CELL_H*LCD_W];
 char cells[TERM_COLS];memset(cells,' ',sizeof cells);cells[0]='H';cells[2]='I';
 lcdtext_row(cells,2,0xFFFF,0x0000,px);
 assert(px[0]==0xFFFF&&px[1]==0&&px[4]==0xFFFF);          /* H, top row: #...# */
 assert(px[5]==0);                                       /* spacing column */
 assert(px[7*LCD_W]==0);                                  /* spacing row */
 assert(px[12]==0xFFFF&&px[13]==0&&px[16]==0xFFFF);
 assert(px[7*LCD_W+12]==0xFFFF);                          /* cursor block covers the spacing row too */
 assert(px[6*LCD_W+239]==0);                              /* untouched blank cell */
/* a snapshot of "FM-1" proves the font, cell layout and row renderer end to end */
 memset(cells,' ',sizeof cells);memcpy(cells,"FM-1",4);
 lcdtext_row(cells,-1,1,0,px);
 static const char *const want[7]={
  "#####.#...#.........#...",
  "#.....##.##........##...",
  "#.....#.#.#.........#...",
  "####..#.#.#.#####...#...",
  "#.....#...#.........#...",
  "#.....#...#.........#...",
  "#.....#...#........###.."};
 for(unsigned y=0;y<8;y++){
  char line[25];
  for(unsigned x=0;x<24;x++)line[x]=px[y*LCD_W+x]?'#':'.';
  line[24]=0;
  if(y<7){if(strcmp(line,want[y])){fprintf(stderr,"row %u: %s != %s\n",y,line,want[y]);assert(0);}}
  else assert(!strcmp(line,"........................"));
 }
}

int main(void){
 test_keyboard();test_terminal();test_lcd();
 puts("ui tests: PASS");
 return 0;
}
