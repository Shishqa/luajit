#include "write.h"

#include <stdio.h>

void print_counters(ProfileState* ps) {
  assert(ps != NULL);

  printf(
      "counters:\n"
      "\tlfunc: %lu\n"
      "\tffunc: %lu\n"
      "\tcfunc: %lu\n"
      "\tinterp: %lu\n"
      "\ttrace: %lu\n",
      ps->counters.vmstate[I], ps->counters.vmstate[I], ps->counters.vmstate[C],
      ps->counters.vmstate[J], ps->counters.vmstate[N]);  // XXX not sure about states
}

void write_trace(ProfileState* ps) { ++ps->counters.vmstate[N]; }

void write_lfunc(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[I];
  dump_callchain_lfunc(ps);
}

void write_ffunc(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[I];
}

void write_cfunc(ProfileState* ps) {
  assert(ps != NULL);

  ++ps->counters.vmstate[C];
  dump_callchain_cfunc(ps);
}

void write_vmstate(ProfileState* ps) { ++ps->counters.vmstate[I]; }

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

