/*
 * Library of ASM snippets that are useful for branch predictor code.
 */

##include <toolkit/lib.h>

#include "psnip.h"
#include "asm-macros.h"
#include "flush_reload.h"
#include "spec_lib.h"

/*
 * Victim branch source using an indirect call
 * Regs:
 * - RCX: Pointer to SpecData struct
 * Stack:
 * - Return address
 * Clobbers: RDI, RSI, R8
 */
SNIP_START src_call_ind
    movq DATA__fr_buf_p(%rcx), %rdi                     // SIZE: 4
    movq DATA__secret(%rcx), %rsi                       // SIZE: 4
    movq $0, %rdx                                       // SIZE: 7
    movq DATA__spec_dst_p_p(%rcx), %r8                  // SIZE: 4
    clflush (%r8)                                       // SIZE: 4
    mfence // Keep here for now. Creates a new BB       // SIZE: 3
    lfence                                              // SIZE: 3
    call *(%r8)                                         // SIZE: 3
    // Push return to next cacheline to prevent overwriting of BTB entry
    .fill 64, 1, 0x90
    ret
    int3
SNIP_END src_call_ind

/*
 * Victim branch source using an indirect jmp
 * Regs:
 * - RCX: Pointer to SpecData struct
 * Stack:
 * - Return address
 * Clobbers: RDI, RSI, R8
 */
SNIP_START src_jmp_ind
    movq DATA__fr_buf_p(%rcx), %rdi                     // SIZE: 4
    movq DATA__secret(%rcx), %rsi                       // SIZE: 4
    movq $0, %rdx                                       // SIZE: 7
    movq DATA__spec_dst_p_p(%rcx), %r8                  // SIZE: 4
    clflush (%r8)                                       // SIZE: 4
    mfence // Keep here for now. Creates a new BB       // SIZE: 3
    lfence                                              // SIZE: 3
    jmp *(%r8) // Calls return                          // SIZE: 3
    int3
SNIP_END src_jmp_ind

/*
 * Gadget Code, leaking the secret when executed speculatively
 * Regs:
 * - RDI: Base address of FR buffer
 * - RSI: Secret encoded as offset into FR buffer
 * Stack:
 * - Return Address
 * Clobbers: RDI, RSI, RAX
 */
SNIP_START dst_gadget
    shlq $FR_STRIDE_BITS, %rsi
    movq (%rdi, %rsi), %rax
    lfence
    ret
    int3
SNIP_END dst_gadget

/*
 * Gadget Code, determining the size of the speculation windows in terms of memory fetches that can be done
 * Acces FR_BUF[0], uses the return value recursively to index into FR_BUF.
 * Regs:
 * - RDI: Base address of FR buffer
 * - RSI: Secret encoded as offset into FR buffer
 * Stack:
 * - Return Address
 * Clobbers: RDI, RSI, RAX
 */
SNIP_START dst_gadget_time
    // Access FR_BUF[0]
    movq (%rdi), %rax
    // Access FR_BUF[%rax]
    shlq $FR_STRIDE_BITS, %rax
    movq (%rdi, %rax), %rax
    // Access FR_BUF[%rax]
    shlq $FR_STRIDE_BITS, %rax
    movq (%rdi, %rax), %rax
    // Access FR_BUF[%rax]
    shlq $FR_STRIDE_BITS, %rax
    movq (%rdi, %rax), %rax
    // Access FR_BUF[%rax]
    shlq $FR_STRIDE_BITS, %rax
    movq (%rdi, %rax), %rax
    // Access FR_BUF[%rax]
    shlq $FR_STRIDE_BITS, %rax
    movq (%rdi, %rax), %rax
    // Access FR_BUF[%rax]
    shlq $FR_STRIDE_BITS, %rax
    movq (%rdi, %rax), %rax
    // Access FR_BUF[%rax]
    shlq $FR_STRIDE_BITS, %rax
    movq (%rdi, %rax), %rax
    lfence
    ret
    int3
SNIP_END dst_gadget_time

/*
 * Gadget Code, leaking the tainted secret when executed speculatively
 * The secret is tained by adding constant i to it
 * Regs:
 * - RDI: Base address of FR buffer
 * - RSI: Secret encoded as an index into FR buffer
 * Stack:
 * - Return Address
 * Clobbers: RDI, RSI, RAX
 */
.macro dst_gadget_i i
    SNIP_START dst_gadget_\i
        addq $\i, %rsi
        shlq $FR_STRIDE_BITS, %rsi
        movq (%rdi, %rsi), %rax
        lfence
        ret
        int3
    SNIP_END dst_gadget_\i
.endm

dst_gadget_i 0
dst_gadget_i 1
dst_gadget_i 2
dst_gadget_i 3
dst_gadget_i 4
dst_gadget_i 5
dst_gadget_i 6
dst_gadget_i 7
dst_gadget_i 8
dst_gadget_i 9
dst_gadget_i 10
dst_gadget_i 11
dst_gadget_i 12
dst_gadget_i 13
dst_gadget_i 14
dst_gadget_i 15
dst_gadget_i 16
dst_gadget_i 17
dst_gadget_i 18
dst_gadget_i 19
dst_gadget_i 20
dst_gadget_i 21
dst_gadget_i 22
dst_gadget_i 23
dst_gadget_i 24
dst_gadget_i 25
dst_gadget_i 26
dst_gadget_i 27
dst_gadget_i 28
dst_gadget_i 29
dst_gadget_i 30
dst_gadget_i 31

# Dummy Code
# Stack:
# - Return Address
SNIP_START dst_dummy
    lfence
    ret
    int3
SNIP_END dst_dummy

SNIP_START history
    movq DATA__hist(%rcx), %rax    // SIZE: 4
    test $0x00000001, %eax          // SIZE: 5
    jz 1f                           // Size: 2
    .skip 0x8, 0x90
    1:
    test $0x00000002, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000004, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000008, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000010, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000020, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000040, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000080, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000100, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000200, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000400, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000800, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00001000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00002000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00004000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00008000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00010000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00020000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00040000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00080000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00100000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00200000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00400000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00800000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x01000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x02000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x04000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x08000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x10000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x20000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x40000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x80000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
// Mid
    movq (DATA__hist + DATA__hist_size)(%rcx), %rax
    test $0x00000001, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000002, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000004, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000008, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000010, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000020, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000040, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000080, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000100, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000200, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000400, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00000800, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00001000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00002000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00004000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00008000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00010000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00020000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00040000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00080000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00100000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00200000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00400000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x00800000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x01000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x02000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x04000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x08000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x10000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x20000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x40000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
    test $0x80000000, %eax
    jz 1f
    .skip 0x8, 0x90
    1:
SNIP_END history

SNIP_START rdpmc
    // Backup RSI and RSI <- RCX
    movq %rsi,  DATA__memory0(%rcx)
    movq %rcx, %rsi
    // Backup RDX and RAX
    movq %rdx,  DATA__memory1(%rsi)
    movq %rax,  DATA__memory2(%rsi)
    // Load index and run RDPMC
    movq  DATA__memory3(%rsi), %rcx
    rdpmc
    // Store obtained values
    movq %rdx,  DATA__memory4(%rsi)
    movq %rax,  DATA__memory5(%rsi)
    // Reset registers
    movq  DATA__memory1(%rsi), %rdx
    movq  DATA__memory2(%rsi), %rax
    movq %rsi, %rcx
    movq  DATA__memory0(%rcx), %rsi
SNIP_END rdpmc
