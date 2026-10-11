/* Types the first-milestone program using ONLY the FM-1 key mapping (27 note keys
 * + panel buttons, via src/keyboard.c), the same path the firmware uses, into
 * Microsoft BASIC running on the 6502 core, and checks the 40x24 terminal. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/cpu6502.h"
#include "../src/keyboard.h"
#include "../src/terminal.h"

static BasicVM vm;static Cpu6502 cpu;static Terminal term;static Keyboard kb;
static void out(void *c,uint8_t ch){term_put((Terminal*)c,(char)ch);}
static void run(unsigned long n){for(unsigned long i=0;i<n&&!cpu.stopped;i++){vm.ticks=i;cpu_step(&cpu);}}
static void deliver(int c){if(c>=0&&!(c>=0x11&&c<=0x14)){assert(vm_input(&vm,(uint8_t)c)==0);run(300000);}}
static void panel(unsigned p){deliver(keyboard_panel(&kb,p));}
/* press whatever physical keys are needed to produce character c */
static void type(char c){
 if(c=='\r'){panel(2);return;} if(c==' '){panel(4);return;} if(c=='\b'){panel(3);return;}
 for(int layer=0;layer<2;layer++)for(int shift=0;shift<2;shift++)for(unsigned n=0;n<27;n++){
  Keyboard t;keyboard_init(&t);t.layer=layer?KEY_SYMBOLS:KEY_ALPHA;t.shift=(uint8_t)shift;
  if(keyboard_note(&t,n)!=(unsigned char)c)continue;
  if(kb.layer!=t.layer)panel(7);     /* SAVE toggles the symbol layer */
  if(kb.shift!=t.shift)panel(1);     /* SCL toggles shift */
  deliver(keyboard_note(&kb,n));
  if(kb.shift)panel(1);              /* the on-screen shift is one-shot here: release it again */
  return;
 }
 fprintf(stderr,"cannot type %c\n",c);exit(1);
}
static void line(const char *s){for(;*s;s++)type(*s);type('\r');}
static int screen_has(const char *want){for(int y=0;y<TERM_ROWS;y++)if(strstr(term.cells[y],want)){return 1;}return 0;}
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"usage: %s basic.bin\n",argv[0]);return 2;}
 FILE *f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
 term_init(&term);vm_init(&vm,out,&term);
 if(fread(vm.mem+0x8000,1,0x8000,f)!=0x8000){fprintf(stderr,"need a 32 KiB ROM\n");return 2;}
 fclose(f);keyboard_init(&kb);cpu_reset(&cpu,&vm);
 run(3000000);
 assert(screen_has("BYTES FREE")&&screen_has("READY."));
 line("PRINT 2+2");
 assert(screen_has(" 4"));
 line("10 PRINT \"HELLO FM-1\"");
 line("20 PRINT 2+2:REM A:B;C<D>E,F.G?H");
 line("RUN");
 assert(screen_has("HELLO FM-1"));
 assert(screen_has("20 PRINT 2+2:REM A:B;C<D>E,F.G?H"));
 /* backspace key and Ctrl-C key (OCT-) */
 type('P');type('R');type('I');type('N');type('T');type(' ');type('9');type('\b');type('7');type('\r');
 assert(screen_has(" 7"));
 line("30 GOTO 30");line("RUN");
 panel(12);                          /* OCT- = Ctrl-C */
 run(300000);
 assert(screen_has("BREAK"));
 puts("keys: first milestone typed with FM-1 keys only: PASS");
 return 0;
}
