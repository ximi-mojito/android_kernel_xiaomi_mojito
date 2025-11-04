/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM power // just "power" for historical reasons

#if !defined(_TRACE_GPU_POWER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_GPU_POWER_H

#include <linux/tracepoint.h>

TRACE_EVENT(gpu_frequency,

	TP_PROTO(unsigned int state, unsigned int gpu_id),

	TP_ARGS(state, gpu_id),

	TP_STRUCT__entry(
		__field(unsigned int, state)
		__field(unsigned int, gpu_id)
	),

	TP_fast_assign(
		__entry->state = state;
		__entry->gpu_id = gpu_id;
	),

	TP_printk("state=%u gpu_id=%u", __entry->state, __entry->gpu_id)
);

TRACE_EVENT(gpu_work_period,

	TP_PROTO(u32 gpu_id, u32 uid, u64 start_time_ns, u64 end_time_ns,
		u64 total_active_duration_ns),

	TP_ARGS(gpu_id, uid, start_time_ns, end_time_ns, total_active_duration_ns),

	TP_STRUCT__entry(
		__field(u32, gpu_id)
		__field(u32, uid)
		__field(u64, start_time_ns)
		__field(u64, end_time_ns)
		__field(u64, total_active_duration_ns)
	),

	TP_fast_assign(
		__entry->gpu_id = gpu_id;
		__entry->uid = uid;
		__entry->start_time_ns = start_time_ns;
		__entry->end_time_ns = end_time_ns;
		__entry->total_active_duration_ns = total_active_duration_ns;
	),

	TP_printk("gpu_id=%u uid=%u start_time_ns=%llu end_time_ns=%llu total_active_duration_ns=%llu",
		__entry->gpu_id,
		__entry->uid,
		__entry->start_time_ns,
		__entry->end_time_ns,
		__entry->total_active_duration_ns)
);

#endif /* _TRACE_GPU_POWER_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE gpu_power
#include <trace/define_trace.h>
