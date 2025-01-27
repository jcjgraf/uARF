/**
 * Automatic IBRS BTI Guest
 *
 * That is the impact of Automatic IBRS on BTI considering the guest? Is is still possible?
 *
 * We consider the attack vectors:
 *  - G -> G
 *
 */

#include "uarf/flush_reload.h"
#include "uarf/jita.h"
#include "uarf/kmod/pi.h"
#include "uarf/kmod/rap.h"
#include "uarf/lib.h"
#include "uarf/log.h"
#include "uarf/spec_lib.h"
#include "uarf/test.h"

#include <ucall_common.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_TEST
#endif

psnip_declare(history, psnip_history);
psnip_declare(src_call_ind, psnip_src_call_ind);
psnip_declare(src_jmp_ind, psnip_src_jmp_ind);
psnip_declare(dst_gadget, psnip_dst_gadget);
psnip_declare(dst_dummy, psnip_dst_dummy);

psnip_declare(exp_guest_bti_jmp, psnip_jmp);

psnip_declare_define(psnip_test, "ret\n\t");

struct TestCaseData {
	uint32_t seed;
	jita_ctxt_t *jita_main;
	jita_ctxt_t *jita_gadget;
	jita_ctxt_t *jita_dummy;
};

struct GuestData {
	union {
		void *code_ptr;
		uint64_t code_addr;
		void (*f)(void);
	};
	struct SpecData spec_data;
};

// Out globals that we also need in our guest
struct GuestData guestData;

stub_t stub_main;
stub_t stub_gadget;
stub_t stub_dummy;

static void run_spec_global(void)
{
	struct SpecData *data = &guestData.spec_data;

	// Ensure we have access to the required data
	// It easier to debug if we fail here that later
	GUEST_ASSERT(*(uint64_t *)data->spec_prim_p || true);
	GUEST_ASSERT(*(uint64_t *)data->spec_dst_p_p || true);
	GUEST_ASSERT(**(uint64_t **)data->spec_dst_p_p || true);
	GUEST_ASSERT(*(uint64_t *)data->fr_buf_p || true);

	asm volatile("lea return_here%=, %%rax\n\t"
		     "pushq %%rax\n\t"
		     "jmp *%0\n\t"
		     "return_here%=:\n\t" ::"r"(data->spec_prim_p),
		     "c"(data)
		     : "rax", "rdx", "rdi", "rsi", "r8", "memory");
}

static void guest_main(void)
{
	GUEST_PRINTF("Guest started\n");

	run_spec_global();

	GUEST_PRINTF("Spec done\n");
	GUEST_DONE();
}

void map_mem_to_guest(struct kvm_vm *vm, uint64_t va, uint64_t size)
{
	LOG_TRACE("(vm: %p, va: 0x%lx, size: %lu)\n", vm, va, size);
	static uint64_t next_slot = NR_MEM_REGIONS;
	static uint64_t used_pages = 0;

	uint64_t va_aligned = ALIGN_DOWN(va, PAGE_SIZE);
	LOG_DEBUG("Aligned 0x%lx to 0x%lx\n", va, va_aligned);

	uint64_t num_pages_to_map = DIV_ROUND_UP(size, PAGE_SIZE);
	uint64_t gpa =
		(vm->max_gfn - used_pages - num_pages_to_map) * PAGE_SIZE;

	LOG_DEBUG(
		"Add %lu pages (required: %lu B) of physical memory to guest at gpa 0x%lx (slot %lu)\n",
		num_pages_to_map, size, gpa, next_slot);
	vm_userspace_mem_region_add(vm, DEFAULT_VM_MEM_SRC, gpa, next_slot,
				    num_pages_to_map, 0);

	LOG_DEBUG("Add mapping in guest for gpa 0x%lx to gva 0x%lx\n", gpa,
		  va_aligned);
	virt_map(vm, va_aligned, gpa, num_pages_to_map);
	used_pages += num_pages_to_map;
	next_slot++;

	uint64_t *addr = addr_gpa2hva(vm, (vm_paddr_t)gpa);
	LOG_DEBUG("Copy %lu bytes from 0x%lx to 0x%lx\n", size, va, _ul(addr));
	memcpy(addr, _ptr(va), size);
}

void map_stub_to_guest(struct kvm_vm *vm, stub_t *stub)
{
	LOG_TRACE("(vm: %p, stub: %p)\n", vm, stub);
	map_mem_to_guest(vm, stub->base_addr, stub->size);
}

static void run_vcpu(struct kvm_vcpu *vcpu)
{
	LOG_TRACE("(vcpu: %p)\n", vcpu);
	struct ucall uc;

	while (true) {
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
			TEST_FAIL("Unhandled ucall: %ld\nexit_reason: %u (%s)",
				  uc.cmd, vcpu->run->exit_reason,
				  exit_reason_str(vcpu->run->exit_reason));
		}
	}
}

TEST_CASE_ARG(basic, arg)
{
	struct TestCaseData *data = (struct TestCaseData *)arg;
	srand(data->seed);

	struct kvm_vcpu *vcpu;
	struct kvm_vm *vm;

	uint64_t extra_pages = 10 * (3 + 512);

	// struct FrConfig fr = fr_init(8, 6, (size_t[]){0, 1, 2, 3, 5, 10});
	struct FrConfig fr = fr_init(8, 1, NULL);

	stub_main = stub_init();
	stub_gadget = stub_init();
	stub_dummy = stub_init();

	IBPB();

	AUTO_IBRS_ON();
	// AUTO_IBRS_OFF();

	fr_reset(&fr);

	jita_allocate(data->jita_main, &stub_main, rand47());
	jita_allocate(data->jita_gadget, &stub_gadget, rand47());
	jita_allocate(data->jita_dummy, &stub_dummy, rand47());

	struct history h1 = get_randomized_history();

	struct SpecData spec_data = {
		.spec_prim_p = stub_main.addr,
		.spec_dst_p_p = _ul(&stub_gadget.addr),
		.fr_buf_p = fr.buf2.addr,
		.secret = 0,
		.hist = h1,
	};

	// Create VM
	vm = __vm_create_with_one_vcpu(&vcpu, extra_pages, guest_main);

	guestData.spec_data = spec_data;

	// Sync globals
	sync_global_to_guest(vm, guestData);
	sync_global_to_guest(vm, stub_main);
	sync_global_to_guest(vm, stub_gadget);
	sync_global_to_guest(vm, stub_dummy);

	// Map stubs
	map_stub_to_guest(vm, &stub_main);
	map_stub_to_guest(vm, &stub_gadget);
	map_stub_to_guest(vm, &stub_dummy);

	// Map other memory
	map_mem_to_guest(vm, fr.buf.base_addr, fr.buf_size);
	map_mem_to_guest(vm, fr.buf2.base_addr, fr.buf_size);
	map_mem_to_guest(vm, fr.res_addr, fr.res_size);

	// Run VM
	LOG_INFO("Starting VM\n");
	run_vcpu(vcpu);
	// run_spec_global();

	// Cleanup
	kvm_vm_free(vm);

	fr_flush(&fr);

	fr_reload(&fr);
	jita_deallocate(data->jita_main, &stub_main);
	jita_deallocate(data->jita_gadget, &stub_gadget);
	jita_deallocate(data->jita_dummy, &stub_dummy);

	LOG_INFO("Done\n");

	fr_print(&fr);

	fr_deinit(&fr);

	TEST_PASS();
}

static struct TestCaseData data1;

TEST_SUITE()
{
	uint32_t seed = get_seed();
	LOG_INFO("Using seed: %u\n", seed);
	pi_init();
	rap_init();

	jita_ctxt_t jita_main = jita_init();
	jita_ctxt_t jita_gadget = jita_init();
	jita_ctxt_t jita_dummy = jita_init();

	// jita_push_psnip(&jita_main_jmp, &psnip_history);
	// jita_push_psnip(&jita_main_jmp, &psnip_history);
	// jita_push_psnip(&jita_main_jmp, &psnip_history);
	// jita_push_psnip(&jita_main_jmp, &psnip_history);
	// jita_push_psnip(&jita_main_jmp, &psnip_history);

	// jita_clone(&jita_main_jmp, &jita_main_call);

	// jita_push_psnip(&jita_main_jmp, &psnip_src_call_ind);
	// jita_push_psnip(&jita_main, &psnip_src_jmp_ind);

	jita_push_psnip(&jita_main, &psnip_jmp);

	jita_push_psnip(&jita_gadget, &psnip_dst_gadget);
	jita_push_psnip(&jita_dummy, &psnip_dst_dummy);

	data1 = (struct TestCaseData){
		.seed = seed++,
		.jita_main = &jita_main,
		.jita_gadget = &jita_gadget,
		.jita_dummy = &jita_dummy,
	};

	RUN_TEST_CASE_ARG(basic, &data1, "UU, jmp");

	pi_deinit();
	rap_deinit();

	return 0;
}
