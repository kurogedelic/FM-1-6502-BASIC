#include "cpu6502.h"
#include <stdio.h>
#include <stdlib.h>
static void put_char(void *ctx,uint8_t ch){(void)ctx;putchar(ch);fflush(stdout);}
int main(int argc,char **argv){
 if(argc<2||argc>4){fprintf(stderr,"usage: %s ROM.bin [load-address-hex] [reset-address-hex]\n",argv[0]);return 2;}
 unsigned long load=0x8000,reset=0x8000;
 if(argc>2){char *end;load=strtoul(argv[2],&end,16);if(*end||load>65535)return 2;reset=load;}
 if(argc>3){char *end;reset=strtoul(argv[3],&end,16);if(*end||reset>65535)return 2;}
 FILE *f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
 if(fseek(f,0,SEEK_END)){fclose(f);return 2;}
 long length=ftell(f);
 if(length<0||(unsigned long)length>VM_MEM_SIZE-load){fprintf(stderr,"ROM does not fit at %04lX\n",load);fclose(f);return 2;}
 rewind(f);
 BasicVM *vm=malloc(sizeof(*vm));if(!vm){fclose(f);return 2;}
 vm_init(vm,put_char,NULL);
 if(fread(vm->mem+load,1,(size_t)length,f)!=(size_t)length){fprintf(stderr,"ROM read failed\n");fclose(f);free(vm);return 2;}
 fclose(f);
 vm->mem[0xfffc]=(uint8_t)reset;vm->mem[0xfffd]=(uint8_t)(reset>>8);
 Cpu6502 cpu;cpu_reset(&cpu,vm);
 for(unsigned long i=0;i<10000000ul;i++){
  /* Nonblocking input is not available in this portable host runner.
     Preload piped input before executing an interactive ROM. */
  int result=cpu_step(&cpu);
  if(result==1){fprintf(stderr,"\nBRK at %04X after %lu steps\n",(unsigned)((cpu.pc-1)&65535),i+1);free(vm);return 0;}
  if(result<0){fprintf(stderr,"\nUnsupported opcode at %04X\n",(unsigned)((cpu.pc-1)&65535));free(vm);return 1;}
 }
 fprintf(stderr,"\nExecution budget exhausted at %04X\n",cpu.pc);
 free(vm);return 1;
}
