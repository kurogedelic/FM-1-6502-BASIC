#include <assert.h>
#include <string.h>
#include "../src/cpu6502.h"
#include "../src/terminal.h"
static void out(void*p,uint8_t ch){term_put((Terminal*)p,(char)ch);}
static int run(Cpu6502*c,int limit){int s=0;while(limit-- && s==0)s=cpu_step(c);return s;}
int main(void){
 Terminal t;term_init(&t);BasicVM vm;vm_init(&vm,out,&t);
 const uint8_t program[]={0xa9,'H',0x8d,0x00,0xd0,0xa9,'I',0x8d,0x00,0xd0,0x00};
 assert(vm_load(&vm,0x8000,program,sizeof(program))==0);
 vm.mem[0xfffc]=0;vm.mem[0xfffd]=0x80;
 Cpu6502 cpu;cpu_reset(&cpu,&vm);cpu.trap_brk=1;
 assert(run(&cpu,32)==1);assert(!memcmp(t.cells[0],"HI",2));
 /* indexed load, arithmetic, compare, branch, stack and subroutine */
 const uint8_t advanced[]={
  0xa2,0x02,0xa9,0x03,0x95,0x10, /* LDX #2; LDA #3; STA $10,X */
  0xa9,0x00,0xb5,0x10,0x18,0x69,0x04, /* LDA $10,X; CLC; ADC #4 */
  0xc9,0x07,0xd0,0x0b, /* CMP #7; BNE failure */
  0x20,0x1e,0x81, /* JSR subroutine */
  0xc9,0x08,0xd0,0x06, /* CMP #8; BNE failure */
  0xa9,'!',0x8d,0x00,0xd0,0x00, /* output !; BRK */
  0x48,0x68,0x18,0x69,0x01,0x60 /* subroutine: PHA PLA CLC ADC #1 RTS */
 };
 assert(vm_load(&vm,0x8100,advanced,sizeof(advanced))==0);
 vm.mem[0xfffc]=0;vm.mem[0xfffd]=0x81;cpu_reset(&cpu,&vm);cpu.trap_brk=1;
 assert(run(&cpu,100)==1);assert(cpu.a=='!');assert(t.cells[0][2]=='!');
 /* decimal addition: 15 + 27 = 42 BCD */
 const uint8_t decimal[]={0xf8,0x18,0xa9,0x15,0x69,0x27,0x00};
 assert(vm_load(&vm,0x8200,decimal,sizeof(decimal))==0);
 vm.mem[0xfffc]=0;vm.mem[0xfffd]=0x82;cpu_reset(&cpu,&vm);cpu.trap_brk=1;
 assert(run(&cpu,20)==1);assert(cpu.a==0x42);
 /* real BRK, IRQ and NMI through the vectors, RTI returning */

 {
  const uint8_t main_code[]={0x58,0x00,0xea,0xa9,'K',0x8d,0x00,0xd0,0x00}; /* CLI; BRK; (pad); LDA #'K'; STA $D000; BRK */
  const uint8_t handler[]={0xe6,0x20,0x40}; /* INC $20; RTI */
  assert(vm_load(&vm,0x8300,main_code,sizeof(main_code))==0);
  assert(vm_load(&vm,0x8400,handler,sizeof(handler))==0);
  vm.mem[0x20]=0;
  vm.mem[0xfffc]=0;vm.mem[0xfffd]=0x83;vm.mem[0xfffe]=0;vm.mem[0xffff]=0x84;vm.mem[0xfffa]=0;vm.mem[0xfffb]=0x84;
  cpu_reset(&cpu,&vm);
  assert(cpu_step(&cpu)==0);            /* CLI */
  assert(cpu_step(&cpu)==0);            /* BRK vectors to handler */
  assert(cpu.pc==0x8400 && (cpu.p&4));
   assert(cpu_step(&cpu)==0 && vm.mem[0x20]==1);
  assert(cpu_step(&cpu)==0);            /* RTI returns past the BRK padding byte */
  assert(cpu.pc==0x8303);
  cpu_nmi(&cpu);assert(cpu.pc==0x8400);
  assert(cpu_step(&cpu)==0);assert(cpu_step(&cpu)==0);assert(cpu.pc==0x8303 && vm.mem[0x20]==2);
  assert(!(cpu.p&4));
  cpu_irq(&cpu);assert(cpu.pc==0x8400 && (cpu.p&4));
  cpu_irq(&cpu);assert(cpu.pc==0x8400);  /* masked while I is set */
 }
 return 0;
}
