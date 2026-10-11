#include "cpu6502.h"
#include <string.h>
enum { F_C=1,F_Z=2,F_I=4,F_D=8,F_B=16,F_U=32,F_V=64,F_N=128 };
static uint8_t rd(Cpu6502*c,uint16_t a){return vm_read(c->vm,a);}
static void wr(Cpu6502*c,uint16_t a,uint8_t v){vm_write(c->vm,a,v);}
static uint8_t next(Cpu6502*c){return rd(c,c->pc++);}
static uint16_t imm16(Cpu6502*c){uint8_t lo=next(c);return (uint16_t)(lo|((uint16_t)next(c)<<8));}
static uint16_t zp(Cpu6502*c){return next(c);}
static uint16_t zpx(Cpu6502*c){return (uint8_t)(next(c)+c->x);}
static uint16_t zpy(Cpu6502*c){return (uint8_t)(next(c)+c->y);}
static uint16_t ab(Cpu6502*c){return imm16(c);}
static uint16_t ax(Cpu6502*c){return (uint16_t)(imm16(c)+c->x);}
static uint16_t ay(Cpu6502*c){return (uint16_t)(imm16(c)+c->y);}
static uint16_t ptr(Cpu6502*c,uint8_t p){return (uint16_t)(rd(c,p)|((uint16_t)rd(c,(uint8_t)(p+1))<<8));}
static uint16_t ix(Cpu6502*c){return ptr(c,(uint8_t)(next(c)+c->x));}
static uint16_t iy(Cpu6502*c){return (uint16_t)(ptr(c,next(c))+c->y);}
static void nz(Cpu6502*c,uint8_t v){c->p=(uint8_t)((c->p&~(F_N|F_Z))|(v?0:F_Z)|(v&F_N));}
static void push(Cpu6502*c,uint8_t v){wr(c,(uint16_t)(0x100|c->sp--),v);}
static uint8_t pop(Cpu6502*c){return rd(c,(uint16_t)(0x100|++c->sp));}
static void branch(Cpu6502*c,int yes){int8_t d=(int8_t)next(c);if(yes)c->pc=(uint16_t)(c->pc+d);}
static void cmp(Cpu6502*c,uint8_t a,uint8_t b){c->p=(uint8_t)((c->p&~F_C)|(a>=b?F_C:0));nz(c,(uint8_t)(a-b));}
static void adc(Cpu6502*c,uint8_t b){
 uint8_t a=c->a,carry=(c->p&F_C)?1:0;
 unsigned sum=(unsigned)a+b+carry;
 uint8_t binary=(uint8_t)sum;
 uint8_t overflow=(uint8_t)((~(a^b)&(a^binary)&0x80)?F_V:0);
 if(c->p&F_D){
  unsigned lo=(a&15)+(b&15)+carry;
  unsigned hi=(a>>4)+(b>>4);
  if(lo>9){lo+=6;hi++;}
  if(hi>9)hi+=6;
  c->a=(uint8_t)((hi<<4)|(lo&15));
  c->p=(uint8_t)((c->p&~(F_C|F_V))|(hi>15?F_C:0)|overflow);
  nz(c,binary); /* NMOS 6502 decimal flags derive from binary intermediate (approximation). */
 }else{
  c->a=binary;c->p=(uint8_t)((c->p&~(F_C|F_V))|(sum>255?F_C:0)|overflow);nz(c,c->a);
 }
}
static void sbc(Cpu6502*c,uint8_t b){
 uint8_t a=c->a,borrow=(c->p&F_C)?0:1;
 int diff=(int)a-(int)b-borrow;
 uint8_t binary=(uint8_t)diff;
 uint8_t overflow=(uint8_t)(((a^b)&(a^binary)&0x80)?F_V:0);
 if(c->p&F_D){
  int lo=(a&15)-(b&15)-borrow;
  int hi=(a>>4)-(b>>4);
  if(lo<0){lo-=6;hi--;}
  if(hi<0)hi-=6;
  c->a=(uint8_t)(((unsigned)hi<<4)|((unsigned)lo&15));
 }else c->a=binary;
 c->p=(uint8_t)((c->p&~(F_C|F_V))|(diff>=0?F_C:0)|overflow);
 nz(c,binary);
}
static uint8_t asl(Cpu6502*c,uint8_t v){c->p=(uint8_t)((c->p&~F_C)|(v&128?F_C:0));v=(uint8_t)(v<<1);nz(c,v);return v;}
static uint8_t lsr(Cpu6502*c,uint8_t v){c->p=(uint8_t)((c->p&~F_C)|(v&1));v>>=1;nz(c,v);return v;}
static uint8_t rol(Cpu6502*c,uint8_t v){uint8_t carry=c->p&F_C;c->p=(uint8_t)((c->p&~F_C)|(v&128?F_C:0));v=(uint8_t)((v<<1)|carry);nz(c,v);return v;}
static uint8_t ror(Cpu6502*c,uint8_t v){uint8_t carry=c->p&F_C;c->p=(uint8_t)((c->p&~F_C)|(v&1));v=(uint8_t)((v>>1)|(carry?128:0));nz(c,v);return v;}
void cpu_reset(Cpu6502*c,BasicVM*vm){memset(c,0,sizeof(*c));c->vm=vm;c->sp=0xfd;c->p=F_I|F_U;c->pc=(uint16_t)(rd(c,0xfffc)|((uint16_t)rd(c,0xfffd)<<8));}
int cpu_step(Cpu6502*c){
 if(c->stopped)return -1;
 uint8_t op=next(c),v=0;uint16_t a=0;
 /* 6502 group-1 opcodes: ORA AND EOR ADC STA LDA CMP SBC. */
 if((op&3)==1){
  unsigned mode=(op>>2)&7,kind=(op>>5)&7;
  if(kind==4 && mode==2)goto unsupported;
  switch(mode){
   case 0:a=ix(c);break;case 1:a=zp(c);break;
   case 2:v=next(c);break;case 3:a=ab(c);break;
   case 4:a=iy(c);break;case 5:a=zpx(c);break;
   case 6:a=ay(c);break;default:a=ax(c);break;
  }
  if(kind==4)wr(c,a,c->a);
  else {
   if(mode!=2)v=rd(c,a);
   switch(kind){
    case 0:c->a|=v;nz(c,c->a);break;
    case 1:c->a&=v;nz(c,c->a);break;
    case 2:c->a^=v;nz(c,c->a);break;
    case 3:adc(c,v);break;
    case 5:c->a=v;nz(c,v);break;
    case 6:cmp(c,c->a,v);break;
    case 7:sbc(c,v);break;
   }
  }
  c->cycles++;return 0;
 }
 switch(op){
 case 0xea:break;
 case 0xa2:c->x=next(c);nz(c,c->x);break;
 case 0xa6:c->x=rd(c,zp(c));nz(c,c->x);break;
 case 0xb6:c->x=rd(c,zpy(c));nz(c,c->x);break;
 case 0xae:c->x=rd(c,ab(c));nz(c,c->x);break;
 case 0xbe:c->x=rd(c,ay(c));nz(c,c->x);break;
 case 0xa0:c->y=next(c);nz(c,c->y);break;
 case 0xa4:c->y=rd(c,zp(c));nz(c,c->y);break;
 case 0xb4:c->y=rd(c,zpx(c));nz(c,c->y);break;
 case 0xac:c->y=rd(c,ab(c));nz(c,c->y);break;
 case 0xbc:c->y=rd(c,ax(c));nz(c,c->y);break;
 case 0x86:wr(c,zp(c),c->x);break;case 0x96:wr(c,zpy(c),c->x);break;case 0x8e:wr(c,ab(c),c->x);break;
 case 0x84:wr(c,zp(c),c->y);break;case 0x94:wr(c,zpx(c),c->y);break;case 0x8c:wr(c,ab(c),c->y);break;
 case 0xaa:c->x=c->a;nz(c,c->x);break;case 0x8a:c->a=c->x;nz(c,c->a);break;
 case 0xa8:c->y=c->a;nz(c,c->y);break;case 0x98:c->a=c->y;nz(c,c->a);break;
 case 0xba:c->x=c->sp;nz(c,c->x);break;case 0x9a:c->sp=c->x;break;
 case 0xe8:c->x++;nz(c,c->x);break;case 0xc8:c->y++;nz(c,c->y);break;
 case 0xca:c->x--;nz(c,c->x);break;case 0x88:c->y--;nz(c,c->y);break;
 case 0xe0:cmp(c,c->x,next(c));break;case 0xe4:cmp(c,c->x,rd(c,zp(c)));break;case 0xec:cmp(c,c->x,rd(c,ab(c)));break;
 case 0xc0:cmp(c,c->y,next(c));break;case 0xc4:cmp(c,c->y,rd(c,zp(c)));break;case 0xcc:cmp(c,c->y,rd(c,ab(c)));break;
 case 0xe6:a=zp(c);v=(uint8_t)(rd(c,a)+1);wr(c,a,v);nz(c,v);break;
 case 0xf6:a=zpx(c);v=(uint8_t)(rd(c,a)+1);wr(c,a,v);nz(c,v);break;
 case 0xee:a=ab(c);v=(uint8_t)(rd(c,a)+1);wr(c,a,v);nz(c,v);break;
 case 0xfe:a=ax(c);v=(uint8_t)(rd(c,a)+1);wr(c,a,v);nz(c,v);break;
 case 0xc6:a=zp(c);v=(uint8_t)(rd(c,a)-1);wr(c,a,v);nz(c,v);break;
 case 0xd6:a=zpx(c);v=(uint8_t)(rd(c,a)-1);wr(c,a,v);nz(c,v);break;
 case 0xce:a=ab(c);v=(uint8_t)(rd(c,a)-1);wr(c,a,v);nz(c,v);break;
 case 0xde:a=ax(c);v=(uint8_t)(rd(c,a)-1);wr(c,a,v);nz(c,v);break;
 case 0x0a:c->a=asl(c,c->a);break;case 0x4a:c->a=lsr(c,c->a);break;
 case 0x2a:c->a=rol(c,c->a);break;case 0x6a:c->a=ror(c,c->a);break;
 case 0x06:a=zp(c);v=asl(c,rd(c,a));wr(c,a,v);break;case 0x16:a=zpx(c);v=asl(c,rd(c,a));wr(c,a,v);break;
 case 0x0e:a=ab(c);v=asl(c,rd(c,a));wr(c,a,v);break;case 0x1e:a=ax(c);v=asl(c,rd(c,a));wr(c,a,v);break;
 case 0x46:a=zp(c);v=lsr(c,rd(c,a));wr(c,a,v);break;case 0x56:a=zpx(c);v=lsr(c,rd(c,a));wr(c,a,v);break;
 case 0x4e:a=ab(c);v=lsr(c,rd(c,a));wr(c,a,v);break;case 0x5e:a=ax(c);v=lsr(c,rd(c,a));wr(c,a,v);break;
 case 0x26:a=zp(c);v=rol(c,rd(c,a));wr(c,a,v);break;case 0x36:a=zpx(c);v=rol(c,rd(c,a));wr(c,a,v);break;
 case 0x2e:a=ab(c);v=rol(c,rd(c,a));wr(c,a,v);break;case 0x3e:a=ax(c);v=rol(c,rd(c,a));wr(c,a,v);break;
 case 0x66:a=zp(c);v=ror(c,rd(c,a));wr(c,a,v);break;case 0x76:a=zpx(c);v=ror(c,rd(c,a));wr(c,a,v);break;
 case 0x6e:a=ab(c);v=ror(c,rd(c,a));wr(c,a,v);break;case 0x7e:a=ax(c);v=ror(c,rd(c,a));wr(c,a,v);break;
 case 0x24:v=rd(c,zp(c));goto bit_test;case 0x2c:v=rd(c,ab(c));goto bit_test;
 case 0x18:c->p&=(uint8_t)~F_C;break;case 0x38:c->p|=F_C;break;
 case 0x58:c->p&=(uint8_t)~F_I;break;case 0x78:c->p|=F_I;break;
 case 0xd8:c->p&=(uint8_t)~F_D;break;case 0xf8:c->p|=F_D;break;
 case 0xb8:c->p&=(uint8_t)~F_V;break;
 case 0xf0:branch(c,c->p&F_Z);break;case 0xd0:branch(c,!(c->p&F_Z));break;
 case 0x90:branch(c,!(c->p&F_C));break;case 0xb0:branch(c,c->p&F_C);break;
 case 0x10:branch(c,!(c->p&F_N));break;case 0x30:branch(c,c->p&F_N);break;
 case 0x50:branch(c,!(c->p&F_V));break;case 0x70:branch(c,c->p&F_V);break;
 case 0x4c:c->pc=ab(c);break;
 case 0x6c:a=ab(c);c->pc=(uint16_t)(rd(c,a)|((uint16_t)rd(c,(uint16_t)((a&0xff00)|((a+1)&0xff)))<<8));break;
 case 0x20:a=ab(c);{uint16_t ret=(uint16_t)(c->pc-1);push(c,(uint8_t)(ret>>8));push(c,(uint8_t)ret);}c->pc=a;break;
 case 0x60:a=pop(c);a|=(uint16_t)pop(c)<<8;c->pc=(uint16_t)(a+1);break;
 case 0x40:c->p=(uint8_t)((pop(c)&~F_B)|F_U);a=pop(c);c->pc=(uint16_t)(a|((uint16_t)pop(c)<<8));break;
 case 0x48:push(c,c->a);break;case 0x68:c->a=pop(c);nz(c,c->a);break;
 case 0x08:push(c,(uint8_t)(c->p|F_B|F_U));break;case 0x28:c->p=(uint8_t)((pop(c)&~F_B)|F_U);break;
 case 0x00:c->stopped=1;return 1; /* test harness BRK trap; IRQ not yet implemented */
 default:goto unsupported;
 }
 c->cycles++;return 0;
bit_test:
 c->p=(uint8_t)((c->p&~(F_N|F_V|F_Z))|(v&(F_N|F_V))|((c->a&v)?0:F_Z));
 c->cycles++;return 0;
unsupported:c->stopped=1;return -1;
}
