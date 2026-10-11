#include <assert.h>
#include <string.h>
#include "../src/cpu6502.h"
#include "../src/terminal.h"
static void out(void *p,uint8_t ch){term_put((Terminal*)p,(char)ch);}
int main(void){
 Terminal t;term_init(&t);BasicVM vm;vm_init(&vm,out,&t);
 /* LDA #'H'; STA $D000; LDA #'I'; STA $D000; BRK */
 const uint8_t program[]={0xa9,'H',0x8d,0x00,0xd0,0xa9,'I',0x8d,0x00,0xd0,0x00};
 assert(vm_load(&vm,0x8000,program,sizeof(program))==0);
 vm.mem[0xfffc]=0x00;vm.mem[0xfffd]=0x80;
 Cpu6502 cpu;cpu_reset(&cpu,&vm);
 int status=0;for(int i=0;i<32 && status==0;i++)status=cpu_step(&cpu);
 assert(status==1);assert(!memcmp(t.cells[0],"HI",2));
 return 0;
}
