#define _GNU_SOURCE 1

#include <stdio.h>

#include <link.h>
#include <sys/auxv.h>
#include <unistd.h>

#include "shared_objects.h"

static int count_shared_obj(struct dl_phdr_info *info, size_t size,
                            void *counter) {
  (*(size_t *)counter)++;
  return 0;
}

static void write_so_entry(struct lj_wbuf *buf, const char *name,
                           ElfW(Addr) addr) {
  lj_wbuf_addstring(buf, name);
  lj_wbuf_addu64(buf, addr);
//  printf("%s %lu\n", name, addr);
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

void so_dump(struct lj_wbuf *buf) {

  size_t num = 0;
  dl_iterate_phdr(count_shared_obj, &num);

  lj_wbuf_addu64(buf, num);

  //printf("shared objects: %lu\n", num);
  dl_iterate_phdr(write_shared_obj, buf);
}
