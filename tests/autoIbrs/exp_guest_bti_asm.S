#include "uarf/psnip.h"
#include "uarf/asm-macros.h"
#include "uarf/spec_lib.h"

.section .note.GNU-stack,"",@progbits

UARF_SNIP_START exp_guest_bti_jmp
    movq UARF_DATA__fr_buf_p(%rcx), %rdi                     // SIZE: 4
    movq UARF_DATA__secret(%rcx), %rsi                       // SIZE: 4
    movq $0, %rdx                                       // SIZE: 7
    movq UARF_DATA__spec_dst_p_p(%rcx), %r8                  // SIZE: 4
    clflush (%r8)                                       // SIZE: 4
    mfence // Keep here for now. Creates a new BB       // SIZE: 3
    lfence                                              // SIZE: 3
    jmp *(%r8) // Calls return                          // SIZE: 3
    int3
UARF_SNIP_END exp_guest_bti_jmp
