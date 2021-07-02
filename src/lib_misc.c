/*
** Miscellaneous Lua extensions library.
**
** Major portions taken verbatim or adapted from the LuaVela interpreter.
** Copyright (C) 2015-2019 IPONWEB Ltd.
*/

#define lib_misc_c
#define LUA_LIB

#include <errno.h>
#include <stdio.h>

#include "lauxlib.h"
#include "lj_err.h"
#include "lj_gc.h"
#include "lj_lib.h"
#include "lj_memprof.h"
#include "lj_obj.h"
#include "lj_str.h"
#include "lj_tab.h"
#include "lmisclib.h"
#include "lua.h"

/* ------------------------------------------------------------------------ */

static LJ_AINLINE void setnumfield(struct lua_State *L, GCtab *t,
                                   const char *name, int64_t val) {
  setnumV(lj_tab_setstr(L, t, lj_str_newz(L, name)), (double)val);
}

#define LJLIB_MODULE_misc

LJLIB_CF(misc_getmetrics) {
  struct luam_Metrics metrics;
  GCtab *m;

  lua_createtable(L, 0, 19);
  m = tabV(L->top - 1);

  luaM_metrics(L, &metrics);

  setnumfield(L, m, "strhash_hit", metrics.strhash_hit);
  setnumfield(L, m, "strhash_miss", metrics.strhash_miss);

  setnumfield(L, m, "gc_strnum", metrics.gc_strnum);
  setnumfield(L, m, "gc_tabnum", metrics.gc_tabnum);
  setnumfield(L, m, "gc_udatanum", metrics.gc_udatanum);
  setnumfield(L, m, "gc_cdatanum", metrics.gc_cdatanum);

  setnumfield(L, m, "gc_total", metrics.gc_total);
  setnumfield(L, m, "gc_freed", metrics.gc_freed);
  setnumfield(L, m, "gc_allocated", metrics.gc_allocated);

  setnumfield(L, m, "gc_steps_pause", metrics.gc_steps_pause);
  setnumfield(L, m, "gc_steps_propagate", metrics.gc_steps_propagate);
  setnumfield(L, m, "gc_steps_atomic", metrics.gc_steps_atomic);
  setnumfield(L, m, "gc_steps_sweepstring", metrics.gc_steps_sweepstring);
  setnumfield(L, m, "gc_steps_sweep", metrics.gc_steps_sweep);
  setnumfield(L, m, "gc_steps_finalize", metrics.gc_steps_finalize);

  setnumfield(L, m, "jit_snap_restore", metrics.jit_snap_restore);
  setnumfield(L, m, "jit_trace_abort", metrics.jit_trace_abort);
  setnumfield(L, m, "jit_mcode_size", metrics.jit_mcode_size);
  setnumfield(L, m, "jit_trace_num", metrics.jit_trace_num);

  return 1;
}

/* ------------------------------------------------------------------------ */

#include "lj_libdef.h"

/* ----- misc.sysprof module ---------------------------------------------- */
// Not loaded by default, use: local profile = require("misc.sysprof")
#define LJLIB_MODULE_misc_sysprof

static const char KEY_PROFILE_THREAD = 't';
static const char KEY_PROFILE_FUNC = 'f';

int parse_options(lua_State *L, struct luam_sysprof_options *opt) {
  GCstr *mode = lj_lib_optstr(L, 1);
  GCtab *options = lj_lib_checktab(L, 2);
  
  const char *mode_str = strdata(mode);
  if (0 == strncmp(mode_str, "D", sizeof("D") - 1)) {
    opt->mode = PROFILE_DEFAULT;
  } else if (0 == strncmp(mode_str, "L", sizeof("L") - 1)) {
    opt->mode = PROFILE_LEAF;
  } else if (0 == strncmp(mode_str, "C", sizeof("C") - 1)) {
    opt->mode = PROFILE_CALLGRAPH;
  } else {
    return SYSPROF_ERRUSE;
  }

  opt->interval = 11; // default interval
  cTValue *interval = lj_tab_getstr(options, lj_str_newlit(L, "interval"));
  if (interval) {
    opt->interval = lj_num2u64(numberVnum(interval));
  }

  opt->path = "sysprof.bin"; // default output file
  cTValue *path = lj_tab_getstr(options, lj_str_newlit(L, "path"));
  if (path) {
    opt->path = strVdata(path);
  }

  return SYSPROF_SUCCESS;
}

// sysprof.start(mode, options)
LJLIB_CF(misc_sysprof_start) {
  enum luam_sysprof_err sysprof_status = SYSPROF_SUCCESS;
  GCtab *registry = tabV(registry(L));
  TValue key;
  // Anchor thread and function in registry.
  setlightudV(&key, (void *)&KEY_PROFILE_THREAD);
  setlightudV(&key, (void *)&KEY_PROFILE_FUNC);
  lj_gc_anybarriert(L, registry);

  struct luam_sysprof_options opts = {};
  sysprof_status = parse_options(L, &opts);
  
  if (LJ_UNLIKELY(sysprof_status != PROFILE_SUCCESS)) {
    switch (sysprof_status) {
      case SYSPROF_ERRUSE:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_MISUSE));
        lua_pushinteger(L, EINVAL);
        return 3;
      default:
        lua_assert(0);
        return 0;
    }
  }

  sysprof_status = luaM_sysprof_start(L, &opts);
  
  if (LJ_UNLIKELY(sysprof_status != PROFILE_SUCCESS)) {
    switch (sysprof_status) {
      case SYSPROF_ERRUSE:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_MISUSE));
        lua_pushinteger(L, EINVAL);
        return 3;
      case SYSPROF_ERRRUN:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_ISRUNNING));
        lua_pushinteger(L, EINVAL);
        return 3;
      case SYSPROF_ERRIO:
        return luaL_fileresult(L, 0, opts.path);
      default:
        lua_assert(0);
        return 0;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

// profile.sysprof_stop()
LJLIB_CF(misc_sysprof_stop) {
  GCtab *registry;
  TValue key;
  registry = tabV(registry(L));
  setlightudV(&key, (void *)&KEY_PROFILE_THREAD);
  setnilV(lj_tab_set(L, registry, &key));
  setlightudV(&key, (void *)&KEY_PROFILE_FUNC);
  setnilV(lj_tab_set(L, registry, &key));
  lj_gc_anybarriert(L, registry);

  enum luam_sysprof_err sysprof_status = luaM_sysprof_stop(L);
  if (LJ_UNLIKELY(sysprof_status != PROFILE_SUCCESS)) {
    switch (sysprof_status) {
      case SYSPROF_ERRUSE:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_MISUSE));
        lua_pushinteger(L, EINVAL);
        return 3;
      case SYSPROF_ERRRUN:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_NOTRUNNING));
        lua_pushinteger(L, EINVAL);
        return 3;
      case SYSPROF_ERRIO:
        return luaL_fileresult(L, 0, NULL);
      default:
        lua_assert(0);
        return 0;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

// profile.sysprof_report()
LJLIB_CF(misc_sysprof_report) {
  struct luam_sysprof_data sysprof_data = {};
  GCtab *data_tab = NULL;
  GCtab *count_tab = NULL;

  luaM_sysprof_report(&sysprof_data);

  lua_createtable(L, 0, 3);
  data_tab = tabV(L->top - 1);

  setnumfield(L, data_tab, "samples", sysprof_data.samples);
  setnumfield(L, data_tab, "overruns", sysprof_data.overruns);
 
  lua_createtable(L, 0, LUAM_VMST_COUNT);
  count_tab = tabV(L->top - 1);

  setnumfield(L, count_tab, "INTERP", sysprof_data.vmstate[LUAM_VMST_INTERP]);
  setnumfield(L, count_tab, "LFUNC",  sysprof_data.vmstate[LUAM_VMST_LFUNC]);
  setnumfield(L, count_tab, "FFUNC",  sysprof_data.vmstate[LUAM_VMST_FFUNC]);
  setnumfield(L, count_tab, "CFUNC",  sysprof_data.vmstate[LUAM_VMST_CFUNC]);
  setnumfield(L, count_tab, "GC",     sysprof_data.vmstate[LUAM_VMST_GC]);
  setnumfield(L, count_tab, "EXIT",   sysprof_data.vmstate[LUAM_VMST_EXIT]);
  setnumfield(L, count_tab, "RECORD", sysprof_data.vmstate[LUAM_VMST_RECORD]);
  setnumfield(L, count_tab, "OPT",    sysprof_data.vmstate[LUAM_VMST_OPT]);
  setnumfield(L, count_tab, "ASM",    sysprof_data.vmstate[LUAM_VMST_ASM]);
  setnumfield(L, count_tab, "TRACE",  sysprof_data.vmstate[LUAM_VMST_TRACE]);

  lua_setfield(L, -2, "vmstate");

  return 1;
}

/* ------------------------------------------------------------------------ */

/* ----- misc.memprof module ---------------------------------------------- */

#define LJLIB_MODULE_misc_memprof

/*
** Yep, 8Mb. Tuned in order not to bother the platform with too often flushes.
*/
#define STREAM_BUFFER_SIZE (8 * 1024 * 1024)

/* Structure given as ctx to memprof writer and on_stop callback. */
struct memprof_ctx {
  /* Output file stream for data. */
  FILE *stream;
  /* Profiled global_State for lj_mem_free at on_stop callback. */
  global_State *g;
  /* Buffer for data. */
  uint8_t buf[STREAM_BUFFER_SIZE];
};

/*
** Default buffer writer function.
** Just call fwrite to the corresponding FILE.
*/
static size_t buffer_writer_default(const void **buf_addr, size_t len,
                                    void *opt) {
  struct memprof_ctx *ctx = opt;
  FILE *stream = ctx->stream;
  const void *const buf_start = *buf_addr;
  const void *data = *buf_addr;
  size_t write_total = 0;

  lua_assert(len <= STREAM_BUFFER_SIZE);

  for (;;) {
    const size_t written = fwrite(data, 1, len - write_total, stream);

    if (LJ_UNLIKELY(written == 0)) {
      /* Re-tries write in case of EINTR. */
      if (errno != EINTR) {
        /* Will be freed as whole chunk later. */
        *buf_addr = NULL;
        return write_total;
      }

      errno = 0;
      continue;
    }

    write_total += written;
    lua_assert(write_total <= len);

    if (write_total == len) break;

    data = (uint8_t *)data + (ptrdiff_t)written;
  }

  *buf_addr = buf_start;
  return write_total;
}

/* Default on stop callback. Just close the corresponding stream. */
static int on_stop_cb_default(void *opt, uint8_t *buf) {
  struct memprof_ctx *ctx = opt;
  FILE *stream = ctx->stream;
  UNUSED(buf);
  lj_mem_free(ctx->g, ctx, sizeof(*ctx));
  return fclose(stream);
}

/* local started, err, errno = misc.memprof.start(fname) */
LJLIB_CF(misc_memprof_start) {
  struct lj_memprof_options opt = {0};
  const char *fname = strdata(lj_lib_checkstr(L, 1));
  struct memprof_ctx *ctx;
  int memprof_status;

  /*
  ** FIXME: more elegant solution with ctx.
  ** Throws in case of OOM.
  */
  ctx = lj_mem_new(L, sizeof(*ctx));
  opt.ctx = ctx;
  opt.buf = ctx->buf;
  opt.writer = buffer_writer_default;
  opt.on_stop = on_stop_cb_default;
  opt.len = STREAM_BUFFER_SIZE;

  ctx->g = G(L);
  ctx->stream = fopen(fname, "wb");

  if (ctx->stream == NULL) {
    lj_mem_free(ctx->g, ctx, sizeof(*ctx));
    return luaL_fileresult(L, 0, fname);
  }

  memprof_status = lj_memprof_start(L, &opt);

  if (LJ_UNLIKELY(memprof_status != PROFILE_SUCCESS)) {
    switch (memprof_status) {
      case PROFILE_ERRUSE:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_MISUSE));
        lua_pushinteger(L, EINVAL);
        return 3;
#if LJ_HASMEMPROF
      case PROFILE_ERRRUN:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_ISRUNNING));
        lua_pushinteger(L, EINVAL);
        return 3;
      case PROFILE_ERRIO:
        return luaL_fileresult(L, 0, fname);
#endif
      default:
        lua_assert(0);
        return 0;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

/* local stopped, err, errno = misc.memprof.stop() */
LJLIB_CF(misc_memprof_stop) {
  int status = lj_memprof_stop(L);
  if (status != PROFILE_SUCCESS) {
    switch (status) {
      case PROFILE_ERRUSE:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_MISUSE));
        lua_pushinteger(L, EINVAL);
        return 3;
#if LJ_HASMEMPROF
      case PROFILE_ERRRUN:
        lua_pushnil(L);
        lua_pushstring(L, err2msg(LJ_ERR_PROF_NOTRUNNING));
        lua_pushinteger(L, EINVAL);
        return 3;
      case PROFILE_ERRIO:
        return luaL_fileresult(L, 0, NULL);
#endif
      default:
        lua_assert(0);
        return 0;
    }
  }
  lua_pushboolean(L, 1);
  return 1;
}

#include "lj_libdef.h"

/* ------------------------------------------------------------------------ */

LUALIB_API int luaopen_misc(struct lua_State *L) {
  LJ_LIB_REG(L, LUAM_MISCLIBNAME, misc);
  LJ_LIB_REG(L, LUAM_MISCLIBNAME ".memprof", misc_memprof);
  LJ_LIB_REG(L, LUAM_MISCLIBNAME ".sysprof", misc_sysprof);
  return 1;
}
