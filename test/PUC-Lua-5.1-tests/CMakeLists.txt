# Test suite that has been added from PUC-Rio Lua 5.1 test archive
# in scope of https://github.com/tarantool/tarantool/issues/4473.

# See the rationale in the root CMakeLists.txt.
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(TEST_RUNNER  ${CMAKE_CURRENT_SOURCE_DIR}/all.lua)
set(LIB_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(TESTLIB_PATH ${CMAKE_CURRENT_BINARY_DIR}/libs)

# Build additional C libraries for tests.
macro(build_lib lib sources)
  add_library(${lib} SHARED EXCLUDE_FROM_ALL ${sources})
  target_include_directories(${lib} PRIVATE
    ${LUAJIT_SOURCE_DIR}
  )
  set_target_properties(${lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${TESTLIB_PATH}"
    PREFIX ""
  )
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(${lib} PROPERTIES
      LINK_FLAGS "-undefined dynamic_lookup"
    )
  endif()
  target_link_libraries(${lib} PRIVATE libluajit_shared)
  list(APPEND TESTLIBS ${lib})
endmacro()

build_lib(lib1  ${LIB_SOURCES}/lib1.c)
build_lib(lib11 ${LIB_SOURCES}/lib1.c ${LIB_SOURCES}/lib11.c)
build_lib(lib2  ${LIB_SOURCES}/lib2.c)
build_lib(lib21 ${LIB_SOURCES}/lib2.c ${LIB_SOURCES}/lib21.c)

set(LIB2COPY "${TESTLIB_PATH}/lib2${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(LIB_COPY "${TESTLIB_PATH}/-lib2${CMAKE_SHARED_LIBRARY_SUFFIX}")

add_custom_command(
  COMMENT "Copping lib2 to -lib2 for PUC-Rio Lua 5.1 tests"
  OUTPUT  ${LIB_COPY}
  DEPENDS ${TESTLIBS}
  COMMAND ${CMAKE_COMMAND} -E copy ${LIB2COPY} ${LIB_COPY}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
list(APPEND TESTLIBS ${LIB_COPY})

set(CUSTOM_TEST_DIR ${TESTLIB_PATH}/P1)
add_custom_command(
  COMMENT "Create directory for PUC-Rio Lua 5.1 tests"
  OUTPUT  ${CUSTOM_TEST_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CUSTOM_TEST_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set(LUA_PATH "?\;${CMAKE_CURRENT_SOURCE_DIR}/?.lua")

# TODO: PUC-Rio Lua 5.1 test suite also has special header
# <ltests.h> and <ltests.c> translation unit to check some
# internal behaviour of the Lua implementation (see etc/
# directory). It modifies realloc function to check memory
# consistency and also contains tests for yield in hooks
# and for the Lua C API.
# But, unfortunately, <ltests.c> depends on specific PUC-Rio Lua 5.1
# internal headers and should be adopted for LuaJIT.

add_custom_target(PUC-Lua-5.1-tests
  DEPENDS ${LUAJIT_TEST_BINARY} ${TESTLIBS} ${CUSTOM_TEST_DIR}
)

add_custom_command(TARGET PUC-Lua-5.1-tests
  COMMENT "Running PUC-Rio Lua 5.1 tests"
  COMMAND
  env
    # Tarantool doesn't support LUA_INIT and most likely it
    # never will.
    # See https://github.com/tarantool/tarantool/issues/5744
    # for more info.
    # LUA_PATH="${CMAKE_CURRENT_BINARY_DIR}/?.lua\;\;"
    # LUA_INIT="package.path='?\;'..package.path"
    # So use less preferable way for tests.
    LUA_PATH="${LUA_PATH}\;\;"
    # XXX: Adapt to run test on readonly file systems
    # with loadfile(CUR_SOURCE_DIR..filename).
    CUR_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    ${LUAJIT_TEST_COMMAND} ${TEST_RUNNER}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# vim: expandtab tabstop=2 shiftwidth=2
