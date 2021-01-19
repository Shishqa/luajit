# Test suite that has been added from lua-Harness test suite
# in scope of https://github.com/tarantool/tarantool/issues/4473.

# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_program(PROVE prove)
if(NOT PROVE)
  message(WARNING "`prove' is not found, so lua-Harness-tests target is not generated")
  return()
endif()

set(LUA_TEST_FLAGS --failures --shuffle)
if(CMAKE_VERBOSE_MAKEFILE)
  list(APPEND LUA_TEST_FLAGS --verbose)
endif()

string(CONCAT LUA_PATH
  "./?.lua\;"
  "${CMAKE_CURRENT_SOURCE_DIR}/?.lua\;"
  "${LUAJIT_SOURCE_DIR}/?.lua\;"
)

string(CONCAT LUA_CPATH
  "./?${CMAKE_SHARED_LIBRARY_SUFFIX}\;"
  "${LUAJIT_SOURCE_DIR}/?${CMAKE_SHARED_LIBRARY_SUFFIX}\;"
)

# FIXME: Until https://github.com/tarantool/tarantool/issues/5040
# is resolved, Tarantool enters interactive mode if prove input
# is not stdin. As a result test hungs and not run at all.
# This part should be dropped, and argument to prove is passed
# as directory.
file(GLOB TESTS_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.t)
string(REPLACE ";" "\n" TESTS_LIST "${TESTS_LIST}")
set(LIST_FILE ${CMAKE_CURRENT_BINARY_DIR}/tests_list)
file(WRITE ${LIST_FILE} ${TESTS_LIST})

add_custom_target(lua-Harness-tests DEPENDS ${LUAJIT_TEST_BINARY} ${LIST_FILE})

add_custom_command(TARGET lua-Harness-tests
  COMMENT "Running lua-Harness tests"
  COMMAND
  env
    LUA_PATH="${LUA_PATH}\;"
    LUA_CPATH="${LUA_CPATH}\;"
    # Tarantool doesn't support LUA_INIT and most likely it
    # never will.
    # See https://github.com/tarantool/tarantool/issues/5744
    # for more info.
    # So use less preferable way for tests.
    # See the root CMakeLists.txt for more info.
    # XXX: Adapt to run test witht Tarantool on read-only
    # file systems with dofile(CUR_SOURCE_DIR..filename).
    CUR_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    ${PROVE} - < ${LIST_FILE}
      --exec '${LUAJIT_TEST_COMMAND} -l profile_luajit21'
      ${LUA_TEST_FLAGS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# vim: expandtab tabstop=2 shiftwidth=2
