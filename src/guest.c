#include "guest.h"

void syscall_handler_return(void) {
    asm volatile("pushq %%rcx\n\t" ::: "memory");
    return;
}
