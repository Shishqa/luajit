#ifndef PROFILE_IMPL_H
#define PROFILE_IMPL_H 

#include <bits/stdint-uintn.h>
#include <pthread.h>

#include "../lj_obj.h"
#include "../lj_timer.h"
#include "../lj_wbuf.h"
#include "../lj_sysprof.h"

#include "iobuffer.h"

/*
enum PROFILE_STATE {
  NATIVE = 0,   // native (trace)
  INTERP = 1,   // interpeted
  LFUNC = 2,    // lfunc
  FFUNC = 3,    // ffunc
  CFUNC = 4,    // cfunc
  GCOLL = 5,    // garbage collector
  JITCOMP = 6,  // jit compiler
  STATE_MAX = 7
};
*/

struct profile_data {
  uint64_t samples;
  uint64_t vmstate[LJ_VMST__MAX + 1];
};

struct sig_context {
  uint64_t rip;
};

#define BACKTRACE_BUF_SIZE 4096

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

#endif /* ifndef PROFILE_IMPL_H */
