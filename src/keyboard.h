#ifndef FM_BASIC_KEYBOARD_H
#define FM_BASIC_KEYBOARD_H
#include <stdint.h>
/* 27 note keys, left to right. Panel key IDs are defined by the caller. */
typedef enum { KEY_ALPHA, KEY_SYMBOLS } KeyLayer;
typedef struct { KeyLayer layer; uint8_t shift, ctrl; } Keyboard;
void keyboard_init(Keyboard *k);
int keyboard_note(Keyboard *k, unsigned note_index);
int keyboard_panel(Keyboard *k, unsigned panel_index);
#endif
