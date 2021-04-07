#ifndef PROFILE
#define PROFILE

#include "lua.h"

enum lj_sysprof_mode {
  PROFILE_DEFAULT,
  PROFILE_LEAF,
  PROFILE_CALLGRAPH
};

struct lj_sysprof_options {
  int fd;
  uint32_t interval;
  enum lj_sysprof_mode mode;
};

LUA_API void lj_sysprof_start(struct lua_State *L, const struct lj_sysprof_options *opt);

LUA_API void lj_sysprof_stop(struct lua_State *L);

#endif /* ifndef PROFILE */
