#ifndef _LJ_SYSPROF_IOBUFFER_H
#define _LJ_SYSPROF_IOBUFFER_H

#include <stdint.h>
#include <stdlib.h>

#include "../lj_obj.h"

enum IOBUFFER_CONSTANTS { DEFAULT_BUF_SIZE = 4096 };

struct iobuffer {
  int fd;
  global_State *g;
  uint8_t buf[DEFAULT_BUF_SIZE];
};

void init_iobuf(struct iobuffer *buf, global_State *g, int fd);

void terminate_iobuf(struct iobuffer *buf);

size_t flush_iobuf(const void **data, size_t len, void *opt);

#endif  // _LJ_SYSPROF_IOBUFFER_H
