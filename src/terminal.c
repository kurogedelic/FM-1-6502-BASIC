#include "terminal.h"
#include <string.h>
void term_init(Terminal *t) { memset(t,0,sizeof(*t)); for(int y=0;y<TERM_ROWS;y++) memset(t->cells[y],' ',TERM_COLS); }
static void newline(Terminal *t) { t->x=0; if (++t->y>=TERM_ROWS) { memmove(t->cells[0],t->cells[1],(TERM_ROWS-1)*TERM_COLS); memset(t->cells[TERM_ROWS-1],' ',TERM_COLS); t->y=TERM_ROWS-1; } }
/* Line endings: CR is a newline; an LF directly after a CR belongs to the same
 * CR LF and is ignored. Wrapping at column 40 is deferred until the next
 * printable character (x == TERM_COLS means "wrap pending"), so a line of
 * exactly 40 characters followed by CR LF does not leave a blank line. */
void term_put(Terminal *t,char c) {
 if(c=='\r'){newline(t);t->last_cr=1;return;}
 if(c=='\n'){if(!t->last_cr)newline(t);t->last_cr=0;return;}
 t->last_cr=0;
 if(c=='\b'){if(t->x){--t->x;t->cells[t->y][t->x]=' ';}return;}
 if(c<32||c>126)return;
 if(t->x>=TERM_COLS)newline(t);
 t->cells[t->y][t->x++]=c;
}
void term_write(Terminal *t,const char *s){while(*s)term_put(t,*s++);}
