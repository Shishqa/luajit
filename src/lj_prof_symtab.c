#define lj_prof_symtab_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_trace.h"

#include "lj_wbuf.h"
#include "lj_prof_symtab.h"

static const unsigned char ljs_header[] = {'l', 'j', 's', LJS_CURRENT_VERSION,
					   0x0, 0x0, 0x0};

void lj_prof_dump_symtab(struct lj_wbuf *out, const struct global_State *g)
{
  const GCRef *iter = &g->gc.root;
  const GCobj *o;
  const size_t ljs_header_len = sizeof(ljs_header) / sizeof(ljs_header[0]);

  /* Write prologue. */
  lj_wbuf_addn(out, ljs_header, ljs_header_len);

  while ((o = gcref(*iter)) != NULL) {
    switch (o->gch.gct) {
    case (~LJ_TPROTO): {
      const GCproto *pt = gco2pt(o);
      lj_wbuf_addbyte(out, SYMTAB_LFUNC);
      lj_wbuf_addu64(out, (uintptr_t)pt);
      lj_wbuf_addstring(out, proto_chunknamestr(pt));
      lj_wbuf_addu64(out, (uint64_t)pt->firstline);
      break;
    }
    /* TODO: dump trace
    case (~LJ_TTRACE): {
      break;
    }
    */
    default:
      break;
    }
    iter = &o->gch.nextgc;
  }

  lj_wbuf_addbyte(out, SYMTAB_FINAL);
}

