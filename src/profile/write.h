#ifndef WRITE
#define WRITE

#include <assert.h>
#include <stdint.h>

#include "profile_impl.h"
#include "callchain.h"
#include "profile.h"

void write_finalize(struct profiler_state* ps);

void print_counters(struct profiler_state* ps);

void write_stack(struct profiler_state *ps, uint32_t vmstate);

void write_lfunc(struct profiler_state *ps);
void write_cfunc(struct profiler_state *ps);
void write_dummy(struct profiler_state *ps);


#endif /* ifndef WRITE */
