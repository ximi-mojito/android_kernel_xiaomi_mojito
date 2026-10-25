// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 \xx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Winfinite-recursion"

#if defined(__GNUC__) && !defined(__clang__)
#define KEEP_SYMBOL_NO_TAIL __attribute__((used)) noinline __attribute__((optimize("no-optimize-sibling-calls")))
#else
#define KEEP_SYMBOL_NO_TAIL __attribute__((used)) noinline __attribute__((disable_tail_calls))
#endif

#ifdef MODULE /* stub for free_uid() */
__asm__ (
	"\n .text			"
	"\n .p2align 2			"
	"\n .global gp_free_uid		"
	"\n gp_free_uid:		"
	"\n brk #1			"
);					
KEEP_SYMBOL_NO_TAIL void gp_free_uid(struct user_struct *up);

#else /* ! MODULE */
extern void free_uid(struct user_struct *);
#define gp_free_uid free_uid
#endif /* ! MODULE */

KEEP_SYMBOL_NO_TAIL struct user_struct *gphook_find_user(kuid_t uid)
{
	// we can actually return early if we want to
	// but, this makes sure everything does the same work.
	struct user_struct *ret = gphook_find_user(uid);
	if (getpriority_should_swap(*(int *)&uid)) {
		gp_free_uid(ret);
		return nullptr;
	}

	return ret;
}

#pragma GCC diagnostic pop
#undef KEEP_SYMBOL_NO_TAIL

#define estimate_sz	(256 * 4)

#ifdef CONFIG_MODULES
#define kernel_function_lookup(name) kallsyms_lookup_name(#name)
#else
#define kernel_function_lookup(name) ({ 						\
	pr_info("kernel_function_lookup: %s addr: 0x%lx\n", #name, (uintptr_t)&name);	\
	(uintptr_t)&name; 								\
})
#endif

#define self_patch_hook(hook_fn, tgt) ({					\
	uintptr_t hook_p = kernel_function_lookup(hook_fn);			\
	int ret = arm64_b_or_bl_patch(hook_p, get_ksym_size(hook_p, estimate_sz), hook_p, tgt);	\
	pr_info("patch_hook: %s ret: %d \n", #hook_fn, ret);			\
	ret;									\
})

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
#define syscall_lookup(name) kallsyms_lookup_name("__arm64_" name)
#else
#define syscall_lookup(name) kallsyms_lookup_name(name)
#endif

static __init int getpriority_hook_init(void)
{
	uintptr_t symbol_addr = kallsyms_lookup_name("find_user");
	if (!symbol_addr)
		return -ENOENT;

	int ret = self_patch_hook(gphook_find_user, symbol_addr);
	if (ret)
		return ret;

#ifdef MODULE
	uintptr_t symbol_addr2 = kallsyms_lookup_name("free_uid");
	if (!symbol_addr2)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(gphook_find_user, get_ksym_size(gphook_find_user, estimate_sz), kernel_function_lookup(gp_free_uid), (uintptr_t)symbol_addr2);
	if (ret)
		return ret;
#endif

	// now patch callsites
	uintptr_t callsite = syscall_lookup("sys_getpriority");
	if (!callsite)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), symbol_addr, (uintptr_t)&gphook_find_user);
	if (ret)
		return ret;

	callsite = syscall_lookup("sys_setpriority");
	if (!callsite)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), symbol_addr, (uintptr_t)&gphook_find_user);
	if (ret)
		return ret;

	callsite = syscall_lookup("sys_ioprio_set");
	if (!callsite)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), symbol_addr, (uintptr_t)&gphook_find_user);
	if (ret)
		return ret;

	callsite = syscall_lookup("sys_ioprio_get");
	if (!callsite)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), symbol_addr, (uintptr_t)&gphook_find_user);
	if (ret)
		return ret;

	pr_info("%s: ok! \n", __func__);
	return ret;
}

static int getpriority_hook_exit(void)
{
	uintptr_t symbol_addr = kallsyms_lookup_name("find_user");
	if (!symbol_addr)
		return -ENOENT;

	uintptr_t hook_addr = (uintptr_t)&gphook_find_user;
	if (!hook_addr)
		return -ENOENT;

	// restore callsites
	uintptr_t callsite = syscall_lookup("sys_getpriority");
	if (!callsite)
		return -ENOENT;

	int ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), hook_addr, symbol_addr);
	if (ret)
		return ret;

	callsite = syscall_lookup("sys_setpriority");
	if (!callsite)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), hook_addr, symbol_addr);
	if (ret)
		return ret;

	callsite = syscall_lookup("sys_ioprio_set");
	if (!callsite)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), hook_addr, symbol_addr);
	if (ret)
		return ret;

	callsite = syscall_lookup("sys_ioprio_get");
	if (!callsite)
		return -ENOENT;

	ret = arm64_b_or_bl_patch(callsite, get_ksym_size(callsite, estimate_sz), hook_addr, symbol_addr);
	if (ret)
		return ret;

	return 0;
}
