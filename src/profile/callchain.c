#include "callchain.h"

#include "../luajit.h"
#include "../lj_frame.h"

#include "profile_impl.h"
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

/*============================================================================*/
enum CALLCHAIN_TYPE {
  CALLCHAIN_NATIVE,
  CALLCHAIN_LUA,
  CALLCHAIN_TRACE
};

enum PROF_FRAME_TYPE {
  LFUNC,
  CFUNC,
  FFUNC,
  BOT_FRAME = 0x80
};
/*============================================================================*/

void dump_lua_state(struct profiler_state *ps) {
  UNUSED(ps);
}

void dump_lfunc(struct lj_wbuf *buf, GCfunc *func) 
{
  GCproto *pt = funcproto(func);
  lj_wbuf_addbyte(buf, LFUNC);
  lj_wbuf_addu64(buf, (uintptr_t)pt); 
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
  /* IMPRORTANT
   * This function should be called ONLY from
   * profiler callback since lua_State might be
   * not consistent during signal handling
   * */

  struct lj_wbuf* buf = &ps->buf;

  lj_wbuf_addbyte(buf, CALLCHAIN_LUA);

  lua_State *L = gco2th(gcref(ps->g->cur_L));
  assert(L != NULL);

  cTValue *frame, *nextframe, *bot = tvref(L->stack) + LJ_FR2;
  int level = 0;

  /* Traverse frames backwards. */
  for (nextframe = frame = L->base - 1; frame > bot; ) {
    if (frame_gc(frame) == obj2gco(L)) {
      goto next;  /* Skip dummy frames. See lj_err_optype_call(). */
    }

    break;

    fprintf(stderr, "new frame %p nextframe %p bot %p\n", frame, nextframe, bot);

    GCfunc *fn = frame_func(frame);
    if (!fn) {
      break;
    }

    if (isluafunc(fn)) {
      dump_lfunc(buf, fn); 
    } else if (isffunc(fn)) {
      dump_ffunc(buf, fn); 
    } else {
      dump_cfunc(buf, fn); 
    }
    fprintf(stderr, "frame end\n");

    if (++level == 1) {
      break;
    }

next:
    nextframe = frame;
    if (frame_islua(frame)) {
      frame = frame_prevl(frame);
    } else {
      frame = frame_prevd(frame);
    }
  }
 
  lj_wbuf_addbyte(buf, BOT_FRAME);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void dump_callchain_native(struct profiler_state *ps) {
  struct lj_wbuf* buf = &ps->buf;

  const int depth =
      backtrace(ps->backtrace_buf, 4000);

  lj_wbuf_addbyte(buf, CALLCHAIN_NATIVE);
  lj_wbuf_addu64(buf, (uint64_t)depth);

  for (int i = depth - 1; i >= 0; --i) {
    lj_wbuf_addu64(buf, (uintptr_t)ps->backtrace_buf[i]);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
