#ifndef LJ_PROFILE_STATE
#define LJ_PROFILE_STATE

#include <signal.h>

#include "lj_obj.h"
#include "luajit.h"
#include "profile/iobuffer.h"

enum PROFILE_STATES {
  NATIVE = 0,   // native (trace)
  INTERP = 1,   // interpeted
  LFUNC = 2,    // lfunc
  FFUNC = 3,    // ffunc
  CFUNC = 4,    // cfunc
  GCOLL = 5,        // garbage collector
  JITCOMP = 6,  // jit compiler
  STATE_MAX = 7
};

typedef struct {
  uint64_t vmstate[STATE_MAX];

} profile_counters;

/* Profiler state. */
typedef struct ProfileState {
  global_State *g;            /* VM state that started the profiler. */
  luaJIT_profile_callback cb; /* Profiler callback. */
  void *data;                 /* Profiler callback data. */
  SBuf sb;                    /* String buffer for stack dumps. */
  struct iobuffer obuf;       /* IO buffer for profile data filedumps */
  void* backtrace_buf[DEFAULT_BUF_SIZE];
  int interval;               /* Sample interval in milliseconds. */
  int samples;                /* Number of samples for next callback. */
  profile_counters counters;
  int vmstate; /* VM state when profile timer triggered. */
#if LJ_PROFILE_SIGPROF
  struct sigaction oldsa; /* Previous SIGPROF state. */
#elif LJ_PROFILE_PTHREAD
  pthread_mutex_t lock; /* g->hookmask update lock. */
  pthread_t thread;    /* Timer thread. */
  int abort;            /* Abort timer thread. */
#elif LJ_PROFILE_WTHREAD
#if LJ_TARGET_WINDOWS
  HINSTANCE wmm;         /* WinMM library handle. */
  WMM_TPFUNC wmm_tbp;    /* WinMM timeBeginPeriod function. */
  WMM_TPFUNC wmm_tep;    /* WinMM timeEndPeriod function. */
#endif
  CRITICAL_SECTION lock; /* g->hookmask update lock. */
  HANDLE thread;         /* Timer thread. */
  int abort;             /* Abort timer thread. */
#endif
} ProfileState;

#endif
