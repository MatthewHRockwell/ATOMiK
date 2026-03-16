// SPDX-License-Identifier: GPL-2.0
/*
 * atomik_trace_define.c — Tracepoint instantiation
 *
 * This file creates the actual tracepoint implementations.
 * Must be compiled exactly once (included via Kbuild).
 * Other files that call trace_*() include atomik_trace.h
 * without CREATE_TRACE_POINTS.
 */

#define CREATE_TRACE_POINTS
#include "atomik_trace.h"
