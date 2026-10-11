#ifndef FM_BASIC_VM_H
#define FM_BASIC_VM_H
#include <stdint.h>
#include <stddef.h>
#define VM_MEM_SIZE 65536u
#define VM_IO_DATA 0xD000u
#define VM_IO_STATUS 0xD001u
typedef struct {
 uint8_t mem[VM_MEM_SIZE];
 uint8_t input[128];
 unsigned head,tail;
 unsigned rom_start;   /* writes at or above this address are ignored (default 0x8000); 0x10000 = all RAM */
 unsigned long ticks; /* advanced by the host once per executed instruction */
 unsigned long last_poll;
 unsigned long idle_polls; /* back-to-back empty status reads (a ROM waiting for a key) */
 void (*output)(void *,uint8_t);
 void *output_context;
} BasicVM;
void vm_init(BasicVM *vm,void (*output)(void *,uint8_t),void *ctx);
uint8_t vm_read(BasicVM *vm,uint16_t addr);
void vm_write(BasicVM *vm,uint16_t addr,uint8_t data);
int vm_input(BasicVM *vm,uint8_t data);
size_t vm_input_space(const BasicVM *vm);
int vm_load(BasicVM *vm,uint16_t address,const uint8_t *rom,size_t size);
#endif
