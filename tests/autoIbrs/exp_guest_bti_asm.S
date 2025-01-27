#include "uarf/psnip.h"
#include "uarf/asm-macros.h"
#include "uarf/spec_lib.h"

SNIP_START exp_guest_bti_jmp
    movq DATA__fr_buf_p(%rcx), %rdi                     // SIZE: 4
    movq DATA__secret(%rcx), %rsi                       // SIZE: 4
    movq $0, %rdx                                       // SIZE: 7
    movq DATA__spec_dst_p_p(%rcx), %r8                  // SIZE: 4
    clflush (%r8)                                       // SIZE: 4
    mfence // Keep here for now. Creates a new BB       // SIZE: 3
    lfence                                              // SIZE: 3
    jmp *(%r8) // Calls return                          // SIZE: 3
    int3
SNIP_END exp_guest_bti_jmp
