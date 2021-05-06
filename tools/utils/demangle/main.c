#include "demangle.h"

void demangle(const char* path, uint64_t base) {
  ujpp_demangle_load_so(path, base, SO_SHARED);
}
