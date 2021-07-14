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

#define UNW_LOCAL_ONLY
#include <libunwind.h>

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

  cTValue *frame, *bot = tvref(L->stack)+LJ_FR2;
  /* Traverse frames backwards. */
  for (frame = top_frame; frame > bot; frame = frame_prev(frame)) {
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

enum { PROFILER_STACK_DEPTH = 4 };

void dump_callchain_host(struct profiler_state *ps) {
  int frame_no = 0;
  struct lj_wbuf *buf = &ps->buf;
  unw_word_t sp = 0, old_sp = 0, ip = 0;
  unw_context_t unw_context = {};
  unw_getcontext(&unw_context);
  unw_cursor_t unw_cur;
  unw_init_local(&unw_cur, &unw_context);
  int unw_status;
  lj_wbuf_addbyte(buf, CALLCHAIN_HOST);
  while ((unw_status = unw_step(&unw_cur)) > 0) {
    old_sp = sp;
    unw_get_reg(&unw_cur, UNW_REG_IP, &ip);
    unw_get_reg(&unw_cur, UNW_REG_SP, &sp);
    if (sp == old_sp) {
      fprintf(stderr, "unwinding error: previous frame "
                "identical to this frame (corrupt stack?)");
      break;
    }
    if (LJ_LIKELY(frame_no >= PROFILER_STACK_DEPTH)) {
    	lj_wbuf_addu64(buf, (uintptr_t)ip);
    }
    ++frame_no;
  }
  if (unw_status != 0)
    fprintf(stderr, "unwinding error: %i", unw_status);
  lj_wbuf_addu64(buf, (uintptr_t)NULL);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void dump_callchain_trace(struct profiler_state *ps) {
  struct lj_wbuf *buf = &ps->buf;

  uint32_t traceno = ps->vmstate;
  jit_State *J = G2J(ps->g);

  lj_wbuf_addbyte(buf, CALLCHAIN_TRACE);
  lj_wbuf_addu64(buf, traceno);
  lj_wbuf_addu64(buf, (uintptr_t)J->prev_pt);
  lj_wbuf_addu64(buf, J->prev_line);

  // TODO: unwind trace's inlined functions
}
