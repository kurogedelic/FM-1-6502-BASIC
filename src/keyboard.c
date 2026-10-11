#include "keyboard.h"
void keyboard_init(Keyboard *k){k->layer=KEY_ALPHA;k->shift=0;k->ctrl=0;}
/* 27 note keys, left to right (F3 .. G5).
 * Letters: three QWERTY rows, 27th key = space.
 * Symbols: everything a BASIC program needs without a computer keyboard:
 *   digits, quote, $ ( ) + - * / ^ = < > , . : ; ?
 * With SHIFT the symbol layer gives the remaining ASCII punctuation. */
int keyboard_note(Keyboard *k,unsigned n){
 static const char alpha[27+1]="QWERTYUIOPASDFGHJKLZXCVBNM ";
 static const char sym[27+1]  ="1234567890\"$()+-*/^=<>,.:;?";
 static const char symsh[15+1]="!@#%&'~|\\_`[]{}"; /* SHIFT, first 15 symbol keys; the rest are unchanged */
 if(n>=27)return -1;
 char c;
 if(k->layer==KEY_ALPHA){
  c=alpha[n];
  if(k->shift && c>='A'&&c<='Z')c=(char)(c+('a'-'A'));
  if(k->ctrl && c>='@'&&c<='_')c=(char)(c&31);
 }else{
  c=sym[n];
  if(k->shift && n<15)c=symsh[n];
 }
 return (unsigned char)c;
}
/* Panel button indices, in Felucca's label order: FX,SCL,ENV,LFO,EDIT,GLO,HOME,SAVE,ARP,SEQ,PLAY,REC,OCT-,OCT+.
 * Which printed label sits on which matrix button is a best guess that has not
 * been verified on every button (see docs/FM1_PORT.md). */
int keyboard_panel(Keyboard *k,unsigned p){
 switch(p){
 case 0:return '\t';
 case 1:k->shift^=1;return -1;
 case 2:return '\r';
 case 3:return '\b';
 case 4:return ' ';
 case 5:k->ctrl^=1;return -1;
 case 6:return 27;
 case 7:k->layer=k->layer==KEY_ALPHA?KEY_SYMBOLS:KEY_ALPHA;return -1;
 case 8:return 0x11; /* cursor left, private event */
 case 9:return 0x12; /* cursor right */
 case 10:return 0x13; /* cursor up */
 case 11:return 0x14; /* cursor down */
 case 12:return 3;   /* OCT-: Ctrl-C, BASIC BREAK */
 case 13:return 21;  /* OCT+: Ctrl-U, erase the line being typed */
 default:return -1;
 }
}
