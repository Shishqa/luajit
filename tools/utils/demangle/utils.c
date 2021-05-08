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

	fd = open(path, O_RDONLY);

	if (-1 == fd) {
		return NULL;
  }

	if (-1 == fstat(fd, &st)) {
		goto err_exit;
  }

	sz = st.st_size;

	if (0 == sz) {
		goto err_exit;
  }

	buf = mmap(0, sz, PROT_READ, MAP_SHARED, fd, 0);

	if (MAP_FAILED == buf) {
		goto err_exit;
  }

	close(fd); /* Igore errors. */
	*fsize = sz;

	return (char *)buf;
err_exit:

	close(fd); /* Igore errors. */
	return NULL;
}

int ujpp_utils_unmap_file(void *mem, size_t sz)
{
	return munmap(mem, sz);
}
