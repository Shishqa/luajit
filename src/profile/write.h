#ifndef WRITE
#define WRITE

#include <assert.h>
#include <stdint.h>

#include "callchain.h"
#include "profile.h"
#include "../lj_profile.h"
#include "../lj_profile_state_impl.h"

void print_counters();

void write_trace(ProfileState* ps);
void write_lfunc(ProfileState* ps);
void write_ffunc(ProfileState* ps);
void write_cfunc(ProfileState* ps);
void write_vmstate(ProfileState* ps);
void write_symtab(const struct global_State* g);

#endif /* ifndef WRITE */
