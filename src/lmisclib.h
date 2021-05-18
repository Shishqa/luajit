/*
** Miscellaneous public C API extensions.
**
** Major portions taken verbatim or adapted from the LuaVela.
** Copyright (C) 2015-2019 IPONWEB Ltd.
*/

#ifndef _LMISCLIB_H
#define _LMISCLIB_H

#include "lua.h"

/* API for obtaining various platform metrics. */

struct luam_Metrics {
  /*
  ** Number of strings being interned (i.e. the string with the
  ** same payload is found, so a new one is not created/allocated).
  */
  size_t strhash_hit;
  /* Total number of strings allocations during the platform lifetime. */
  size_t strhash_miss;

  /* Amount of allocated string objects. */
  size_t gc_strnum;
  /* Amount of allocated table objects. */
  size_t gc_tabnum;
  /* Amount of allocated udata objects. */
  size_t gc_udatanum;
  /* Amount of allocated cdata objects. */
  size_t gc_cdatanum;

  /* Memory currently allocated. */
  size_t gc_total;
  /* Total amount of freed memory. */
  size_t gc_freed;
  /* Total amount of allocated memory. */
  size_t gc_allocated;

  /* Count of incremental GC steps per state. */
  size_t gc_steps_pause;
  size_t gc_steps_propagate;
  size_t gc_steps_atomic;
  size_t gc_steps_sweepstring;
  size_t gc_steps_sweep;
  size_t gc_steps_finalize;

  /*
  ** Overall number of snap restores (amount of guard assertions
  ** leading to stopping trace executions).
  */
  size_t jit_snap_restore;
  /* Overall number of abort traces. */
  size_t jit_trace_abort;
  /* Total size of all allocated machine code areas. */
  size_t jit_mcode_size;
  /* Amount of JIT traces. */
  unsigned int jit_trace_num;
};

LUAMISC_API void luaM_metrics(lua_State *L, struct luam_Metrics *metrics);

/* --- Sysprof ------------------------------------------------------------- */

enum luam_sysprof_vmstate {
  LUAM_VMST_INTERP,
  LUAM_VMST_LFUNC,
  LUAM_VMST_FFUNC,
  LUAM_VMST_CFUNC,
  LUAM_VMST_GC,
  LUAM_VMST_EXIT,
  LUAM_VMST_RECORD,
  LUAM_VMST_OPT,
  LUAM_VMST_ASM,
  LUAM_VMST_TRACE,
  /* Number of vmstates */
  LUAM_VMST_COUNT
};

struct luam_sysprof_data {
  /* Number of samples collected */
  uint64_t samples;
  /* Timer overrun */
  uint64_t overruns;
  /* Vmstate counters */
  uint64_t vmstate[LUAM_VMST_COUNT];
};

/* Profiling mode:
** > PROFILE_DEFAULT collects only data described in lj_sysprof_data,
**   which can be collected with lj_sysprof_report after profiler stops.
**   This mode doesn't use i/o for dumping data.
** > PROFILE_LEAF collects counters described above and top frames of
**   both guest and host stacks
** > PROFILE_CALLGRAPH collects counters described above and callchains
**   for both guest and host
**
**  Collected counters can be received via luaM_sysprof_report. Callchains
**  are streamed to the file, specified in luam_sysprof_options and then
**  can be parsed with luajit-parse-sysprof utility
*/
enum luam_sysprof_mode {
  PROFILE_DEFAULT,
  PROFILE_LEAF,
  PROFILE_CALLGRAPH
};

/* Profiler options */
struct luam_sysprof_options {
  /* Sampling interval in msec */
  uint32_t interval;
  /* Profiling mode */
  enum luam_sysprof_mode mode;
  /* Output path */
  const char *path;
};

enum luam_sysprof_err {
  SYSPROF_SUCCESS = 0,
  SYSPROF_ERRUSE, 
  SYSPROF_ERRRUN, 
  SYSPROF_ERRIO, 
};

LUAMISC_API int luaM_sysprof_start(lua_State *L,
                                   const struct luam_sysprof_options *opt);

LUAMISC_API int luaM_sysprof_stop(lua_State *L);

LUAMISC_API void luaM_sysprof_report(struct luam_sysprof_data *data);

#define LUAM_MISCLIBNAME "misc"
LUALIB_API int luaopen_misc(lua_State *L);

#endif /* _LMISCLIB_H */
