#include "keyboard.h"
void keyboard_init(Keyboard *k){k->layer=KEY_ALPHA;k->shift=0;k->ctrl=0;}
int keyboard_note(Keyboard *k,unsigned n){
 static const char alpha[]="QWERTYUIOPASDFGHJKLZXCVBNM ";
 static const char sym[]="1234567890!@#$%^&*()-+=/?.,:; ";
 if(n>=27)return -1;
 char c=k->layer==KEY_ALPHA?alpha[n]:sym[n];
 if(k->shift && c>='A'&&c<='Z')c=(char)(c+('a'-'A'));
 if(k->ctrl && c>='@'&&c<='_')c=(char)(c&31);
 return (unsigned char)c;
}
/* Panel indices from Felucca: FX,SCL,ENV,LFO,EDIT,GLO,HOME,SAVE,ARP,SEQ,PLAY,REC */
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
 default:return -1;
 }
}
