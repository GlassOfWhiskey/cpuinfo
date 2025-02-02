#include <sched.h>
#include <cpuinfo/log.h>

#include <riscv/api.h>
#include <riscv/linux/api.h>

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)

#include <sys/hwprobe.h>

void cpuinfo_riscv_linux_decode_vendor_uarch_from_hwprobe(
		uint32_t processor,
		enum cpuinfo_vendor vendor[restrict static 1],
		enum cpuinfo_uarch uarch[restrict static 1]) {
	struct riscv_hwprobe pairs[] = {
		{ .key = RISCV_HWPROBE_KEY_MVENDORID, },
		{ .key = RISCV_HWPROBE_KEY_MARCHID, },
		{ .key = RISCV_HWPROBE_KEY_MIMPID, },
	};
	const size_t pairs_count = sizeof(pairs) / sizeof(struct riscv_hwprobe);

	/* Create a CPU set with this processor flagged. */
	const size_t cpu_count = processor + 1;
	cpu_set_t* cpu_set = CPU_ALLOC(cpu_count);
	if (cpu_set == NULL) {
		cpuinfo_log_warning("failed to allocate space for cpu_set");
		return;
	}

	const size_t cpu_set_size = CPU_ALLOC_SIZE(cpu_count);
	CPU_ZERO_S(cpu_set_size, cpu_set);
	CPU_SET_S(processor, cpu_set_size, cpu_set);

	/* Request all available information from hwprobe. */
	int ret = __riscv_hwprobe(pairs, pairs_count,
                                  cpu_set_size, (unsigned long*)cpu_set,
                                  0 /* flags */);
	if (ret < 0) {
		cpuinfo_log_warning("failed to get hwprobe information, err: %d", ret);
		goto cleanup;
	}

	/**
	 * The syscall may not have populated all requested keys, loop through
	 * the list and store the values that were discovered.
	 */
	uint32_t vendor_id = 0;
	uint32_t arch_id = 0;
	uint32_t imp_id = 0;
	for (size_t pair = 0; pair < pairs_count; pair++) {
		switch (pairs[pair].key) {
			case RISCV_HWPROBE_KEY_MVENDORID:
				vendor_id = pairs[pair].value;
				break;
			case RISCV_HWPROBE_KEY_MARCHID:
				arch_id = pairs[pair].value;
				break;
			case RISCV_HWPROBE_KEY_MIMPID:
				imp_id = pairs[pair].value;
				break;
			default:
				/* The key value may be -1 if unsupported. */
				break;
		}
	}
	cpuinfo_riscv_decode_vendor_uarch(vendor_id, arch_id, imp_id,
					  vendor, uarch);

cleanup:
	CPU_FREE(cpu_set);
}

#else

void cpuinfo_riscv_linux_decode_vendor_uarch_from_hwprobe(
		uint32_t processor,
		enum cpuinfo_vendor vendor[restrict static 1],
		enum cpuinfo_uarch uarch[restrict static 1]) {
	/* Do nothing */
}

#endif

