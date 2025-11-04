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

#endif /* _TRACE_GPU_POWER_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE gpu_power
#include <trace/define_trace.h>
