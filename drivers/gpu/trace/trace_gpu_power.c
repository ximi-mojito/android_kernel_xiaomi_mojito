// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2025 Google LLC
 */

#include <linux/module.h>

#define CREATE_TRACE_POINTS
#include <trace/events/gpu_power.h>

EXPORT_TRACEPOINT_SYMBOL(gpu_frequency);
