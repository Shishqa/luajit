#include "../lua.h"
#include "../lj_obj.h"
#include <stdint.h>

#if LJ_HASPROFILE

#include "../lj_buf.h"
#include "../lj_debug.h"
#include "../lj_dispatch.h"
#include "../lj_frame.h"
#if LJ_HASJIT
#include "../lj_jit.h"
#include "../lj_trace.h"
#endif

#include "../lj_profile.h"
#include "../luajit.h"

#include "profile.h"
#include "profile_impl.h"
#include "iobuffer.h"
#include "write.h"
#include "symtab.h"

#include <pthread.h>

static struct profiler_state profiler_state;

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

  uint32_t _vmstate = ~(uint32_t)(g->vmstate);
  uint32_t vmstate = _vmstate < LJ_VMST_TRACE ? _vmstate : LJ_VMST_TRACE;
  ++ps->data.vmstate[vmstate];

  write_stack(ps, vmstate);
}

/* -- Public profiling API ------------------------------------------------ */

/* Start profiling with default profling API */
void profile_start(lua_State *L, const struct profiler_opt *opt, int fd) {
  struct profiler_state *ps = &profiler_state;

  if (ps->g) {
    profile_stop();
    if (ps->g) return; /* Profiler in use by another VM. */
  }

  ps->g = G(L);
  ps->thread = pthread_self();

  ps->data.samples = 0;
  memset(ps->data.vmstate, 0, 
         sizeof(ps->data.vmstate)); /* Init counters for each state */

  init_iobuf(&ps->obuf, fd, DEFAULT_BUF_SIZE);

  write_symtab(&ps->obuf, ps->g);

  ps->timer.opt.interval = opt->interval;
  ps->timer.opt.callback = profile_signal_handler;
  lj_timer_start(&ps->timer);
}


/* Stop profiling. */
void profile_stop(void) {
  struct profiler_state *ps = &profiler_state;
    
  lj_timer_stop(&ps->timer);
  
  flush_iobuf(&ps->obuf);
  release_iobuf(&ps->obuf);
  
  write_symtab(&ps->obuf, ps->g);
    
  print_counters(ps);
}

#endif
