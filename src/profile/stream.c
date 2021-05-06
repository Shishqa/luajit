#include "callchain.h"
#include "profile_impl.h"

#include "../lj_wbuf.h"

void write_full_info(struct profiler_state *ps) {
  dump_callchain_lua(ps);
  dump_callchain_native(ps);
}

void write_native(struct profiler_state *ps) { dump_callchain_native(ps); }

void write_stub(struct profiler_state *ps) { UNUSED(ps); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

typedef void (*event_handler)(struct profiler_state *ps);

static event_handler event_handlers[] = {
    write_stub,      /* LJ_VMST_INTERP */
    write_full_info, /* LJ_VMST_LFUNC */
    write_full_info, /* LJ_VMST_FFUNC */
    write_full_info, /* LJ_VMST_CFUNC */
    write_stub,      /* LJ_VMST_GC */
    write_stub,      /* LJ_VMST_EXIT */
    write_stub,      /* LJ_VMST_RECORD */
    write_stub,      /* LJ_VMST_OPT */
    write_stub,      /* LJ_VMST_ASM */
    write_native     /* TRACE */
};

void stream_event(struct profiler_state *ps, uint32_t vmstate) {
  lua_assert((vmstate & ~(uint32_t)0xf) == 0);
  lj_wbuf_addbyte(&ps->buf, (uint8_t)vmstate);

  event_handler handler;
  handler = event_handlers[vmstate];
  lua_assert(NULL != handler);
  handler(ps);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
