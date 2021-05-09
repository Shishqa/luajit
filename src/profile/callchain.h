#ifndef CALLCHAIN
#define CALLCHAIN

#include "profile_impl.h"

void dump_callchain_lua(struct profiler_state* ps);

void dump_callchain_native(struct profiler_state* ps);

void dump_callchain_trace(struct profiler_state* ps);

#endif /* ifndef CALLCHAIN */
