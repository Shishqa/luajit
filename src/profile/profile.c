#define _GNU_SOURCE
#include <ucontext.h>

#include "../lj_obj.h"
#include "../lua.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

#if LJ_HASPROFILE

#include "../lj_buf.h"
#include "../lj_debug.h"
#include "../lj_dispatch.h"
#include "../lj_frame.h"
#if LJ_HASJIT
#include "../lj_jit.h"
#include "../lj_trace.h"
#endif

#include "../luajit.h"

#include "../lj_wbuf.h"

#include "../lj_prof_symtab.h"
#include "iobuffer.h"
#include "profile.h"
#include "profile_impl.h"
#include "shared_objects.h"

#include <pthread.h>

static struct profiler_state profiler_state;

static const uint8_t LJP_FORMAT_VERSION = 0x1;

static const uint8_t ljp_header[] = { 'l', 'j', 'p', LJP_FORMAT_VERSION, 
  0x0, 0x0, 0x0 };

/* -- Main Payload -------------------------------------------------------- */

void profile_signal_handler(int sig, siginfo_t* info, void* ctx) 
{
  UNUSED(sig);
  UNUSED(info);
  UNUSED(ctx);

  struct profiler_state *ps = &profiler_state;

  lua_assert(pthread_self() == ps->thread);

  ps->data.samples++;

  global_State *g = ps->g;
  ps->vmstate = g->vmstate;

  ps->ctx.rip = (uint64_t)((ucontext_t *)ctx)->uc_mcontext.gregs[REG_RIP];

  uint32_t _vmstate = ~(uint32_t)(g->vmstate);
  uint32_t vmstate = _vmstate < LJ_VMST_TRACE ? _vmstate : LJ_VMST_TRACE;
  ++ps->data.vmstate[vmstate];

  stream_event(ps, vmstate);
}

/* -- Public profiling API ------------------------------------------------ */

/* Start profiling with default profling API */
void lj_sysprof_start(lua_State *L, const struct lj_sysprof_options *opt) {
  struct profiler_state *ps = &profiler_state;

  if (ps->g) {
    lj_sysprof_stop(L);
    if (ps->g)
      return; /* Profiler in use by another VM. */
  }

  ps->g = G(L);
  ps->thread = pthread_self();

  memcpy(&ps->opt, opt, sizeof(ps->opt));

  ps->data.samples = 0;
  memset(ps->data.vmstate, 0,
         sizeof(ps->data.vmstate)); /* Init counters for each state */

  init_iobuf(&ps->obuf, opt->fd);
  lj_wbuf_init(&ps->buf, flush_iobuf, &ps->obuf, ps->obuf.buf,
               sizeof(ps->obuf.buf));

  dump_symtab(&ps->buf, ps->g);
  so_dump(&ps->buf);

  lj_wbuf_addn(&ps->buf, ljp_header, sizeof(ljp_header));

  ps->timer.opt.interval = opt->interval;
  ps->timer.opt.callback = profile_signal_handler;
  lj_timer_start(&ps->timer);
}

/* Stop profiling. */
void lj_sysprof_stop(lua_State *L) {
  struct profiler_state *ps = &profiler_state;
  global_State *g = ps->g;

  if (G(L) == g) {
    lj_timer_stop(&ps->timer);

    // print_counters(ps);

    lj_wbuf_addbyte(&ps->buf, 0xBB);

    lj_wbuf_flush(&ps->buf);
    lj_wbuf_terminate(&ps->buf);

    ps->g = NULL;
  }
}

#endif
