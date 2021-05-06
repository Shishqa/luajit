cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(demangle)

add_library(demangle SHARED
  main.c
  demangle.h
  demangle.c
  elf.h
  elf.c
  hash.h
  hash.c
  utils.h
  utils.c
  vector.h
  vector.c
  )

target_link_libraries(demangle -lstdc++)
