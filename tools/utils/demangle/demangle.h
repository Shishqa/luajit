/*
 * Interfaces for C++ symbols demangling.
 * Copyright (C) 2020-2021 LuaVela Authors. See Copyright Notice in COPYRIGHT
 * Copyright (C) 2015-2020 IPONWEB Ltd. See Copyright Notice in COPYRIGHT
 */

#ifndef _UJPP_DEMANGLE_H
#define _UJPP_DEMANGLE_H

#include <inttypes.h>

#include "vector.h"
#include "utils.h"

struct sym_info {
	uint64_t addr; /* Raw offset to symbol */
	uint64_t size; /* Physical size in the SO*/
	const char *name; /* Symbol */
};

struct shared_obj {
	const char *path; /* Full path to object */
	const char *short_name; /* Basename */
	uint64_t base; /* VA */
	uint64_t size; /* Size from stat() function */
	struct vector symbols; /* Symbols from nm */
	uint8_t found;
};

/* Free all mem used for symbols */
void ujpp_demangle_free_symtab(struct shared_obj *so);
/* Reads symbol table of given shared object */
struct shared_obj* load_so(const char *path, uint64_t base);

#endif /* !_UJPP_DEMANGLE_H */
