# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_program(PROVE prove)
if(NOT PROVE)
  message(WARNING "`prove' is not found, so tarantool-tests target is not generated")
  return()
endif()

add_custom_command(
  COMMENT "Running lua-Harness tests"
  OUTPUT tests.ok
  DEPENDS ${LUAJIT_TEST_BINARY}
  COMMAND
    ${PROVE} ${CMAKE_CURRENT_SOURCE_DIR}
      --exec ${LUAJIT_TEST_BINARY}
      --failures --shuffle
    && touch tests.ok
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(lua-Harness DEPENDS tests.ok)
