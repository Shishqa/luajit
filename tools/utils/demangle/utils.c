/*
 * This module implements auxiliary functions for ffunc's demangling, qsort
 * comparators, etc.
 * Copyright (C) 2020-2021 LuaVela Authors. See Copyright Notice in COPYRIGHT
 * Copyright (C) 2015-2020 IPONWEB Ltd. See Copyright Notice in COPYRIGHT
 */

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "utils.h"
#include <string.h>

#include <errno.h>

static enum so_type utils_obj_type(const char *path)
{
	int is_vdso = strcmp(path, "vdso") == 0;

	if (strstr(path, ".so") != NULL)
		return SO_SHARED;
	else if (is_vdso)
		return SO_VDSO;
	else
		return SO_BIN;
}

void ujpp_utils_die_nofunc(const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fputc('\n', stderr);
	exit(1);
}

void *ujpp_utils_allocz(size_t sz)
{
	void *p = calloc(1, sz);

	if (p == NULL) {
		ujpp_utils_die("failed to allocate %zu bytes", NULL);
		return NULL; /* unreachable. */
	}

	return p;
}

char *ujpp_utils_map_file(const char *path, size_t *fsize)
{
	int fd;
	size_t sz;
	void *buf;
	struct stat st;

  fprintf(stderr, "MAP\n");

	fd = open(path, O_RDONLY);

	if (-1 == fd) {
    fprintf(stderr, "%d \n\n", errno);
    fprintf(stderr, "\n\n");
		return NULL;
  }

	if (-1 == fstat(fd, &st)) {
    fprintf(stderr, "bad fstat\n");
		goto err_exit;
  }

	sz = st.st_size;

	if (0 == sz) {
    fprintf(stderr, "zero size\n");
		goto err_exit;
  }

	buf = mmap(0, sz, PROT_READ, MAP_SHARED, fd, 0);

	if (MAP_FAILED == buf) {
    fprintf(stderr, "map failed\n");
		goto err_exit;
  }

	close(fd); /* Igore errors. */
	*fsize = sz;

  fprintf(stderr, "BUF!\n");
	return (char *)buf;
err_exit:

  fprintf(stderr, "error???\n");
	close(fd); /* Igore errors. */
	return NULL;
}

int ujpp_utils_unmap_file(void *mem, size_t sz)
{
	return munmap(mem, sz);
}
