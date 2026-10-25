// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 \xx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

// heapified kprobe registration, original from https://github.com/tiann/KernelSU/commit/4593ae81c
static struct kprobe *init_kprobe(const char *name, kprobe_pre_handler_t handler)
{
	struct kprobe *kp = kzalloc(sizeof(struct kprobe), GFP_KERNEL | __GFP_NOFAIL);
	kp->symbol_name = name;
	kp->pre_handler = handler;

	int ret = register_kprobe(kp);
	pr_info("%s: register %s kprobe: %d\n", __func__, name, ret);
	if (ret) {
		kfree(kp);
		return nullptr;
	}

	return kp;
}

static void destroy_kprobe(struct kprobe **kp_ptr)
{
	struct kprobe *kp = *kp_ptr;
	if (!kp)
		return;

	unregister_kprobe(kp);
	synchronize_rcu();
	kfree(kp);
	*kp_ptr = nullptr;
}

static struct kprobe *kp_find_user = nullptr;

// struct user_struct *find_user(kuid_t uid)
static int kp_find_user_pre(struct kprobe *p, struct pt_regs *regs)
{
	kuid_t *p_uid = (kuid_t *)&PT_REGS_PARM1(regs);

	if (getpriority_should_swap(*(int *)p_uid))
		*(int *)p_uid = HIDDEN_PRIORITY_UID;

	return 0;
}

static __init int getpriority_kp_init(void)
{
	kp_find_user = init_kprobe("find_user", (kprobe_pre_handler_t)kp_find_user_pre);
	if (!kp_find_user)
		return -EAGAIN;

	pr_info("%s: kp armed: 0x%lx \n", __func__, (uintptr_t)kp_find_user);
	return 0;
}

static int getpriority_kp_exit(void)
{
	if (!!kp_find_user)
		pr_info("%s: disarming kp: 0x%lx \n", __func__, (uintptr_t)kp_find_user);

	destroy_kprobe(&kp_find_user);
	return 0;
}
