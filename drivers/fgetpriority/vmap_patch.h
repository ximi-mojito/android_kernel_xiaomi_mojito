// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 \xx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#pragma once

// vmap_patch.h safely write to read-only fn_ptr slots or syscall table.

/**
 * normally we want this on stop_machine, but we don't need HARD synchronization
 * we just let !cpuX use what they have in cache while we patch what is on cpuX.
 * not a big deal. this is safe since, pointer-write is atomic and the old ptr
 * still exists while in-patching.
 *
 * then we can just kick all the cpus, so they see the change and refetch.
 */
static inline void patch_ptr_slot_kick_cpu(void **target_slot, void *new_ptr)
{
	WRITE_ONCE(*target_slot, new_ptr);
	kick_all_cpus_sync();
}

static int write_to_readonly_slot(uintptr_t slot_ptr, uintptr_t new_ptr)
{
	if (!slot_ptr || !new_ptr)
		return -EINVAL;

	uintptr_t addr = slot_ptr;
	uintptr_t base = addr & PAGE_MASK;
	uintptr_t offset = addr & ~PAGE_MASK;

	struct page *page = phys_to_page(__pa(base));
	if (!page)
		return -EFAULT;

	void *writable_addr = vmap(&page, 1, VM_MAP, PAGE_KERNEL);
	if (!writable_addr)
		return -ENOMEM;

	void **target_slot = (void **)((uintptr_t)writable_addr + offset);

	patch_ptr_slot_kick_cpu(target_slot, (void *)new_ptr);

	vunmap(writable_addr);
	smp_mb();

	return 0;
}
