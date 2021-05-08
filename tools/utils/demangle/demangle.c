/*
 * This module provides functionality to demangle dumped address using symbol
 * tables.
 * Copyright (C) 2020-2021 LuaVela Authors. See Copyright Notice in COPYRIGHT
 * Copyright (C) 2015-2020 IPONWEB Ltd. See Copyright Notice in COPYRIGHT
 */

#include <libgen.h>
#include <sys/stat.h>
#include <sys/auxv.h>

#include <string.h>

#include "demangle.h"
#include "utils.h"
#include "elf.h"

enum {
  MAPS_LINE_LEN = 256
};

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

static int demangle_cmp_syms(const void *sym_first, const void *sym_second)
{
	const struct sym_info *si1 = *(const struct sym_info **)sym_first;
	const struct sym_info *si2 = *(const struct sym_info **)sym_second;

	return (int)((int64_t)si1->addr - (int64_t)si2->addr);
}

static size_t demangle_determine_vdso_size(void)
{
	FILE *fp;
	char str[MAPS_LINE_LEN];

	fp = fopen("/proc/self/maps", "r");

	if (fp == NULL)
		return 0;

	while (fgets(str, sizeof(str), fp) != NULL) {
		char *delim;
		void *start;
		void *end;

		if (!strstr(str, "vdso"))
			continue;

		delim = strstr(str, "-");

		if (delim == NULL) {
			fclose(fp);
			return 0;
		}

		sscanf(str, "%p", &start);
		sscanf(delim + 1, "%p", &end);

		fclose(fp);
		return (size_t)((uint8_t *)end - (uint8_t *)start);
	}

	fclose(fp);
	return 0;
}

struct shared_obj* load_so(const char *path, uint64_t base)
{
	struct shared_obj *so;
	struct stat st;
	int found = stat(path, &st) != -1;

  enum so_type type = utils_obj_type(path);

	so = ujpp_utils_allocz(sizeof(*so));
	ujpp_vector_init(&so->symbols);

	so->path = path;
	so->base = base;
	so->short_name = basename((char *)so->path);
	so->found = found != 0;

	switch (type) {
	case SO_BIN: {
		so->size = ujpp_elf_text_sz(so->path);

		if (so->found)
			ujpp_elf_parse_file(so->path, &so->symbols);
		break;
	}
	case SO_VDSO: {
		const char *vdso = (const char *)getauxval(AT_SYSINFO_EHDR);
		size_t vdso_sz = demangle_determine_vdso_size();

		/* getauxval() fails under valgrind for example. */
		if (vdso == NULL || vdso_sz == 0)
			break;

		so->size = vdso_sz;
		so->found = 1;
		ujpp_elf_parse_mem(vdso, &so->symbols);
		break;
	}
	case SO_SHARED: {
		if (found) {
			so->size = st.st_size;
			ujpp_elf_parse_file(so->path, &so->symbols);
		}
		break;
	}
	default:
		ujpp_utils_die("Wrong object type %u", type);
	}

	if (so->found) {
    fprintf(stderr, "found %lu!\n", so->symbols.size);
  } else {
    fprintf(stderr, "not found\n");
  }
  
  return so;
}

void ujpp_demangle_free_symtab(struct shared_obj *so)
{
	for (size_t i = 0; i < so->symbols.size; i++) {
		const struct sym_info *si = so->symbols.elems[i];

		free((void *)si->name);
	}

	ujpp_vector_free(&so->symbols);
}
