/**
 * Hello World program running code in a kvm VM
 *
 * This file is a kvm selftest.
 * Designed to work with Linux v6.6
 *
 * Steps to run:
 * - Open `tools/testing/selftests/kvm/`
 * - Copy this test to `kvm/x86_64/rev_eng`
 * - Symlink uARF root into `kvm/lib` as `uarf`
 * - Symlink uARF/include into `kvm/include` as `uarf`
 * - Edit `kvm/Makefile`
 *    - Add `TEST_GEN_PROGS_x86_64 += x86_64/rev_eng/exa_guest`
 *    - Add `LDLIBS += -L./lib/uarf -luarf`
 * - Run `make && ./x86_64/rev_eng/exa_guest`
 */

#include "uarf/jita.h"
#include "uarf/kmod/pi.h"
#include "uarf/kmod/rap.h"
#include "uarf/lib.h"
#include "uarf/psnip.h"

#include <err.h>
#include <syscall.h>
#include <unistd.h>

#include <ucall_common.h>

#define NUM_ADDITINAL_PAGES 1

psnip_declare_define(test_dst, "lfence\n"
                               "ret\n"
                               "int3\n");

typedef struct registers {
    union {
        void *code_ptr;
        uint64_t code_addr;
        void (*f)(void *);
    };
} registers_t;

registers_t registers;

void do_call(registers_t *r) {
    asm volatile("call *%0\n" ::"r"(r->code_ptr) :);
}

static void guest_main(void) {
    GUEST_PRINTF("Hello Guest!\n");

    do_call(&registers);

    GUEST_DONE();
}

static void run_vcpu(struct kvm_vcpu *vcpu) {
    struct ucall uc;

    vcpu_run(vcpu);

    switch (get_ucall(vcpu, &uc)) {
    case UCALL_SYNC:
        return;
    case UCALL_DONE:
        return;
    case UCALL_ABORT:
        REPORT_GUEST_ASSERT(uc);
    case UCALL_PRINTF:
        // allow the guest to print debug information
        printf("guest | %s", uc.buffer);
        break;
    default:
        TEST_ASSERT(false, "Unexpected exit: %s",
                    exit_reason_str(vcpu->run->exit_reason));
    }
}

void copy_to_guest(struct kvm_vm *vm, stub_t *stub) {
    static u64 next_slot = NR_MEM_REGIONS;
    static u64 used_pages = 0;

    u64 guest_num_pages = stub->size / PAGE_SIZE;
    u64 guest_test_phys_mem = (vm->max_gfn - used_pages - guest_num_pages) * PAGE_SIZE;

    // map guest memory
    vm_userspace_mem_region_add(vm, DEFAULT_VM_MEM_SRC, guest_test_phys_mem, next_slot,
                                guest_num_pages, 0);
    virt_map(vm, stub->base_addr, guest_test_phys_mem, guest_num_pages);
    used_pages += guest_num_pages;
    ++next_slot;

    // copy the data to the guest
    u64 *addr = addr_gpa2hva(vm, (vm_paddr_t) guest_test_phys_mem);
    memcpy(addr, stub->base_ptr, stub->size);
}

int test(void) {
    printf("Run hopping test\n");

    // prepare VM
    struct kvm_vcpu *vcpu;
    struct kvm_vm *vm;

    printf("Create VM\n");
    // VM wants to know total extra memory for page tables
    // * 4 * 512 is needed cause the utils assume consecutive memory pages
    u64 extra_pages = NUM_ADDITINAL_PAGES * 2 * (2 * 512);
    vm = __vm_create_with_one_vcpu(&vcpu, extra_pages, guest_main);
    // start up vm (until printf)
    run_vcpu(vcpu);

    // allocate some code
    jita_ctxt_t jita = jita_init();
    jita_push_psnip(&jita, &test_dst);
    stub_t stub = stub_init();
    u64 train_dst_addr = rand47();
    jita_allocate(&jita, &stub, train_dst_addr);

    // also map the code into the VM
    copy_to_guest(vm, &stub);

    printf("send state information to VM\n");
    // inform the guest about the current configuration
    registers.code_addr = train_dst_addr;
    sync_global_to_guest(vm, registers);

    // Host user
    printf("run in host user\n");
    // do_call(&registers);
    rup_call(registers.code_ptr, NULL);

    // Host kernel
    printf("run in host kernel\n");
    rap_call(registers.code_ptr, NULL);

    // Guest kernel or supervisor?
    printf("run in guest\n");
    run_vcpu(vcpu);

    // TODO: Guest kernel or supervisor?

    // cleanup
    jita_deallocate(&jita, &stub);
    kvm_vm_free(vm);

    return 0;
}

int main(void) {
    srandom(getpid());

    rap_init();
    pi_init();

    test();

    rap_deinit();
    pi_init();

    printf("done\n");
}
