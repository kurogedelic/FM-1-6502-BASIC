#ifndef FM_BASIC_CPU6502_H
#define FM_BASIC_CPU6502_H
#include "vm.h"
typedef struct { BasicVM *vm; uint16_t pc; uint8_t a,x,y,sp,p; unsigned long cycles; int stopped; } Cpu6502;
void cpu_reset(Cpu6502 *cpu,BasicVM *vm);
int cpu_step(Cpu6502 *cpu); /* 0=ok, -1=unsupported opcode, 1=BRK */
#endif
