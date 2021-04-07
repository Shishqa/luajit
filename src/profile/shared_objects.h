#ifndef SHARED_OBJECTS
#define SHARED_OBJECTS

#include "../lj_def.h"
#include "../lj_wbuf.h"
#include <stdint.h>

struct lua_State;

void so_dump(struct lj_wbuf *buf);

#endif /* ifndef SHARED_OBJECTS */
