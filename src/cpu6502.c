#include "cpu6502.h"
#include <string.h>
#define C 1
#define Z 2
#define I 4
#define D 8
#define B 16
#define U 32
#define V 64
#define N 128
static uint8_t rd(Cpu6502 *c,uint16_t a){return vm_read(c->vm,a);}
static void wr(Cpu6502 *c,uint16_t a,uint8_t v){vm_write(c->vm,a,v);}
static uint8_t fetch(Cpu6502 *c){return rd(c,c->pc++);}
static uint16_t word(Cpu6502 *c){uint8_t lo=fetch(c);return (uint16_t)(lo|((uint16_t)fetch(c)<<8));}
static void nz(Cpu6502 *c,uint8_t v){c->p=(uint8_t)((c->p&~(N|Z))|(v?0:Z)|(v&N));}
static void push(Cpu6502 *c,uint8_t v){wr(c,(uint16_t)(0x100|c->sp--),v);}
static uint8_t pop(Cpu6502 *c){return rd(c,(uint16_t)(0x100|++c->sp));}
static void branch(Cpu6502 *c,int take){int8_t d=(int8_t)fetch(c);if(take)c->pc=(uint16_t)(c->pc+d);}
static void adc(Cpu6502 *c,uint8_t b){
 uint8_t a=c->a,carry=(c->p&C)?1:0;
 unsigned sum=(unsigned)a+b+carry;
 uint8_t r=(uint8_t)sum;
 c->p=(uint8_t)((c->p&~(C|V))|(sum>255?C:0)|((~(a^b)&(a^r)&128)?V:0));
 /* Decimal mode is not implemented yet. */
 c->a=r;nz(c,r);
}
void cpu_reset(Cpu6502 *c,BasicVM *vm){memset(c,0,sizeof(*c));c->vm=vm;c->sp=0xfd;c->p=I|U;c->pc=(uint16_t)(rd(c,0xfffc)|((uint16_t)rd(c,0xfffd)<<8));}
int cpu_step(Cpu6502 *c){
 if(c->stopped)return -1;
 uint8_t op=fetch(c),v;uint16_t a;
 switch(op){
 case 0xea:break; /* NOP */
 case 0xa9:c->a=fetch(c);nz(c,c->a);break;
 case 0xa5:c->a=rd(c,fetch(c));nz(c,c->a);break;
 case 0xad:c->a=rd(c,word(c));nz(c,c->a);break;
 case 0xa2:c->x=fetch(c);nz(c,c->x);break;
 case 0xa0:c->y=fetch(c);nz(c,c->y);break;
 case 0x85:wr(c,fetch(c),c->a);break;
 case 0x8d:a=word(c);wr(c,a,c->a);break;
 case 0x86:wr(c,fetch(c),c->x);break;
 case 0x84:wr(c,fetch(c),c->y);break;
 case 0xe8:c->x++;nz(c,c->x);break;
 case 0xc8:c->y++;nz(c,c->y);break;
 case 0xca:c->x--;nz(c,c->x);break;
 case 0x88:c->y--;nz(c,c->y);break;
 case 0x69:adc(c,fetch(c));break;
 case 0x18:c->p&=(uint8_t)~C;break;
 case 0x38:c->p|=C;break;
 case 0xc9:v=fetch(c);a=(uint16_t)c->a-v;c->p=(uint8_t)((c->p&~C)|(c->a>=v?C:0));nz(c,(uint8_t)a);break;
 case 0xf0:branch(c,c->p&Z);break;
 case 0xd0:branch(c,!(c->p&Z));break;
 case 0x90:branch(c,!(c->p&C));break;
 case 0xb0:branch(c,c->p&C);break;
 case 0x4c:c->pc=word(c);break;
 case 0x20:a=word(c);{uint16_t ret=(uint16_t)(c->pc-1);push(c,(uint8_t)(ret>>8));push(c,(uint8_t)ret);}c->pc=a;break;
 case 0x60:a=pop(c);a|=(uint16_t)pop(c)<<8;c->pc=(uint16_t)(a+1);break;
 case 0x48:push(c,c->a);break;
 case 0x68:c->a=pop(c);nz(c,c->a);break;
 case 0x08:push(c,(uint8_t)(c->p|B|U));break;
 case 0x28:c->p=(uint8_t)((pop(c)&~B)|U);break;
 case 0x00:c->stopped=1;return 1; /* temporary BRK trap, not interrupt semantics */
 default:c->stopped=1;return -1;
 }
 c->cycles++;return 0;
}
