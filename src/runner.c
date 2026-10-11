#define _POSIX_C_SOURCE 200809L
#include "cpu6502.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Host ROM runner.
 *   runner [options] ROM.bin [load-address-hex] [reset-address-hex]
 * Options:
 *   --exit-on-idle   exit 0 once stdin is at EOF and the ROM is polling the
 *                    input status register with nothing left to read
 *   --max-steps N    instruction budget (default 10000000)
 * Standard input is streamed into the virtual input queue as the ROM drains
 * it. Characters written to $D000 go to standard output unchanged. */
static void put_char(void *ctx,uint8_t ch){(void)ctx;putchar(ch);fflush(stdout);}

typedef struct { int eof; } Input;
static void feed(BasicVM *vm,Input *in,int wait_ms){
 size_t room=vm_input_space(vm);
 if(in->eof||room==0)return;
 if(wait_ms>0){struct pollfd p={0,POLLIN,0};if(poll(&p,1,wait_ms)<=0)return;}
 uint8_t buf[128];
 if(room>sizeof(buf))room=sizeof(buf);
 ssize_t n=read(0,buf,room);
 if(n==0){in->eof=1;return;}
 if(n<0){if(errno!=EAGAIN&&errno!=EINTR)in->eof=1;return;}
 for(ssize_t i=0;i<n;i++)(void)vm_input(vm,buf[i]);
}

int main(int argc,char **argv){
 int exit_on_idle=0;unsigned long max_steps=10000000ul;
 const char *pos[3]={0,0,0};int npos=0;
 for(int i=1;i<argc;i++){
  if(!strcmp(argv[i],"--exit-on-idle"))exit_on_idle=1;
  else if(!strcmp(argv[i],"--max-steps")&&i+1<argc){char *end;max_steps=strtoul(argv[++i],&end,10);if(*end||!max_steps)return 2;}
  else if(npos<3&&argv[i][0]!='-')pos[npos++]=argv[i];
  else npos=4;
 }
 if(npos<1||npos>3){fprintf(stderr,"usage: %s [--exit-on-idle] [--max-steps N] ROM.bin [load-address-hex] [reset-address-hex]\n",argv[0]);return 2;}
 unsigned long load=0x8000,reset=0x8000;
 if(npos>1){char *end;load=strtoul(pos[1],&end,16);if(*end||load>65535)return 2;reset=load;}
 if(npos>2){char *end;reset=strtoul(pos[2],&end,16);if(*end||reset>65535)return 2;}
 FILE *f=fopen(pos[0],"rb");if(!f){perror(pos[0]);return 2;}
 if(fseek(f,0,SEEK_END)){fclose(f);return 2;}
 long length=ftell(f);
 if(length<0||(unsigned long)length>VM_MEM_SIZE-load){fprintf(stderr,"ROM does not fit at %04lX\n",load);fclose(f);return 2;}
 rewind(f);
 BasicVM *vm=malloc(sizeof(*vm));if(!vm){fclose(f);return 2;}
 vm_init(vm,put_char,NULL);
 if(fread(vm->mem+load,1,(size_t)length,f)!=(size_t)length){fprintf(stderr,"ROM read failed\n");fclose(f);free(vm);return 2;}
 fclose(f);
 /* A ROM image that covers the vector area supplies its own vectors. */
 if(npos>2||load+(unsigned long)length<=0xfffdul){vm->mem[0xfffc]=(uint8_t)reset;vm->mem[0xfffd]=(uint8_t)(reset>>8);}
 int fl=fcntl(0,F_GETFL);if(fl>=0)fcntl(0,F_SETFL,fl|O_NONBLOCK);
 Input in={0};
 Cpu6502 cpu;cpu_reset(&cpu,vm);
 for(unsigned long i=0;i<max_steps;i++){
  if((i&63)==0){
   int idle=vm->idle_polls>1000;
   feed(vm,&in,idle&&!in.eof&&!exit_on_idle?10:0);
   if(!in.eof&&idle&&exit_on_idle)feed(vm,&in,10);
   if(in.eof&&idle&&vm->head==vm->tail&&exit_on_idle){free(vm);return 0;}
  }
  vm->ticks=i;
  int result=cpu_step(&cpu);
  if(result==1){fprintf(stderr,"\nBRK at %04X after %lu steps\n",(unsigned)((cpu.pc-1)&65535),i+1);free(vm);return 0;}
  if(result<0){fprintf(stderr,"\nUnsupported opcode %02X at %04X\n",(unsigned)vm->mem[(cpu.pc-1)&65535],(unsigned)((cpu.pc-1)&65535));free(vm);return 1;}
 }
 fprintf(stderr,"\nExecution budget exhausted at %04X\n",cpu.pc);
 free(vm);return 1;
}
