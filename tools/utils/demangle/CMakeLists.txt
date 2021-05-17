cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(sysprof_demangle SHARED
  demangle.h
  demangle.c
  elf.h
  elf.c
  utils.h
  utils.c
  vector.h
  vector.c
  )

target_link_libraries(sysprof_demangle -lstdc++)
