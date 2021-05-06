cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS yes)

project(symbols)

add_executable(main
  main.c
  demangle.c
  demangle.h
  )

