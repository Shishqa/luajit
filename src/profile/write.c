#include "write.h"

#include <stdio.h>

#include "callchain.h"

void write_finalize(ProfileState* ps) { write_iobuf(&ps->obuf, "\n", 1); }

void print_counters(ProfileState* ps) {
  assert(ps != NULL);

  printf(
      "counters:\n"
      "\tlfunc: %lu\n"
      "\tffunc: %lu\n"
      "\tcfunc: %lu\n"
      "\tinterp: %lu\n"
      "\ttrace: %lu\n"
      "\tjit compiler: %lu\n"
      "\tgarbage collector: %lu\n",
      ps->counters.vmstate[LFUNC], ps->counters.vmstate[FFUNC],
      ps->counters.vmstate[CFUNC], ps->counters.vmstate[INTERP],
      ps->counters.vmstate[NATIVE], ps->counters.vmstate[JITCOMP],
      ps->counters.vmstate[GCOLL]);
}

void write_trace(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[NATIVE];
}

void write_lfunc(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[LFUNC];
  dump_callchain_cfunc(ps);
  dump_callchain_lfunc(ps);
  write_finalize(ps);
}

void write_ffunc(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[INTERP];
}

void write_cfunc(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[CFUNC];
  dump_callchain_cfunc(ps);
  write_finalize(ps);
}

void write_interp(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[INTERP];
}
void write_gcoll(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[GCOLL];
}
void write_jitcomp(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[JITCOMP];
}

void write_symtab(const struct global_State* g) {
  assert(g != NULL);

  const GCRef* iter = &g->gc.root;
  const GCobj* o;

  while ((o = gcref(*iter)) != NULL) {
    switch (o->gch.gct) {
      case (~LJ_TPROTO): {
        const GCproto* pt = gco2pt(o);
        printf("%lu %s %lu\n", (uintptr_t)pt, proto_chunknamestr(pt),
               (uint64_t)pt->firstline);
        break;
      }
      default:
        break;
    }
    iter = &o->gch.nextgc;
  }
}

typedef void (*stacktrace_func)(ProfileState* ps);
static stacktrace_func stacktrace_handlers[] = {
    write_trace, write_interp, write_lfunc,  write_ffunc,
    write_cfunc, write_gcoll,  write_jitcomp};

void write_stack(ProfileState* ps) {
  assert(ps != NULL);

  stacktrace_func handler;
  handler = stacktrace_handlers[ps->vmstate];
  assert(NULL != handler);
  handler(ps);
}

void write_lfunc_callback(void* data, lua_State* L, int samples, int vmstate) {
  ProfileState* ps = data;
  if (ps->vmstate == LFUNC) {
    write_lfunc(ps);
  }
}
