add_custom_command(
  COMMENT "Running LuaJIT cleanup tests"
  OUTPUT tests.ok
  DEPENDS ${LUAJIT_TEST_BINARY}
  COMMAND
    ${LUAJIT_TEST_BINARY} ${CMAKE_CURRENT_SOURCE_DIR}/test.lua
    && touch tests.ok
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(LuaJIT-test-cleanup DEPENDS tests.ok)
