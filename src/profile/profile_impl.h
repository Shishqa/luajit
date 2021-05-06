#ifndef PROFILE_IMPL_H
#define PROFILE_IMPL_H 

#include <bits/stdint-uintn.h>
#include <pthread.h>

#include "../lj_obj.h"
#include "../lj_timer.h"
#include "../lj_wbuf.h"
#include "../lj_sysprof.h"

#include "iobuffer.h"

struct profile_data {
  uint64_t samples;
  uint64_t vmstate[LJ_VMST__MAX + 1];
};

struct sig_context {
  uint64_t rip;
};

enum {
  BACKTRACE_BUF_SIZE = 4096
};

struct profiler_state {
  global_State *g;
  int vmstate; /* current vmstate */
  pthread_t thread;
  struct sig_context ctx;
  struct lj_wbuf buf;
  /* TMP */ struct iobuffer obuf;
  /* TMP */ void* backtrace_buf[BACKTRACE_BUF_SIZE]; /* buffer for backtrace output */
  struct profile_data data;
  struct lj_sysprof_options opt;
  struct lj_timer timer;
};

void stream_event(struct profiler_state* ps, uint32_t vmstate);

void print_counters(struct profiler_state* ps);

#endif /* ifndef PROFILE_IMPL_H */
