#ifndef _LJ_SYSPROF_CALLCHAIN_H
#define _LJ_SYSPROF_CALLCHAIN_H

#include "sysprof_impl.h"

void dump_callchain_lua(struct profiler_state* ps);

void dump_callchain_host(struct profiler_state* ps);

void dump_callchain_trace(struct profiler_state* ps);

#endif /* _LJ_SYSPROF_CALLCHAIN_H */
