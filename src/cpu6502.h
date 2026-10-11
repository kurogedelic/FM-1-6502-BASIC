#ifndef FM_BASIC_CPU6502_H
#define FM_BASIC_CPU6502_H
#include "vm.h"
typedef struct {
 BasicVM *vm; uint16_t pc; uint8_t a,x,y,sp,p; unsigned long cycles; int stopped;
 int trap_brk; /* set by test harnesses: BRK stops the CPU instead of vectoring through $FFFE */
} Cpu6502;
void cpu_reset(Cpu6502 *cpu,BasicVM *vm);
int cpu_step(Cpu6502 *cpu); /* 0=ok, -1=unsupported opcode, 1=BRK trapped (trap_brk only) */
void cpu_irq(Cpu6502 *cpu); /* maskable interrupt: ignored while the I flag is set */
void cpu_nmi(Cpu6502 *cpu); /* non-maskable interrupt */
#endif
