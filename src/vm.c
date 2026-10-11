#include "vm.h"
#include <string.h>
void vm_init(BasicVM *vm,void (*output)(void *,uint8_t),void *ctx){memset(vm,0,sizeof(*vm));vm->rom_start=0x8000u;vm->output=output;vm->output_context=ctx;}
uint8_t vm_read(BasicVM *vm,uint16_t a){
 if(a==VM_IO_STATUS){if(vm->head!=vm->tail)return 1;vm->idle_polls=(vm->ticks-vm->last_poll<=16)?vm->idle_polls+1:1;vm->last_poll=vm->ticks;return 0;}
 if(a==VM_IO_DATA){vm->idle_polls=0;if(vm->head==vm->tail)return 0;uint8_t v=vm->input[vm->tail];vm->tail=(vm->tail+1u)%sizeof(vm->input);return v;}
 return vm->mem[a];
}
void vm_write(BasicVM *vm,uint16_t a,uint8_t v){if(a==VM_IO_DATA){if(vm->output)vm->output(vm->output_context,v);return;}if(a==VM_IO_STATUS||a>=vm->rom_start)return;vm->mem[a]=v;}
int vm_input(BasicVM *vm,uint8_t v){unsigned n=(vm->head+1u)%sizeof(vm->input);if(n==vm->tail)return -1;vm->input[vm->head]=v;vm->head=n;return 0;}
int vm_load(BasicVM *vm,uint16_t a,const uint8_t *rom,size_t size){if(!rom||size>VM_MEM_SIZE-(size_t)a)return -1;memcpy(vm->mem+a,rom,size);return 0;}
size_t vm_input_space(const BasicVM *vm){return (vm->tail+sizeof(vm->input)-vm->head-1u)%sizeof(vm->input);}
