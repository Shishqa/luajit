#include "write.h"

#include <stdint.h>
#include <stdio.h>

#include "callchain.h"
#include "profile_impl.h"

void write_finalize(struct profiler_state *ps) 
{ 
  write_iobuf(&ps->obuf, "\n", 1); 
}

void print_counters(struct profiler_state *ps) 
{
  for (size_t vmstate = 0; vmstate <= LJ_VMST_TRACE; vmstate++) {
    printf("state %lu: %lu\n", vmstate, ps->data.vmstate[vmstate]);
  }
}

void write_dummy(struct profiler_state *ps) {
  UNUSED(ps);
}

void write_lfunc(struct profiler_state* ps) {
  assert(ps != NULL);

  dump_callchain_cfunc(ps);
  //dump_callchain_lfunc(ps);
  write_finalize(ps);
}

void write_cfunc(struct profiler_state *ps) {
  assert(ps != NULL);

  dump_callchain_cfunc(ps);
  write_finalize(ps);
}

typedef void (*stacktrace_func)(struct profiler_state *ps);

static stacktrace_func stacktrace_handlers[] = {
  write_dummy, /* LJ_VMST_INTERP */
  write_lfunc, /* LJ_VMST_LFUNC */
  write_dummy, /* LJ_VMST_FFUNC */
  write_cfunc, /* LJ_VMST_CFUNC */
  write_dummy, /* LJ_VMST_GC */
  write_dummy, /* LJ_VMST_EXIT */
  write_dummy, /* LJ_VMST_RECORD */
  write_dummy, /* LJ_VMST_OPT */
  write_dummy, /* LJ_VMST_ASM */
  write_dummy  /* TRACE */
};

void write_stack(struct profiler_state *ps, uint32_t vmstate) {
  assert(ps != NULL);

  stacktrace_func handler;
  handler = stacktrace_handlers[vmstate];
  assert(NULL != handler);
  handler(ps);
}

/*
void write_lfunc_callback(void* data, lua_State* L, int samples, int vmstate) {
  ProfileState* ps = data;
  if (ps->vmstate == LFUNC) {
    write_lfunc(ps);
  }
}
*/
