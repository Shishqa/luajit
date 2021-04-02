/*============================================================================*/

#include "profile.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lauxlib.h"
#include "../lj_debug.h"
#include "../lj_frame.h"
#include "../lj_obj.h"
#include "../luajit.h"
#include "../lualib.h"
#include "iobuffer.h"
#include "write.h"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*
typedef void (*stacktrace_func)(struct profiler_state *ps);

static stacktrace_func stacktrace_handlers[] = {write_trace,   write_lfunc,
                                                write_cfunc,   write_vmstate,
                                                write_vmstate, write_vmstate};

void write_stack(struct profiler_state *ps) {
  stacktrace_func handler;
  handler = stacktrace_handlers[ps->vmstate];
  assert(NULL != handler);
  handler(ps);
}


void get_vm_state(lua_State* L) {
  global_State* g = G(L);
}


void profile_callback() {
  switch (vmstate) {
    case 'N':
      ps->vmstate = N;
      break;
    case 'I':
      ps->vmstate = I;
      break;
    case 'C':
      ps->vmstate = C;
      break;
    case 'G':
      ps->vmstate = G;
      break;
    case 'J':
      ps->vmstate = J;
      break;
    default:
      assert("unreachable");
      break;
  }

  ps->data.vmstate[ps->vmstate]++;
  ps->L = L;

  write_stack(ps);
}
*/
