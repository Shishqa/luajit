#define stream_c
#define LUA_CORE

#include "callchain.h"
#include "sysprof_impl.h"

#include "sotab.h"
#include "../lj_prof_symtab.h"

#include "../lj_wbuf.h"

enum { LJP_FORMAT_VERSION = 0x1 };

static const uint8_t ljp_header[] = {'l', 'j', 'p', LJP_FORMAT_VERSION,
                                     0x0, 0x0, 0x0};

enum { LJP_EPILOGUE_BYTE = 0xBB };

void stream_prologue(struct profiler_state *ps) 
{
  lj_prof_dump_symtab(&ps->buf, ps->g);
  lj_prof_dump_sotab(&ps->buf);
  lj_wbuf_addn(&ps->buf, ljp_header, sizeof(ljp_header));
}

void stream_epilogue(struct profiler_state *ps) 
{
  lj_wbuf_addbyte(&ps->buf, LJP_EPILOGUE_BYTE);
  lj_wbuf_addu64(&ps->buf, ps->data.samples);
  lj_wbuf_addu64(&ps->buf, ps->data.overruns);
}

void write_lua(struct profiler_state *ps) 
{
  dump_callchain_lua(ps);
  dump_callchain_host(ps);
}

void write_host(struct profiler_state *ps) 
{ 
  dump_callchain_host(ps); 
}

void write_trace(struct profiler_state *ps) 
{
  dump_callchain_trace(ps);
  dump_callchain_lua(ps); // failing :( 
  // todo: host?
}

typedef void (*event_handler)(struct profiler_state *ps);

static event_handler event_handlers[] = {
    write_host,  /* LJ_VMST_INTERP */
    write_lua,   /* LJ_VMST_LFUNC */
    write_lua,   /* LJ_VMST_FFUNC */
    write_lua,   /* LJ_VMST_CFUNC */
    write_host,  /* LJ_VMST_GC */
    write_host,  /* LJ_VMST_EXIT */
    write_host,  /* LJ_VMST_RECORD */
    write_host,  /* LJ_VMST_OPT */
    write_host,  /* LJ_VMST_ASM */
    write_trace  /* LJ_VMST_TRACE */
};

void stream_event(struct profiler_state *ps, uint32_t vmstate) 
{  
  lua_assert((vmstate & ~(uint32_t)0xf) == 0);
  lj_wbuf_addbyte(&ps->buf, (uint8_t)vmstate);

  event_handler handler = event_handlers[vmstate];
  lua_assert(NULL != handler);
  handler(ps);
}

