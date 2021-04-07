#include "callchain.h"

#include "../luajit.h"
#include "profile_impl.h"
#include <assert.h>

#include <stdio.h>

void dump_callchain_lfunc(struct profiler_state *ps) {
  /* IMPRORTANT
   * This function should be called ONLY from
   * profiler callback since lua_State might be
   * not consistent during signal handling
   * */

  assert(ps != NULL);

  lua_State* L = gco2th(gcref(ps->g->cur_L));
  assert(L != NULL);

  size_t dumpstr_len = 0;

  const char* stack_dump =
      luaJIT_profile_dumpstack(L, "F;", -4000, &dumpstr_len);

  lj_wbuf_addn(&ps->buf, stack_dump, dumpstr_len);
}

void dump_callchain_cfunc(struct profiler_state *ps) {
  const int depth = backtrace(ps->backtrace_buf, 4096);

  char** names = backtrace_symbols(ps->backtrace_buf, depth);

  for (ssize_t i = depth - 1; i >= 0; --i) {
    lj_wbuf_addn(&ps->buf, names[i],
                 strlen(names[i]));  // FIXME definetely will cause massive
                                    // profiling overhead
    lj_wbuf_addn(&ps->buf, ";", 1);
  }

  free(names);
}

