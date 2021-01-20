# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_custom_command(
  COMMENT "Running LuaJIT-test-cleanup"
  OUTPUT tests.ok
  DEPENDS ${LUAJIT_TEST_BINARY}
  COMMAND
    ${LUAJIT_TEST_BINARY} ${CMAKE_CURRENT_SOURCE_DIR}/test.lua
    && touch tests.ok
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(LuaJIT-test-cleanup DEPENDS tests.ok)
