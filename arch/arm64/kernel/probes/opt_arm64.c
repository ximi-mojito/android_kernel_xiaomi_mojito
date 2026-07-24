// SPDX-License-Identifier: GPL-2.0-only
/*
 * Code for Kernel probes Jump optimization.
 *
 * Copyright (C) 2021 Hisilicon Limited
 */

#include <linux/jump_label.h>
#include <linux/kprobes.h>

#include <asm/cacheflush.h>
#include <asm/compiler.h>
#include <asm/insn.h>
#include <asm/kprobes.h>
#include <asm/patching.h>

#define OPTPROBE_BATCH_SIZE 64
#define GET_LO_VAL(val)		FIELD_GET(GENMASK(31, 0), val)
#define GET_HI_VAL(val)		FIELD_GET(GENMASK(63, 32), val)

#define TMPL_VAL_IDX \
	(optprobe_template_val - optprobe_template_entry)
#define TMPL_CALL_COMMON \
	(optprobe_template_common - optprobe_template_entry)
#define TMPL_RESTORE_ORIGN_INSN \
	(optprobe_template_restore_orig_insn - optprobe_template_entry)
#define TMPL_RESTORE_END \
	(optprobe_template_restore_end - optprobe_template_entry)
#define TMPL_END_IDX \
	(optprobe_template_end - optprobe_template_entry)

static bool insn_page_in_use;

void *alloc_optinsn_page(void)
{
	if (insn_page_in_use)
		return NULL;
	insn_page_in_use = true;
	return &optinsn_slot;
}

void free_optinsn_page(void *page)
{
	insn_page_in_use = false;
}

int arch_check_optimized_kprobe(struct optimized_kprobe *op)
{
	return 0;
}

int arch_prepared_optinsn(struct arch_optimized_insn *optinsn)
{
	return optinsn->trampoline != NULL;
}

int arch_within_optimized_kprobe(struct optimized_kprobe *op, kprobe_opcode_t *addr)
{
	return op->kp.addr == addr;
}

static void optprobe_set_pc_value(struct optimized_kprobe *op, struct pt_regs *regs)
{
	regs->pc = (unsigned long)op->kp.addr;
}

static int optprobe_check_branch_limit(unsigned long pc, unsigned long addr)
{
	long offset;

	if ((pc & 0x3) || (addr & 0x3))
		return -ERANGE;

	offset = (long)addr - (long)pc;
	if (offset < -SZ_128M || offset >= SZ_128M)
		return -ERANGE;

	return 0;
}

int arch_prepare_optimized_kprobe(struct optimized_kprobe *op, struct kprobe *orig)
{
	kprobe_opcode_t *code, *buf;
	u32 insn;
	int ret = -ENOMEM;
	int i;

	buf = kcalloc(MAX_OPTINSN_SIZE, sizeof(kprobe_opcode_t), GFP_KERNEL);
	if (!buf)
		return ret;

	code = get_optinsn_slot();
	if (!code)
		goto out;

	if (optprobe_check_branch_limit((unsigned long)code, (unsigned long)orig->addr + 8)) {
		ret = -ERANGE;
		goto error;
	}

	op->set_pc = optprobe_set_pc_value;
	memcpy(buf, optprobe_template_entry, MAX_OPTINSN_SIZE * sizeof(kprobe_opcode_t));

	insn = aarch64_insn_gen_branch_imm((unsigned long)&code[TMPL_CALL_COMMON],
					   (unsigned long)&optprobe_common,
					   AARCH64_INSN_BRANCH_LINK);
	buf[TMPL_CALL_COMMON] = insn;

	insn = aarch64_insn_gen_branch_imm((unsigned long)&code[TMPL_RESTORE_END],
					   (unsigned long)op->kp.addr + 4,
					   AARCH64_INSN_BRANCH_NOLINK);
	buf[TMPL_RESTORE_END] = insn;

	buf[TMPL_VAL_IDX] = cpu_to_le32(GET_LO_VAL((unsigned long)op));
	buf[TMPL_VAL_IDX + 1] = cpu_to_le32(GET_HI_VAL((unsigned long)op));
	buf[TMPL_RESTORE_ORIGN_INSN] = orig->opcode;

	/* Setup template */
	for (i = 0; i < MAX_OPTINSN_SIZE; i++)
		aarch64_insn_patch_text_nosync(code + i, buf[i]);

	flush_icache_range((unsigned long)code, (unsigned long)(&code[TMPL_VAL_IDX]));
	/* Set op->optinsn.trampoline means prepared. */
	op->optinsn.trampoline = code;

out:
	kfree(buf);
	return ret;

error:
	free_optinsn_slot(code, 0);
	goto out;
}

void arch_optimize_kprobes(struct list_head *oplist)
{
	struct optimized_kprobe *op, *tmp;
	kprobe_opcode_t insns[OPTPROBE_BATCH_SIZE];
	void *addrs[OPTPROBE_BATCH_SIZE];
	int i = 0;

	list_for_each_entry_safe(op, tmp, oplist, list) {
		WARN_ON(kprobe_disabled(&op->kp));

		/*
		 * Backup instructions which will be replaced
		 * by jump address
		 */
		memcpy(op->optinsn.orig_insn, op->kp.addr, AARCH64_INSN_SIZE);

		addrs[i] = (void *)op->kp.addr;
		insns[i] = aarch64_insn_gen_branch_imm((unsigned long)op->kp.addr,
						       (unsigned long)op->optinsn.trampoline,
						       AARCH64_INSN_BRANCH_NOLINK);

		list_del_init(&op->list);
		if (++i == OPTPROBE_BATCH_SIZE)
			break;
	}

	aarch64_insn_patch_text(addrs, insns, i);
}

void arch_unoptimize_kprobe(struct optimized_kprobe *op)
{
	arch_arm_kprobe(&op->kp);
}

/*
 * Recover original instructions and breakpoints from relative jumps.
 * Caller must call with locking kprobe_mutex.
 */
void arch_unoptimize_kprobes(struct list_head *oplist,
			    struct list_head *done_list)
{
	struct optimized_kprobe *op, *tmp;
	kprobe_opcode_t insns[OPTPROBE_BATCH_SIZE];
	void *addrs[OPTPROBE_BATCH_SIZE];
	int i = 0;

	list_for_each_entry_safe(op, tmp, oplist, list) {
		addrs[i] = (void *)op->kp.addr;
		insns[i] = BRK64_OPCODE_KPROBES;
		list_move(&op->list, done_list);

		if (++i == OPTPROBE_BATCH_SIZE)
			break;
	}

	aarch64_insn_patch_text(addrs, insns, i);
}

void arch_remove_optimized_kprobe(struct optimized_kprobe *op)
{
	if (op->optinsn.trampoline) {
		free_optinsn_slot(op->optinsn.trampoline, 1);
		op->optinsn.trampoline = NULL;
	}
}
