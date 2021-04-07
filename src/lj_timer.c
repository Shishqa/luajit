#include "lj_timer.h"

#include <string.h>

struct lj_timer_opt;
struct lj_timer;

#if LJ_PROFILE_SIGPROF

/* Start profiling timer. */
void lj_timer_start(struct lj_timer *timer)
{
  int interval = timer->opt.interval;
  struct itimerval tm;
  struct sigaction sa;
  tm.it_value.tv_sec = tm.it_interval.tv_sec = interval / 1000;
  tm.it_value.tv_usec = tm.it_interval.tv_usec = (interval % 1000) * 1000;
  setitimer(ITIMER_PROF, &tm, NULL);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sa.sa_sigaction = timer->opt.callback;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGPROF, &sa, &timer->oldsa);
}

/* Stop profiling timer. */
void lj_timer_stop(const struct lj_timer *timer)
{
  struct itimerval tm;
  tm.it_value.tv_sec = tm.it_interval.tv_sec = 0;
  tm.it_value.tv_usec = tm.it_interval.tv_usec = 0;
  setitimer(ITIMER_PROF, &tm, NULL);
  sigaction(SIGPROF, &timer->oldsa, NULL);
}

#elif LJ_PROFILE_PTHREAD

// TODO: fix profile state for other handlers

/* POSIX timer thread. */
static void *profile_thread(ProfileStateBase *ps)
{
  int interval = ps->interval;
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
    if (ps->abort) break;
    ps->handler(0);
  }
  return NULL;
}

/* Start profiling timer thread. */
static void profile_timer_start(ProfileStateBase *ps)
{
  pthread_mutex_init(&ps->lock, 0);
  ps->abort = 0;
  pthread_create(&ps->thread, NULL, (void *(*)(void *))profile_thread, ps);
}

/* Stop profiling timer thread. */
static void profile_timer_stop(ProfileState *ps)
{
  ps->abort = 1;
  pthread_join(ps->thread, NULL);
  pthread_mutex_destroy(&ps->lock);
}

#elif LJ_PROFILE_WTHREAD

/* Windows timer thread. */
static DWORD WINAPI profile_thread(void *psx)
{
  ProfileState *ps = (ProfileState *)psx;
  int interval = ps->interval;
#if LJ_TARGET_WINDOWS
  ps->wmm_tbp(interval);
#endif
  while (1) {
    Sleep(interval);
    if (ps->abort) break;
    profile_trigger(ps);
  }
#if LJ_TARGET_WINDOWS
  ps->wmm_tep(interval);
#endif
  return 0;
}

/* Start profiling timer thread. */
static void profile_timer_start(ProfileState *ps)
{
#if LJ_TARGET_WINDOWS
  if (!ps->wmm) {  /* Load WinMM library on-demand. */
    ps->wmm = LoadLibraryExA("winmm.dll", NULL, 0);
    if (ps->wmm) {
      ps->wmm_tbp = (WMM_TPFUNC)GetProcAddress(ps->wmm, "timeBeginPeriod");
      ps->wmm_tep = (WMM_TPFUNC)GetProcAddress(ps->wmm, "timeEndPeriod");
      if (!ps->wmm_tbp || !ps->wmm_tep) {
	ps->wmm = NULL;
	return;
      }
    }
  }
#endif
  InitializeCriticalSection(&ps->lock);
  ps->abort = 0;
  ps->thread = CreateThread(NULL, 0, profile_thread, ps, 0, NULL);
}

/* Stop profiling timer thread. */
static void profile_timer_stop(ProfileState *ps)
{
  ps->abort = 1;
  WaitForSingleObject(ps->thread, INFINITE);
  DeleteCriticalSection(&ps->lock);
}

#endif

