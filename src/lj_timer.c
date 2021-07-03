/* Simple profiling timer extracted from inbuilt luajit profiler
*/

#define lj_timer_c
#define LUA_CORE

#include "lj_timer.h"

#if LJ_PROFILE_SIGPROF

/* Start profiling timer. */
void lj_timer_start(struct lj_timer *timer) {
  int interval = timer->opt.interval;
  struct itimerval tm;
  struct sigaction sa;
  tm.it_value.tv_sec = tm.it_interval.tv_sec = interval / 1000;
  tm.it_value.tv_usec = tm.it_interval.tv_usec = (interval % 1000) * 1000;
  setitimer(ITIMER_PROF, &tm, NULL);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sa.sa_sigaction = timer->opt.handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGPROF, &sa, &timer->oldsa);
}

/* Stop profiling timer. */
void lj_timer_stop(struct lj_timer *timer) {
  struct itimerval tm;
  tm.it_value.tv_sec = tm.it_interval.tv_sec = 0;
  tm.it_value.tv_usec = tm.it_interval.tv_usec = 0;
  setitimer(ITIMER_PROF, &tm, NULL);
  sigaction(SIGPROF, &timer->oldsa, NULL);
}

#elif LJ_PROFILE_PTHREAD

/* POSIX timer thread. */
static void *profile_thread(struct lj_timer *timer) {
  int interval = timer->opt.interval;
#if !LJ_TARGET_PS3
  struct timespec ts;
  ts.tv_sec = interval / 1000;
  ts.tv_nsec = (interval % 1000) * 1000000;
#endif
  while (1) {
#if LJ_TARGET_PS3
    sys_timer_usleep(interval * 1000);
#else
    nanosleep(&ts, NULL);
#endif
    if (timer->abort)
      break;
    timer->opt.handler(0);
  }
  return NULL;
}

/* Start profiling timer thread. */
void lj_timer_start(struct lj_timer *timer) {
  pthread_mutex_init(&timer->lock, 0);
  timer->abort = 0;
  pthread_create(&timer->thread, NULL, (void *(*)(void *))profile_thread,
                 timer);
}

/* Stop profiling timer thread. */
void lj_timer_stop(struct lj_timer *timer) {
  timer->abort = 1;
  pthread_join(timer->thread, NULL);
  pthread_mutex_destroy(&timer->lock);
}

#elif LJ_PROFILE_WTHREAD

/* Windows timer thread. */
static DWORD WINAPI profile_thread(void *timerx) {
  struct lj_timer *timer = (struct lj_timer *)timerx;
  int interval = timer->opt.interval;
#if LJ_TARGET_WINDOWS
  timer->wmm_tbp(interval);
#endif
  while (1) {
    Sleep(interval);
    if (timer->abort)
      break;
    timer->opt.handler();
  }
#if LJ_TARGET_WINDOWS
  timer->wmm_tep(interval);
#endif
  return 0;
}

/* Start profiling timer thread. */
void lj_timer_start(struct lj_timer *timer) {
#if LJ_TARGET_WINDOWS
  if (!timer->wmm) { /* Load WinMM library on-demand. */
    timer->wmm = LoadLibraryExA("winmm.dll", NULL, 0);
    if (timer->wmm) {
      timer->wmm_tbp =
          (WMM_TPFUNC)GetProcAddress(timer->wmm, "timeBeginPeriod");
      timer->wmm_tep = (WMM_TPFUNC)GetProcAddress(timer->wmm, "timeEndPeriod");
      if (!timer->wmm_tbp || !timer->wmm_tep) {
        timer->wmm = NULL;
        return;
      }
    }
  }
#endif
  InitializeCriticalSection(&timer->lock);
  timer->abort = 0;
  timer->thread = CreateThread(NULL, 0, profile_thread, timer, 0, NULL);
}

/* Stop profiling timer thread. */
void lj_timer_stop(struct lj_timer *timer) {
  timer->abort = 1;
  WaitForSingleObject(timer->thread, INFINITE);
  DeleteCriticalSection(&timer->lock);
}

#endif