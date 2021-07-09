#ifndef _LJ_SYSPROF_IMPL_H
#define _LJ_SYSPROF_IMPL_H

#include <pthread.h>

#include "../lj_obj.h"
#include "../lj_sigtimer.h"
#include "../lj_wbuf.h"
#include "../lmisclib.h"

#include "iobuffer.h"

enum profiling_state {
  IDLE,
  RUNNING,
  ERROR
};

enum profiler_detail {
  BACKTRACE_BUF_SIZE = 4096,
  PROFILING_INTERVAL_DEFAULT = 11
};

struct profiler_state {
  /* State of the profiler */
  enum profiling_state state;
  /* Currently profiling VM */
  global_State *g;
  /* Thread, where profiling started */
  pthread_t thread;
  /* State of the VM in the moment of interruption */ 
  int vmstate; 
  /* Buffer for event stream output */
  struct lj_wbuf buf;                     
  struct iobuffer iobuf;
  /* Buffer for backtrace output */
  void* backtrace_buf[BACKTRACE_BUF_SIZE];  
  struct luam_sysprof_data data;            
  struct luam_sysprof_options opt;
  struct sigtimer timer;
};

int stream_is_needed(struct profiler_state *ps);

void stream_prologue(struct profiler_state *ps);

void stream_epilogue(struct profiler_state *ps);

void stream_event(struct profiler_state *ps, uint32_t vmstate);

#endif 
