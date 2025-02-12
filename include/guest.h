/*
 * Helper for running code in the guest using kvm.
 */

#pragma once

#include "lib.h"
#include "log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_GUEST
#endif

/**
 * Get the current ring from CS
 */
static __always_inline uint8_t get_ring(void) {
    uint64_t cs;
    asm volatile("movq %%cs, %0\n\t" : "=r"(cs));
    return cs & 3;
}

/**
 * Drop privileges from supervisor to user using iret
 */
static __always_inline void supervisor2user(void) {
    // clang-format off
	asm volatile(
		/* Disable interrupts */
		"cli\n\t"

		"movq %%rsp, %%rax\n\t"

		/* Push User SS */
		"pushq $" STR(__USER_DS) "\n\t"

		/* Push RSP */
		"pushq %%rax\n\t"

		/* Push RFLAGS and re-enable interrupts */
		"pushfq\n\t"
		"orl $(" STR(X86_EFLAGS_IOPL | X86_EFLAGS_IF) "), (%%rsp)\n\t"

		/* Push User CS */
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
static __always_inline void user2supervisor(void) {
    asm volatile("mov $1, %%rax\n\t" // Set syscall number
                 "syscall\n\t" ::
                     : "rax", "rcx", "r11");
}

/**
 * Set `handler` as our syscall handler.
 */
#define MSR_EFER  0xc0000080
#define MSR_STAR  0xc0000081
#define MSR_LSTAR 0xc0000082
#define _EFER_SCE 0
#define EFER_SCE  (1 << _EFER_SCE)

void init_syscall(void (*handler)(void), uint16_t kernel_cs, uint16_t user_cs_star) {
    // Enable syscalls
    wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_SCE);

    // Set handler
    wrmsr(MSR_LSTAR, _ul(handler));

    // Set segments
    wrmsr(MSR_STAR, _ul(kernel_cs) << 32 | _ul(user_cs_star) << 48);
}

/**
 * A syscall handler, that simply returns to the syscall callsite
 *
 * As it uses return instead of sysret, it essentially allows to escalate privileges.
 */
void syscall_handler_return(void);
