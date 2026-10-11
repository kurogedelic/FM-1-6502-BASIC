#include <assert.h>
#include <string.h>
#include "../src/terminal.h"
#include "../src/keyboard.h"
#include "../src/vm.h"
static void output(void *ctx,uint8_t ch){term_put((Terminal *)ctx,(char)ch);}
int main(void){
 Terminal t;term_init(&t);term_write(&t,"HELLO");assert(!memcmp(t.cells[0],"HELLO",5));
 Keyboard k;keyboard_init(&k);assert(keyboard_note(&k,0)=='Q');assert(keyboard_note(&k,25)=='M');
 assert(keyboard_panel(&k,7)==-1);assert(keyboard_note(&k,0)=='1');
 assert(keyboard_panel(&k,2)=='\r');
 BasicVM vm;vm_init(&vm,output,&t);assert(vm_input(&vm,'A')==0);assert(vm_read(&vm,VM_IO_STATUS)==1);assert(vm_read(&vm,VM_IO_DATA)=='A');assert(vm_read(&vm,VM_IO_STATUS)==0);
 vm_write(&vm,VM_IO_DATA,'!');assert(t.cells[0][5]=='!');
 const uint8_t rom[]={0xa9,0x42};assert(vm_load(&vm,0x8000,rom,sizeof(rom))==0);assert(vm_read(&vm,0x8001)==0x42);
 return 0;
}
