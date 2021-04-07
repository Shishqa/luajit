#ifndef _LJ_TIMER_H
#define _LJ_TIMER_H

#include <stdint.h>

#include "lj_arch.h"

#if LJ_PROFILE_SIGPROF

#include <sys/time.h>
#include <signal.h>

struct lj_timer_opt {
  void (*callback)(int, siginfo_t*, void*);
  uint32_t interval;
};

struct lj_timer {
  struct sigaction oldsa;	/* Previous SIGPROF state. */
  struct lj_timer_opt opt;
};

#elif LJ_PROFILE_PTHREAD

#include <pthread.h>
#include <time.h>
#if LJ_TARGET_PS3
#include <sys/timer.h>
#endif

struct lj_timer_opt {
  void (*callback)(int, siginfo_t*, void*);
  int signo;
  uint32_t usec;
};

struct lj_timer {
  timer_t id;
  struct sigaction oldsa;	/* Previous SIGPROF state. */
  struct lj_timer_opt opt;
};

// TODO: fix for pthread
//  void (*handler)(struct ProfileStateBase*);
//  pthread_mutex_t lock;		/* g->hookmask update lock. */
//  pthread_t thread;		/* Timer thread. */
//  int abort;			/* Abort timer thread. */


#elif LJ_PROFILE_WTHREAD
#define WIN32_LEAN_AND_MEAN
#if LJ_TARGET_XBOX360
#include <xtl.h>
#include <xbox.h>
#else
#include <windows.h>
#endif
typedef unsigned int (WINAPI *WMM_TPFUNC)(unsigned int);

struct lj_timer_opt {
};

struct lj_timer {
};

// TODO: fix for wthread
//#if LJ_TARGET_WINDOWS
//  HINSTANCE wmm;		/* WinMM library handle. */
//  WMM_TPFUNC wmm_tbp;		/* WinMM timeBeginPeriod function. */
//  WMM_TPFUNC wmm_tep;		/* WinMM timeEndPeriod function. */
//#endif
//  CRITICAL_SECTION lock;	/* g->hookmask update lock. */
//  HANDLE thread;		/* Timer thread. */
//  int abort;			/* Abort timer thread. */

#endif

/* Start timer */
void lj_timer_start(struct lj_timer *timer);

/* Stop timer */
void lj_timer_stop(const struct lj_timer *timer);


#endif /* ifndef _LJ_TIMER_H */
