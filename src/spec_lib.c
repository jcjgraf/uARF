#include "spec_lib.h"
#include <stdlib.h>

struct history get_randomized_history(void) {
    return (struct history){
        .hist[0] = random(),
        .hist[1] = random(),
    };
}

// Do not inline, such that we ensure it is "paged"
void run_spec(void *arg) {
    struct SpecData *data = (struct SpecData *) arg;

    asm volatile("lea return_here%=, %%rax\n\t"
                 "pushq %%rax\n\t"
                 "jmp *%0\n\t"
                 "return_here%=:\n\t" ::"r"(data->spec_prim_p),
                 "c"(data)
                 : "rax", "rdx", "rdi", "rsi", "r8", "memory");
}
