#define sysprof_c
#define LUA_CORE

#define _GNU_SOURCE

#include "../lj_obj.h"
#include "../lua.h"

#if !LJ_DISABLE_SYSPROF

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

#include "../lj_sysprof.h"
#include "sysprof_impl.h"
#include "iobuffer.h"

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

/* Sadly, we have to use a static profiler state.
**
** The SIGPROF variant needs a static pointer to the global state, anyway.
** And it would be hard to extend for multiple threads. You can still use
** multiple VMs in multiple threads, but only profile one at a time.
*/
static struct profiler_state profiler_state;

/* -- Main Payload -------------------------------------------------------- */

static void profile_signal_handler(int sig, siginfo_t *info, void *ctx) {
  UNUSED(sig);
  UNUSED(info);

  struct profiler_state *ps = &profiler_state;

  lua_assert(pthread_self() == ps->thread);
  lua_assert(ps->state == RUNNING);

  ps->data.samples++;
  ps->data.overruns += info->si_overrun;

  global_State *g = ps->g;
  ps->vmstate = g->vmstate;

  uint32_t _vmstate = ~(uint32_t)(g->vmstate);
  uint32_t vmstate = _vmstate < LJ_VMST_TRACE ? _vmstate : LJ_VMST_TRACE;
  ps->data.vmstate[vmstate]++;

  if (ps->opt.mode != PROFILE_DEFAULT) {
    stream_event(ps, vmstate);
  }
}

static int lj_sysprof_init(struct profiler_state *ps, global_State *g,
                           const struct lj_sysprof_options *opt) 
{
  ps->g = g;
  ps->thread = pthread_self();

  memcpy(&ps->opt, opt, sizeof(ps->opt));

  /* Init counters */
  ps->data.samples = 0;
  ps->data.overruns = 0;
  memset(ps->data.vmstate, 0, sizeof(ps->data.vmstate));

  if (opt->mode != PROFILE_DEFAULT) {
    int fd = open(opt->path, O_WRONLY | O_CREAT, 0644);
    if (-1 == fd) {
      return SYSPROF_ERRIO;
    }
    init_iobuf(&ps->iobuf, g, fd); 
    lj_wbuf_init(&ps->buf, flush_iobuf, &ps->iobuf, 
                 ps->iobuf.buf, sizeof(ps->iobuf.buf));
  }

  return SYSPROF_SUCCESS;
}

static int lj_sysprof_validate(struct profiler_state *ps, 
                               const struct lj_sysprof_options *opt) 
{
  if (ps->state != IDLE) {
    return SYSPROF_ERRRUN;
  } else if (opt->interval == 0) {
    return SYSPROF_ERRUSE;
  }
  return SYSPROF_SUCCESS;
}

/* -- Internal profiling API --------------------------------------------- */

/* Start profiling with default profling API */
int lj_sysprof_start(lua_State *L, const struct lj_sysprof_options *opt) {
  struct profiler_state *ps = &profiler_state;

  enum lj_sysprof_err status = SYSPROF_SUCCESS;
  if (SYSPROF_SUCCESS != (status = lj_sysprof_validate(ps, opt))) {
    return status;
  }
  
  status = lj_sysprof_init(ps, G(L), opt);
  if (status != SYSPROF_SUCCESS) {
    return status;
  }
  ps->state = RUNNING;

  stream_prologue(ps);

  ps->timer.opt.interval = opt->interval;
  ps->timer.opt.handler = profile_signal_handler;
  lj_timer_start(&ps->timer);

  return SYSPROF_SUCCESS;
}

/* Stop profiling. */
int lj_sysprof_stop(lua_State *L) {
  struct profiler_state *ps = &profiler_state;
  global_State *g = ps->g;

  if (G(L) == g) {
    lj_timer_stop(&ps->timer);

    stream_epilogue(ps);
    lj_wbuf_flush(&ps->buf);
    lj_wbuf_terminate(&ps->buf);

    close(ps->iobuf.fd);
    terminate_iobuf(&ps->iobuf);

    ps->g = NULL;
    ps->state = IDLE;
    return SYSPROF_SUCCESS;
  }

  return SYSPROF_ERRRUN;
}

#endif
