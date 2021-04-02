#ifndef BUFFER
#define BUFFER

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum IOBUFFER_CONSTANTS { DEFAULT_BUF_SIZE = 4096 };

struct iobuffer {
  char* data;
  char* pos;
  size_t size;
  int fd;
};

void init_iobuf(struct iobuffer* buf, int fd, size_t size);

ssize_t write_iobuf(struct iobuffer* buf, const char* payload, size_t len);

ssize_t flush_iobuf(struct iobuffer* buf);

void release_iobuf(struct iobuffer* buf);

#endif /* ifndef BUFFER */
