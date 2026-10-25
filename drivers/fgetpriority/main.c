// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 \xx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "kernel.h"

#if !defined(__aarch64__)
static_assert(1 == 0, "ARM64 only!");
#endif

#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "fgetpriority: " fmt
#endif

#ifdef MODULE /* overrides for LKM */
static uintptr_t kallsyms_lookup_name_fn = 0x0;
static __nocfi unsigned long __kallsyms_lookup_name(const char *name)
{
	return ((typeof (__kallsyms_lookup_name) *)kallsyms_lookup_name_fn)(name);
}
#define kallsyms_lookup_name __kallsyms_lookup_name

static int bootstrap_kallsyms_lookup_name(uintptr_t ptr)
{
	extern long copy_from_kernel_nofault(void *dst, const void *src, size_t size);

	constexpr int max_iter = 100000;
	constexpr char sym[] = "kallsyms_lookup_name+0x0/";
	uintptr_t anchor = ptr;

	char buf[KSYM_SYMBOL_LEN];
	uint32_t ibuf;
	int iter = 0;

	uintptr_t curr = ptr;
	bool up = true;

	/**
	 * radiating scan, expect some cache trashing.
	 * however for this one it is worth it.
	 */
start_r:
	if (iter >= max_iter) {
		pr_info("%s: bootsrap fail! iter: %d\n", __func__, iter);
		return -ERANGE;
	}

	if (!!copy_from_kernel_nofault(&ibuf, (void *)curr, sizeof(uint32_t)))
		goto inc;

	if (sprint_symbol(buf, curr) < sizeof(sym))
		goto inc;

	if (strstarts(buf, sym))
		goto found;

inc:
	if (up) {
		iter++;
		curr = anchor + sizeof(uint32_t) * iter;
		up = false;
	} else {
		curr = anchor - sizeof(uint32_t) * iter;
		up = true;
	}

	goto start_r;

found:
	pr_info("%s: %s found at 0x%lx iter: %d\n", __func__, buf, curr, iter);
	kallsyms_lookup_name_fn = curr;
	return 0;
}

static __nocfi int __aarch64_insn_patch_text(void *addrs[], u32 insns[], int cnt)
{
	typeof(__aarch64_insn_patch_text) *fn = kallsyms_lookup_name("aarch64_insn_patch_text");
	if (!fn)
		return -ENOENT;
	return fn(addrs, insns, cnt);
}
#define aarch64_insn_patch_text __aarch64_insn_patch_text

static __nocfi u32 __aarch64_insn_gen_branch_imm(unsigned long pc, unsigned long addr, enum aarch64_insn_branch_type type)
{
	typeof(__aarch64_insn_gen_branch_imm) *fn = kallsyms_lookup_name("aarch64_insn_gen_branch_imm");
	if (!fn)
		return -ENOENT;
	return fn(pc, addr, type);
}
#define aarch64_insn_gen_branch_imm __aarch64_insn_gen_branch_imm

/**
 *  kallsyms_lookup_size_offset from sprint_symbol actually
 */
static int compat_kallsyms_lookup_size_offset(unsigned long addr, unsigned long *symbolsize, unsigned long *offset)
{
	char buf[KSYM_SYMBOL_LEN];
	unsigned long u_sz;
	unsigned long u_off;

	if (!addr || !symbolsize || !offset)
		return 0;

	if (!(sprint_symbol(buf, addr) > 0))
		return 0;		
	
//	pr_info("symbol is: %s \n", buf);

	char *c_sz = strchr(buf, '/');
	if (!c_sz)
		return 0;		

	if (kstrtoul(c_sz + 1, 0, &u_sz))
		return 0;		

	// null '/', strtok called back.
	c_sz[0] = 0;

	char *c_off = strchr(buf, '+');
	if (!c_off)
		return 0;	

	if (kstrtoul(c_off + 1, 0, &u_off))
		return 0;		

//	pr_info("symbol_size: %zu \n", u_sz);
//	pr_info("symbol_offset: %zu \n", u_off);

	*symbolsize = u_sz;
	*offset = u_off;

	return 1;
}
#define kallsyms_lookup_size_offset compat_kallsyms_lookup_size_offset

static struct kobject *mod_kobj_parent_backup = nullptr;
#endif // MODULE

// override kprobe reg and unreg
static __nocfi int __register_kprobe(struct kprobe *p)
{
	typeof(__register_kprobe) *fn = kallsyms_lookup_name("register_kprobe");
	if (!fn)
		return -ENOENT;
	return fn(p);
}
#define register_kprobe __register_kprobe

static __nocfi void __unregister_kprobe(struct kprobe *p)
{
	typeof(__unregister_kprobe) *fn = kallsyms_lookup_name("unregister_kprobe");
	if (!fn)
		return;
	return fn(p);
}
#define unregister_kprobe __unregister_kprobe

static size_t get_ksym_size(uintptr_t symbol_addr, size_t retfail)
{
	unsigned long offset = 0;
	unsigned long symbolsize = 0;

	// means lookup failed, just return retfail
	if (!kallsyms_lookup_size_offset(symbol_addr, &symbolsize, &offset))
		return retfail;		

	if (!symbolsize && !offset)
		return retfail;

	return symbolsize;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0) // probe_kernel_read
__weak long copy_from_kernel_nofault(void *dst, const void *src, size_t size)
{
	// https://elixir.bootlin.com/linux/v5.2.21/source/mm/maccess.c#L27
	long ret;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	pagefault_disable();
	ret = __copy_from_user_inatomic(dst, (__force const void __user *)src, size);
	pagefault_enable();
	set_fs(old_fs);

	return ret ? -EFAULT : 0;
}
#endif

#include "arch.h"
#include "arm64_branch_insn.h"
#include "routine.h"
#include "routine_kp.c"
#include "routine_raw.c"

static int __init fgetpriority_init(void)
{
	pr_info("init: Hello World!\n");

#ifdef MODULE
	constexpr char sym[] = "sprint_symbol";
	uintptr_t addr = (uintptr_t)&sprint_symbol;
	pr_info("%s: %s @ 0x%lx \n", __func__, sym, addr);
	int err = bootstrap_kallsyms_lookup_name(addr);
	if (err)
		return err;
#endif

	int ret = getpriority_hook_init();
	pr_info("init: ret: %d\n", ret);
	if (!ret)
		goto done;

	pr_info("init: raw branch patching fail! falling back to kprobes!\n");
	ret = getpriority_kp_init();
	if (!ret)
		goto done;

	pr_info("init: kp fallback failed! ret: %d\n", ret);
	return ret;

done:;
#ifdef MODULE
	mod_kobj_parent_backup = THIS_MODULE->mkobj.kobj.parent;
	kobject_del(&THIS_MODULE->mkobj.kobj);
#endif
	pr_info("init: ok!\n");
	return ret;
}

static void __exit fgetpriority_exit(void)
{
#ifdef MODULE
	int ret = kobject_add(&THIS_MODULE->mkobj.kobj, mod_kobj_parent_backup, "%s", THIS_MODULE->name);
	pr_info("kobject_add ret: %d\n", ret);
	if (ret)
		panic("kobj add failed!\n");

	int r = getpriority_hook_exit();
	if (r)
		pr_info("%s: ret: %d \n", __func__, r);

	getpriority_kp_exit();
	pr_info("exit: ignore rmmod dmesg splat. #notabug\n");
	pr_info("exit: Goodbye!\n");
	return;

#else /* ! MODULE */
	__builtin_trap();
	__builtin_unreachable();
#endif /* ! MODULE */
}

module_init(fgetpriority_init);
module_exit(fgetpriority_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xx");
MODULE_DESCRIPTION("fgetpriority");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
MODULE_IMPORT_NS("VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver");
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
