#include "terminal.h"
#include <string.h>
void term_init(Terminal *t) { memset(t,0,sizeof(*t)); for(int y=0;y<TERM_ROWS;y++) memset(t->cells[y],' ',TERM_COLS); }
static void newline(Terminal *t) { t->x=0; if (++t->y>=TERM_ROWS) { memmove(t->cells[0],t->cells[1],(TERM_ROWS-1)*TERM_COLS); memset(t->cells[TERM_ROWS-1],' ',TERM_COLS); t->y=TERM_ROWS-1; } }
void term_put(Terminal *t,char c) { if(c=='\r'||c=='\n'){newline(t);return;} if(c=='\b'){if(t->x){--t->x;t->cells[t->y][t->x]=' ';}return;} if(c<32||c>126)return; t->cells[t->y][t->x++]=c; if(t->x>=TERM_COLS)newline(t); }
void term_write(Terminal *t,const char *s){while(*s)term_put(t,*s++);}
