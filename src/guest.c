#include "guest.h"

void uarf_syscall_handler_return(void) {
    asm volatile("pushq %%rcx\n\t" ::: "memory");
    return;
}
