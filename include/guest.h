/*
 * Helper for running code in the guest using kvm.
 */

#pragma once

#include "compiler.h"
#include "lib.h"

/**
 * Get the current ring from CS
 */
static __always_inline uint8_t uarf_get_ring(void) {
    uint64_t cs;
    asm volatile("movq %%cs, %0\n\t" : "=r"(cs));
    return cs & 3;
}

/**
 * Drop privileges from supervisor to user using iret
 */
static __always_inline void uarf_supervisor2user(void) {
    // clang-format off
	asm volatile(
		/* Disable interrupts */
		"cli\n\t"

		"movq %%rsp, %%rax\n\t"

		/* Push User SS */
#ifndef __USER_DS
#warning __USER_DS is not defined
#define __USER_DS 0
#endif

		"pushq $" STR(__USER_DS) "\n\t"

		/* Push RSP */
		"pushq %%rax\n\t"

		/* Push RFLAGS and re-enable interrupts */
		"pushfq\n\t"
#ifndef X86_EFLAGS_IF
#warning X86_EFLAGS_IF is not defined
#define X86_EFLAGS_IF 0
#endif

#ifndef X86_EFLAGS_IOPL
#warning X86_EFLAGS_IOPL is not defined
#define X86_EFLAGS_IOPL 0
#endif

		"orl $(" STR(X86_EFLAGS_IOPL | X86_EFLAGS_IF) "), (%%rsp)\n\t"

		/* Push User CS */
#ifndef __USER_CS
#warning __USER_CS is not defined
#define __USER_CS 0
#endif
		"pushq $" STR(__USER_CS) "\n\t"

		/* Push User RIP */
		"lea return_here%=, %%rax\n\t"
		"pushq %%rax\n\t"

		/* Pray */
		"iretq\n\t"

		"return_here%=:\n\t"
		/* TODO clobber stuff */
		::: "rax", "memory"
	);
    // clang-format on
}

/**
 * Escalate privileges from user to supervisor using syscall
 */
static __always_inline void uarf_user2supervisor(void) {
    asm volatile("mov $1, %%rax\n\t" // Set syscall number
                 "syscall\n\t" ::
                     : "rax", "rcx", "r11");
}

/**
 * Set `handler` as our syscall handler.
 */
static inline void uarf_init_syscall(void (*handler)(void), uint16_t kernel_cs,
                                     uint16_t user_cs_star) {
    // Enable syscalls
    uarf_wrmsr(MSR_EFER, uarf_rdmsr(MSR_EFER) | MSR_EFER__SCE);

    // Set handler
    uarf_wrmsr(MSR_LSTAR, _ul(handler));

    // Set segments
    uarf_wrmsr(MSR_STAR, _ul(kernel_cs) << 32 | _ul(user_cs_star) << 48);
}

/**
 * A syscall handler, that simply returns to the syscall callsite
 *
 * As it uses return instead of sysret, it essentially allows to escalate privileges.
 */
void uarf_syscall_handler_return(void);
