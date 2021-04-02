#ifndef CALLCHAIN
#define CALLCHAIN

#include <execinfo.h>
#include <assert.h>

#include "../lj_profile_state_impl.h"
#include "profile.h"

void dump_callchain_lfunc(ProfileState* ps);

void dump_callchain_ffunc(ProfileState* ps);

void dump_callchain_cfunc(ProfileState* ps);

void dump_callchain_trace(ProfileState* ps);

// TODO vmstate?

#endif /* ifndef CALLCHAIN */
