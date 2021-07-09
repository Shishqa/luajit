#define callchain_c
#define LUA_CORE

#include "callchain.h"

#include "../lj_debug.h"
#include "../lj_dispatch.h"
#include "../lj_err.h"
#include "../lj_frame.h"
#include "../lj_obj.h"
#include "../lj_state.h"
#include "../luajit.h"

#include "sysprof_impl.h"
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

#include <execinfo.h>

enum CALLCHAIN_TYPE { 
  CALLCHAIN_HOST, 
  CALLCHAIN_LUA, 
  CALLCHAIN_TRACE 
};

enum PROF_FRAME_TYPE { 
  LFUNC, 
  CFUNC, 
  FFUNC, 
  BOT_FRAME = 0x80 
};

void dump_lfunc(struct lj_wbuf *buf, GCfunc *fn) 
{
  lua_assert(buf && fn);
  GCproto *pt = funcproto(fn);
  lj_wbuf_addbyte(buf, LFUNC);
  lj_wbuf_addu64(buf, (uintptr_t)pt);
  lj_wbuf_addu64(buf, pt->firstline < 0 ? 0 : pt->firstline);
}

void dump_cfunc(struct lj_wbuf *buf, GCfunc *func) 
{
  lj_wbuf_addbyte(buf, CFUNC);
  lj_wbuf_addu64(buf, (uintptr_t)func->c.f);
}

void dump_ffunc(struct lj_wbuf *buf, GCfunc *func) 
{
  lj_wbuf_addbyte(buf, FFUNC);
  lj_wbuf_addu64(buf, func->c.ffid);
}

void dump_callchain_lua(struct profiler_state *ps) 
{
  lua_assert(ps);

  struct lj_wbuf *buf = &ps->buf;

  lj_wbuf_addbyte(buf, CALLCHAIN_LUA);

  lua_State *L = gco2th(gcref(ps->g->cur_L));
  lua_assert(L != NULL);

  cTValue *top_frame = ps->g->top_frame.guesttop.interp_base - 1;

  cTValue *frame, *nextframe, *bot = tvref(L->stack)+LJ_FR2;
  /* Traverse frames backwards. */
  for (nextframe = frame = top_frame; frame > bot; 
       nextframe = frame, frame = frame_prev(frame)) {
    if (frame_gc(frame) == obj2gco(L)) {
      continue;  /* Skip dummy frames. See lj_err_optype_call(). */
    } else if (frame_isvarg(frame)) {
      continue;
    }

    GCfunc *fn = frame_func(frame);
    if (!fn) {
      lj_wbuf_addbyte(buf, CFUNC);
      lj_wbuf_addu64(buf, (uintptr_t)0xBADBEEF);
    } else if (isluafunc(fn)) {
      dump_lfunc(buf, fn);
    } else if (isffunc(fn)) { /* Dump numbered builtins. */
      dump_ffunc(buf, fn);
    } else { /* Dump C function address. */
      dump_cfunc(buf, fn);
    }
  }

  lj_wbuf_addbyte(buf, BOT_FRAME);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

enum { PROFILER_STACK_DEPTH = 5 };

void dump_callchain_host(struct profiler_state *ps) {
  struct lj_wbuf *buf = &ps->buf;

  const int depth = backtrace(ps->backtrace_buf, ps->opt.mode == PROFILE_LEAF
                                                     ? PROFILER_STACK_DEPTH + 1
                                                     : BACKTRACE_BUF_SIZE);
  lua_assert(depth >= PROFILER_STACK_DEPTH);

  lj_wbuf_addbyte(buf, CALLCHAIN_HOST);
  lj_wbuf_addu64(buf, (uint64_t)(depth - PROFILER_STACK_DEPTH));

  for (int i = PROFILER_STACK_DEPTH; i < depth; ++i) {
    lj_wbuf_addu64(buf, (uintptr_t)ps->backtrace_buf[i]);
    if (LJ_UNLIKELY(ps->opt.mode == PROFILE_LEAF)) {
      break;
    }
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void dump_callchain_trace(struct profiler_state *ps) {
  struct lj_wbuf *buf = &ps->buf;

  uint32_t traceno = ps->vmstate;
  GCproto *pt = G2J(ps->g)->pt;

  lj_wbuf_addbyte(buf, CALLCHAIN_TRACE);
  lj_wbuf_addu64(buf, traceno);
  lj_wbuf_addu64(buf, (uintptr_t)pt);
  lj_wbuf_addu64(buf, pt->firstline);

  // TODO: unwind trace
}
