#ifndef CALLCHAIN
#define CALLCHAIN

#include <execinfo.h>
#include <assert.h>

#include "profile_impl.h"
#include "profile.h"

void dump_lua_state(struct profiler_state* ps);

void dump_callchain_lua(struct profiler_state* ps);

void dump_callchain_native(struct profiler_state* ps);

void dump_callchain_trace(struct profiler_state* ps);

#endif /* ifndef CALLCHAIN */
