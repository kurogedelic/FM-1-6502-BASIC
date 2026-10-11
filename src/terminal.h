#ifndef FM_BASIC_TERMINAL_H
#define FM_BASIC_TERMINAL_H
#include <stdint.h>
#define TERM_COLS 40
#define TERM_ROWS 24
typedef struct { char cells[TERM_ROWS][TERM_COLS]; uint8_t x,y,last_cr; } Terminal;
void term_init(Terminal *t);
void term_put(Terminal *t, char ch);
void term_write(Terminal *t, const char *s);
#endif
