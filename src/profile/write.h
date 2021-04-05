#ifndef WRITE
#define WRITE

#include <assert.h>
#include <stdint.h>

#include "../lj_profile.h"
#include "../lj_profile_state_impl.h"
#include "callchain.h"
#include "profile.h"

void print_counters(ProfileState* ps);

void write_trace(ProfileState* ps);
void write_lfunc(ProfileState* ps);
void write_ffunc(ProfileState* ps);
void write_cfunc(ProfileState* ps);
void write_stack(ProfileState* ps);
void write_interp(ProfileState* ps);
void write_gcoll(ProfileState* ps);
void write_jitcomp(ProfileState* ps);

void write_symtab(const struct global_State* g);

void write_lfunc_callback(void* data, lua_State* L, int samples, int vmstate);

#endif /* ifndef WRITE */
