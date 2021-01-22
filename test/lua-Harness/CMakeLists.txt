# Test suite that has been moved from Tarantool repository in
# scope of https://github.com/tarantool/tarantool/issues/4478.

# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_program(PROVE prove)
if(NOT PROVE)
  message(WARNING "`prove' is not found, so lua-Harness target is not generated")
  return()
endif()

add_custom_command(
  COMMENT "Running lua-Harness tests"
  OUTPUT tests.ok
  DEPENDS ${LUAJIT_TEST_BINARY}
  COMMAND
  env
    LUA_PATH="./?.lua\;${LUAJIT_SOURCE_DIR}/?.lua\;\;"
    LUA_CPATH="./?${CMAKE_SHARED_LIBRARY_SUFFIX}\;${LUAJIT_SOURCE_DIR}/?${CMAKE_SHARED_LIBRARY_SUFFIX}\;\;"
    LUA_INIT='package.loaded.tap=nil if package.loaded.strict ~= nil then package.loaded.strict.off() end'
    ${PROVE} - < tests_list
      --exec "${LUAJIT_TEST_BINARY} -l profile_luajit21"
      --failures --shuffle
    && touch tests.ok
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(lua-Harness DEPENDS tests.ok)
