#define _GNU_SOURCE 1

#include <stdio.h>

#include <unistd.h>
#include <link.h>
#include <sys/auxv.h>

#include "../lj_alloc.h"

#include "shared_objects.h"


static int count_shared_obj(struct dl_phdr_info *info, size_t size, void *counter) {
  (*(size_t *)counter)++;
  return 0;
}

static int write_shared_obj(struct dl_phdr_info *info, size_t size, void *data) 
{
  struct lj_wbuf *buf = data;

  if (info->dlpi_name && strlen(info->dlpi_name)) {
    lj_wbuf_addstring(buf, info->dlpi_name);
    printf("%s %lu\n", info->dlpi_name, info->dlpi_addr);
  } else {
    static const size_t SO_MAX_PATH_LENGTH = 256;
    char name[SO_MAX_PATH_LENGTH] = {};
    ssize_t bytes_read = readlink("/proc/self/exe", name, SO_MAX_PATH_LENGTH);
    if (-1 != bytes_read) {
      bytes_read = bytes_read < SO_MAX_PATH_LENGTH ? bytes_read : SO_MAX_PATH_LENGTH - 1;
      name[bytes_read] = '\0';
      lj_wbuf_addstring(buf, name);
      printf("%s %lu\n", name, info->dlpi_addr);
    }
  }

  lj_wbuf_addu64(buf, info->dlpi_addr);
  
  return 0;
}


void so_dump(struct lj_wbuf *buf) {
  
  size_t num = 0;
  dl_iterate_phdr(count_shared_obj, &num);

  lj_wbuf_addu64(buf, num);

  printf("shared objects: %lu\n", num);
  dl_iterate_phdr(write_shared_obj, buf);
}

