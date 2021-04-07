#ifndef BUFFER
#define BUFFER

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum IOBUFFER_CONSTANTS { DEFAULT_BUF_SIZE = 4096 };

struct iobuffer {
  int fd;
  uint8_t buf[DEFAULT_BUF_SIZE];
};

void init_iobuf(struct iobuffer *buf, int fd);

size_t flush_iobuf(const void **data, size_t len, void *opt);

#endif /* ifndef BUFFER */
