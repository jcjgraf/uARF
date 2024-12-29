#include "asm-macros.h"
#include "page.h"
#include "psnip.h"

SNIP_START jita_inc
    inc %rdi
SNIP_END jita_inc

SNIP_START jita_dec
    dec %rdi
SNIP_END jita_dec

SNIP_START jita_jump
   jmp 1f
   .fill 7, 1, 0x90
1:
   nop
SNIP_END jita_jump

SNIP_START jita_ret_val
   mov %rdi, %rax
   ret
SNIP_END jita_ret_val

