#ifndef PROFILE
#define PROFILE

#include "lua.h"

enum profiler_mode {
  PROFILE_DEFAULT,
  PROFILE_LEAF,
  PROFILE_CALLGRAPH
};

struct profiler_opt {
  uint32_t interval;
  enum profiler_mode mode;
};

LUA_API void profile_start(struct lua_State *L, const struct profiler_opt *opt, 
                           int fd);

LUA_API void profile_stop(void);

#endif /* ifndef PROFILE */
