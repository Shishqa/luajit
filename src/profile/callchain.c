#include "callchain.h"

#include "../luajit.h"

void dump_callchain_lfunc(ProfileState* ps) {
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

  write_iobuf(&ps->obuf, stack_dump, dumpstr_len);
}

void dump_callchain_cfunc(ProfileState* ps) {
  const int depth = backtrace(ps->backtrace_buf, DEFAULT_BUF_SIZE);

  char** names = backtrace_symbols(ps->backtrace_buf, depth);

  for (ssize_t i = depth - 1; i >= 0; --i) {
    write_iobuf(&ps->obuf, names[i],
                strlen(names[i]));  // FIXME definetely will cause massive
                                    // profiling overhead
    write_iobuf(&ps->obuf, ";", 1);
  }

  free(names);
}

