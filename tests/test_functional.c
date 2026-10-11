/* Runs Klaus Dormann's 6502 functional test (GPL, NOT bundled; fetch with
 * tools/fetch_functional_test.sh) on the host CPU core.
 *   test_functional 6502_functional_test.bin
 * The image is a full 64 KiB RAM image started at $0400; success is a jump-to-self
 * at $3469. Any other jump-to-self is a failed test case. Exits 0 on success. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/cpu6502.h"
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"usage: %s 6502_functional_test.bin\n",argv[0]);return 2;}
 FILE *f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
 BasicVM *vm=malloc(sizeof(*vm));if(!vm)return 2;
 vm_init(vm,NULL,NULL);
 vm->rom_start=0x10000u; /* the test image is all RAM */
 if(fread(vm->mem,1,VM_MEM_SIZE,f)!=VM_MEM_SIZE){fprintf(stderr,"expected a 64 KiB image\n");return 2;}
 fclose(f);
 vm->mem[0xfffc]=0x00;vm->mem[0xfffd]=0x04;
 Cpu6502 cpu;cpu_reset(&cpu,vm);
 for(unsigned long i=0;i<200000000ul;i++){
  uint16_t pc=cpu.pc;
  int r=cpu_step(&cpu);
  if(r!=0){printf("FAIL: %s at $%04X (opcode %02X) after %lu instructions\n",r<0?"unsupported opcode":"BRK",pc,vm->mem[pc],i);free(vm);return 1;}
  if(cpu.pc==pc){
   if(pc==0x3469){printf("PASS: functional test reached success trap after %lu instructions\n",i);free(vm);return 0;}
   printf("FAIL: trapped at $%04X after %lu instructions (A=%02X X=%02X Y=%02X P=%02X SP=%02X)\n",pc,i,cpu.a,cpu.x,cpu.y,cpu.p,cpu.sp);free(vm);return 1;
  }
 }
 printf("FAIL: instruction budget exhausted at $%04X\n",cpu.pc);free(vm);return 1;
}
