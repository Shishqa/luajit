#ifndef _LJ_SYSPROF_H
#define _LJ_SYSPROF_H

#include "luajit.h"

#define VMST_COUNT 10  // FIXME: define vmstates for user?

struct lj_sysprof_data {
  /* Number of samples collected */
  uint64_t samples;
  /* Timer overrun */
  uint64_t overruns;
  /* Vmstate counters */
  uint64_t vmstate[VMST_COUNT];
};

/* Profiling mode:
** > PROFILE_DEFAULT collects only data described in lj_sysprof_data,
**   which can be collected with lj_sysprof_report after profiler stops.
**   This mode doesn't use i/o for dumping data.
** > PROFILE_LEAF: TODO
** > PROFILE_CALLGRAPH: TODO
*/
enum lj_sysprof_mode { 
  PROFILE_DEFAULT   = 0, 
  PROFILE_LEAF      = 1, 
  PROFILE_CALLGRAPH = 2
};

/* Profiler options */
struct lj_sysprof_options {
  /* Context for the profile writer and final callback. */
  // void *ctx;
  /* Custom buffer to write data */
  // uint8_t *buf;
  /* The buffer's size */
  // size_t buf_len;
  /* Writer function for profile events */
  // lj_wbuf_writer writer;
  /* Callback on profiler stopping */
  // int (*on_stop)(void *ctx, uint8_t *buf);
  /* Sampling interval in msec */
  uint32_t interval;
  /* Profiling mode */
  enum lj_sysprof_mode mode;
  /* Output path */
  const char *path;
};

enum lj_sysprof_err {
  SYSPROF_SUCCESS = 0,
  SYSPROF_ERRUSE  = 1,
  SYSPROF_ERRRUN  = 2,
  SYSPROF_ERRIO   = 3,
};

/* Start the sysprof profiler
 * Returns TODO
 */
int lj_sysprof_start(struct lua_State *L, const struct lj_sysprof_options *opt);

/* Stops the sysprof profiler
 * Returns TODO
 */
int lj_sysprof_stop(struct lua_State *L);

/* Sysprof public C API. */

/*
LUAMISC_API void luaM_sysprof_start(lua_State *L, const char *mode, 
                                  const char *path);

LUAMISC_API void luaM_sysprof_stop(lua_State *L);

LUAMISC_API void luaM_sysprof_report(struct luam_sysprof_data *data);
*/

#endif
