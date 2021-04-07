#ifndef PROFILE_IMPL_H
#define PROFILE_IMPL_H 

#include <pthread.h>

#include "../lj_obj.h"
#include "../lj_timer.h"
#include "iobuffer.h"

#include "../lj_system_profile.h"

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

struct profiler_state {
  global_State *g;
  int vmstate; /* current vmstate */
  pthread_t thread;
  struct iobuffer obuf; /* IO buffer for profile data filedumps */
  /* TMP */ void* backtrace_buf[DEFAULT_BUF_SIZE]; /* buffer for backtrace output */
  struct profile_data data;
  struct profiler_opt opt;
  struct lj_timer timer;
};

#endif /* ifndef PROFILE_IMPL_H */
