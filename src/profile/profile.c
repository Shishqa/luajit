/*============================================================================*/

#include "profile.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "../lauxlib.h"
#include "../luajit.h"
#include "../lualib.h"
#include "write.h"

#include "../lj_obj.h"
#include "../lj_frame.h"
#include "../lj_debug.h"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static struct profiler_state profiler_state;

void init_profiler_state(struct profiler_state *ps, int fd) {
  init_buf(&ps->buf, fd, DEFAULT_BUF_SIZE);
  ps->vmstate = 0;
  memset(ps->data.vmstate, 0, sizeof(ps->data.vmstate));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void profile_callback(void *data, lua_State *L, int samples, int vmstate) {
  struct profiler_state *ps = (struct profiler_state *)(data);

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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void dump_symtab(const struct global_State* g) {
  assert(g != NULL);

  const GCRef *iter = &g->gc.root;
  const GCobj *o;

  while ((o = gcref(*iter)) != NULL) {
    switch (o->gch.gct) {
    case (~LJ_TPROTO): {
      const GCproto *pt = gco2pt(o);
      printf("%lu %s %lu\n", 
             (uintptr_t)pt, 
             proto_chunknamestr(pt),
             (uint64_t)pt->firstline);
      break;
    }
    default:
      break;
    }
    iter = &o->gch.nextgc;
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// API

LUA_API void profile_start(lua_State *L, int fd) {
  struct profiler_state *ps = &profiler_state;
  init_profiler_state(ps, fd);

  dump_symtab(G(L));

  luaJIT_profile_start(L, "f", profile_callback, ps);
}

LUA_API void profile_stop(lua_State *L) {
  luaJIT_profile_stop(L);

  struct profiler_state *ps = &profiler_state;

  for (size_t state = 0; state < STATE_MAX; ++state) {
    printf("State %lu -> %lu\n", state, ps->data.vmstate[state]);
  }
  print_counters();

  dump_symtab(G(L));

  flush_buf(&ps->buf);
  release_buf(&ps->buf);
}

/*============================================================================*/
