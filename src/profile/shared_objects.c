#define _GNU_SOURCE 1

#include <stdio.h>

#include <link.h>
#include <sys/auxv.h>
#include <unistd.h>

#include "shared_objects.h"

static void write_so_entry(struct lj_wbuf *buf, const char *name,
                           ElfW(Addr) addr) {
  lj_wbuf_addbyte(buf, SOTAB_SHARED);
  lj_wbuf_addu64(buf, addr);
  lj_wbuf_addstring(buf, name);
}

static int write_shared_obj(struct dl_phdr_info *info, size_t size,
                            void *data) {
  static const size_t SO_MAX_PATH_LENGTH = 256;
  char name[SO_MAX_PATH_LENGTH] = {};

  struct lj_wbuf *buf = data;

  if (info->dlpi_name && strlen(info->dlpi_name)) {
    write_so_entry(buf, info->dlpi_name, info->dlpi_addr);
  } else {
    ssize_t bytes_read = readlink("/proc/self/exe", name, SO_MAX_PATH_LENGTH);
    if (-1 != bytes_read) {
      bytes_read =
          bytes_read < SO_MAX_PATH_LENGTH ? bytes_read : SO_MAX_PATH_LENGTH - 1;
      name[bytes_read] = '\0';
      write_so_entry(buf, name, info->dlpi_addr);
    }
  }

  return 0;
}

static const uint8_t ljso_header[] = {'l', 'j', 's', 'o', LJSO_CURRENT_VERSION,
                                      0x0, 0x0, 0x0};

void so_dump(struct lj_wbuf *buf) {

  lj_wbuf_addn(buf, ljso_header, sizeof(ljso_header));

  dl_iterate_phdr(write_shared_obj, buf);

  lj_wbuf_addbyte(buf, SOTAB_FINAL);
}
