#include "symtab.h"
#include "../lj_trace.h"
#include "../lj_wbuf.h"
#include <stdint.h>
#include <stdio.h>

static const unsigned char ljs_header[] = {'l', 'j', 's', LJS_CURRENT_VERSION,
					   0x0, 0x0, 0x0};


static void write_symtab_prologue(struct lj_wbuf *out) 
{
  const size_t ljs_header_len = sizeof(ljs_header) / sizeof(ljs_header[0]);
  lj_wbuf_addn(out, (const char*)ljs_header, ljs_header_len);
}

void write_symtab(struct lj_wbuf *out, const struct global_State *g)
{
  write_symtab_prologue(out);

  const GCRef *iter = &g->gc.root;
  const GCobj *o;
  while ((o = gcref(*iter)) != NULL) {
    switch (o->gch.gct) {
    case (~LJ_TPROTO): {
      const GCproto *pt = gco2pt(o);
      printf("lfunc: %lu %s %lu \n", 
             (uintptr_t)pt, 
             proto_chunknamestr(pt), 
             (uint64_t)pt->firstline);
      lj_wbuf_addbyte(out, SYMTAB_LFUNC);
      lj_wbuf_addu64(out, (uintptr_t)pt);
      lj_wbuf_addstring(out, proto_chunknamestr(pt));
      lj_wbuf_addu64(out, (uint64_t)pt->firstline);
      break;
    }
    case (~LJ_TTRACE): {
      const GCtrace *trace = gco2trace(o);
      const GCproto *start_pt = gco2pt(gcref(trace->startpt));
      printf("trace: %lu %lu %lu %lu \n",
             (uintptr_t)trace->traceno,
             (uintptr_t)start_pt,
             (uintptr_t)trace->mcode,
             (uintptr_t)trace->szmcode);
      lj_wbuf_addbyte(out, SYMTAB_TRACE);
      lj_wbuf_addu64(out, (uintptr_t)trace->traceno);
      lj_wbuf_addu64(out, (uintptr_t)start_pt);
      lj_wbuf_addu64(out, (uintptr_t)trace->mcode);
      lj_wbuf_addu64(out, (uintptr_t)trace->szmcode);
    }
    default:
      break;
    }
    iter = &o->gch.nextgc;
  }

  lj_wbuf_addbyte(out, SYMTAB_FINAL);
}
