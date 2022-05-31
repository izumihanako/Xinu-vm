/* syscallinit.c - syscallinit (x86) */

#include <xinu.h>

extern void syscall_handler(void) ;
void syscallinit(void){
    set_evec( 0x2e , (uint32)syscall_handler ) ;
}