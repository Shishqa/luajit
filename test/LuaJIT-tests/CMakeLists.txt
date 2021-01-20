# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_custom_target(LuaJIT-tests DEPENDS ${LUAJIT_TEST_BINARY})

add_custom_command(TARGET LuaJIT-tests
  COMMENT "Running LuaJIT-tests"
  COMMAND
    # Tarantool doesn't support LUA_INIT and most likely it
    # never will.
    # See https://github.com/tarantool/tarantool/issues/5744
    # for more info.
    # So use less preferable way for tests.
    # See the root CMakeLists.txt for more info.
    ${LUAJIT_TEST_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/test.lua
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# vim: expandtab tabstop=2 shiftwidth=2
