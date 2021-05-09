#include "callchain.h"

#include "../lj_frame.h"
#include "../lj_obj.h"
#include "../luajit.h"
#include "../lj_err.h"
#include "../lj_state.h"

#include "profile_impl.h"
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

#include <execinfo.h>

/*============================================================================*/
enum CALLCHAIN_TYPE { CALLCHAIN_NATIVE, CALLCHAIN_LUA, CALLCHAIN_TRACE };

enum PROF_FRAME_TYPE { LFUNC, CFUNC, FFUNC, BOT_FRAME = 0x80 };
/*============================================================================*/

void dump_lfunc(struct lj_wbuf *buf, GCfunc *func) {
  GCproto *pt = funcproto(func);
  lj_wbuf_addbyte(buf, LFUNC);
  lj_wbuf_addu64(buf, (uintptr_t)pt);
  lj_wbuf_addu64(buf, pt->firstline);
}

void dump_cfunc(struct lj_wbuf *buf, GCfunc *func) {
  lj_wbuf_addbyte(buf, CFUNC);
  lj_wbuf_addu64(buf, (uintptr_t)func->c.f);
}

void dump_ffunc(struct lj_wbuf *buf, GCfunc *func) {
  lj_wbuf_addbyte(buf, FFUNC);
  lj_wbuf_addu64(buf, func->c.ffid);
}

void dump_callchain_lua(struct profiler_state *ps) {
  /* IMPRORTANT
   * This function should be called ONLY from
   * profiler callback since lua_State might be
   * not consistent during signal handling
   * */

  struct lj_wbuf *buf = &ps->buf;

  lj_wbuf_addbyte(buf, CALLCHAIN_LUA);

  lua_State *L = gco2th(gcref(ps->g->cur_L));
  assert(L != NULL);

  // cTValue *frame, *bot = tvref(L->stack) + LJ_FR2;

  TValue *frame = L->base - 1;
  void *cf = L->cframe;
  while (frame > tvref(L->stack) + LJ_FR2) {
    switch (frame_typep(frame)) {
    case FRAME_LUA: /* Lua frame. */
    case FRAME_LUAP: {
      GCfunc *fn = frame_func(frame);
      if (!fn) {
        lj_wbuf_addbyte(buf, CFUNC);
        lj_wbuf_addu64(buf, (uintptr_t)(0xBADBEEF));
        goto end;
      }

      if (isluafunc(fn)) {
        dump_lfunc(buf, fn);
      } else if (isffunc(fn)) {
        dump_ffunc(buf, fn);
      } else if (iscfunc(fn)) {
        dump_cfunc(buf, fn);
      } else {
        lua_assert(0);
      }

      frame = frame_prevl(frame);
      }
      break;
    case FRAME_C: /* C frame. */
      //fprintf(stderr, "C\n");
    unwind_c:
      cf = cframe_prev(cf);
      frame = frame_prevd(frame);
      break;
    case FRAME_CP:               /* Protected C frame. */
      //fprintf(stderr, "CP\n");
      goto end;
    case FRAME_CONT: /* Continuation frame. */
      //fprintf(stderr, "CONT\n");
      if (frame_iscont_fficb(frame))
        goto unwind_c;
    case FRAME_VARG: /* Vararg frame. */
      //fprintf(stderr, "VARG\n");
      frame = frame_prevd(frame);
      break;
    case FRAME_PCALL:  /* FF pcall() frame. */
    case FRAME_PCALLH: /* FF pcall() frame inside hook. */
      //fprintf(stderr, "PCALL\n");
      break; // FIXME: check yield?
    }
  }

end:
  //fprintf(stderr, "|||\n");

  lj_wbuf_addbyte(buf, BOT_FRAME);
  return;


  /*
  for (frame = L->base - 1; frame > bot; frame--) {
    GCfunc* fn = frame_func(frame);
    fprintf(stderr, "%p (%p) - %ld - %p", frame, (void*)frame->u64,
  frame_type(frame), fn);

    if (frame_gc(frame) == obj2gco(L)) {
      fprintf(stderr, " __ skip\n");
      continue;
    }

    if (fn) {

      GCproto *pt = NULL;
      if (isluafunc(fn)) {
        pt = funcproto(fn);
        fprintf(stderr, " - LUA %p", pt);
      } else if (isffunc(fn)) {
        fprintf(stderr, " - FFUNC %d", fn->c.ffid);
      } else {
        fprintf(stderr, " - CFUNC %p", fn->c.f);
      }

      if (pt != NULL && pt != (GCproto*)0xffffffffffffffc0) {
        GCstr *name = proto_chunkname(pt);
        const char* n = strdata(name);
        fprintf(stderr, " %s ---> %p", n, frame_prev(frame));
        frame = frame_prev(frame) + 1;
      }
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
  */

  /* Traverse frames backwards. */
  /*
  for (frame = L->base - 1; frame > bot; frame = frame_prev(frame)) {
    lua_assert(frame);

    if (frame_gc(frame) == obj2gco(L)) {
      // fprintf(stderr, "skip\n");
      continue; // Skip dummy frames. See lj_err_optype_call(). 
    }

    // fprintf(stderr, "frame %ld\n", frame_type(frame));

    GCfunc *fn = frame_func(frame);
    if (!fn) {
      // fprintf(stderr, "fail\n");
      _exit(1);
      lj_wbuf_addbyte(buf, CFUNC);
      lj_wbuf_addu64(buf, (uintptr_t)(0xBADBEEF));
      break;
    }

    // fprintf(stderr, "ok\n");

    if (isluafunc(fn)) {
      dump_lfunc(buf, fn);
    } else if (isffunc(fn)) {
      dump_ffunc(buf, fn);
    } else if (iscfunc(fn)) {
      dump_cfunc(buf, fn);
    } else {
      lua_assert(0);
    }
  }

  lj_wbuf_addbyte(buf, BOT_FRAME);
  */
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

enum { PROFILER_STACK_DEPTH = 5 };

void dump_callchain_native(struct profiler_state *ps) {
  struct lj_wbuf *buf = &ps->buf;

  const int depth = backtrace(ps->backtrace_buf, BACKTRACE_BUF_SIZE);

  lua_assert(depth >= PROFILER_STACK_DEPTH);

  lj_wbuf_addbyte(buf, CALLCHAIN_NATIVE);
  lj_wbuf_addu64(buf, (uint64_t)(depth - PROFILER_STACK_DEPTH));

  for (int i = depth - 1; i >= PROFILER_STACK_DEPTH; --i) {
    lj_wbuf_addu64(buf, (uintptr_t)ps->backtrace_buf[i]);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
