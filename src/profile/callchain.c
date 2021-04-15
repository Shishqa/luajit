#include "callchain.h"

#include "../luajit.h"
#include "../lj_frame.h"

#include "profile_impl.h"
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

/*============================================================================*/
enum CALLCHAIN_TYPE {
  CALLCHAIN_NATIVE = 0xFF,
  CALLCHAIN_LUA = 0xFA,
  CALLCHAIN_TRACE = 0xBB
};
/*============================================================================*/

enum PROF_FRAME_TYPE {
  LFUNC,
  CFUNC,
  FFUNC
};

void dump_lua_state(struct profiler_state *ps) {
  UNUSED(ps);
}

void dump_lfunc(struct lj_wbuf *buf, GCfunc *func) 
{
  GCproto *pt = funcproto(func);
  lj_wbuf_addu64(buf, LFUNC);
  lj_wbuf_addu64(buf, (uintptr_t)pt); 
}

void dump_cfunc(struct lj_wbuf *buf, GCfunc *func) 
{
  lj_wbuf_addu64(buf, CFUNC);
  lj_wbuf_addu64(buf, (uintptr_t)func->c.f);
}

void dump_ffunc(struct lj_wbuf *buf, GCfunc *func) 
{
  lj_wbuf_addu64(buf, FFUNC);
  lj_wbuf_addu64(buf, func->c.ffid);
}

void dump_callchain_lua(struct profiler_state *ps) 
{
  /* IMPRORTANT
   * This function should be called ONLY from
   * profiler callback since lua_State might be
   * not consistent during signal handling
   * */

  struct lj_wbuf* buf = &ps->buf;

  lua_State *L = gco2th(gcref(ps->g->cur_L));
  assert(L != NULL);

  cTValue *frame, *nextframe, *bot = tvref(L->stack) + LJ_FR2;
  int level = 0;

  /* Traverse frames backwards. */
  for (nextframe = frame = L->base-1; frame > bot; ) {
    if (frame_gc(frame) == obj2gco(L))
      goto next;  /* Skip dummy frames. See lj_err_optype_call(). */
  
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    GCfunc *fn = frame_func(frame);

    if (fn == NULL) {
      fprintf(stderr, "bad func\n"); 
      return;
    }

    if (isluafunc(fn)) {
      dump_lfunc(buf, fn); 
    } else if (isffunc(fn)) {
      dump_ffunc(buf, fn); 
    } else {
      dump_cfunc(buf, fn); 
    }
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    level++;

next:
    nextframe = frame;
    if (frame_islua(frame)) {
      frame = frame_prevl(frame);
    } else {
      frame = frame_prevd(frame);
    }
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void dump_callchain_native(struct profiler_state *ps) {
  struct lj_wbuf* buf = &ps->buf;

  const int depth =
      backtrace(ps->backtrace_buf,
                sizeof(ps->backtrace_buf) / sizeof(*ps->backtrace_buf));

  lj_wbuf_addu64(buf, CALLCHAIN_NATIVE);
  lj_wbuf_addu64(buf, depth);
  lj_wbuf_addn(buf, ps->backtrace_buf, depth * sizeof(*ps->backtrace_buf));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
