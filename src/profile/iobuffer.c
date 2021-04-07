#include "iobuffer.h"
#include "../lj_def.h"

#include <errno.h>
#include <stdio.h>

void init_iobuf(struct iobuffer* buf, int fd) {
  memset(buf->buf, 0, sizeof(buf->buf));
  buf->fd = fd;
}

size_t flush_iobuf(const void **buf_addr, size_t len, void *ctx) {
  struct iobuffer *iobuf = ctx;
  int fd = iobuf->fd;
  
  const void * const buf_start = *buf_addr;
  const void *data = *buf_addr;
  size_t write_total = 0;

  for (;;) {
    const ssize_t written = write(fd, data, len - write_total);

    if (LJ_UNLIKELY(written <= 0)) {
      /* Re-tries write in case of EINTR. */
      if (errno != EINTR) {
	      /* Will be freed as whole chunk later. */
	      *buf_addr = NULL;
	      return write_total;
      }

      errno = 0;
      continue;
    }

    write_total += written;
    lua_assert(write_total <= len);

    if (write_total == len)
      break;

    data = (uint8_t *)data + (ptrdiff_t)written;
  }

  *buf_addr = buf_start;

  return write_total;
}
