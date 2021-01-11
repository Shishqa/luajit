# Test suite that has been moved from Tarantool repository in
# scope of https://github.com/tarantool/tarantool/issues/4478.

# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_program(PROVE prove)
if(NOT PROVE)
  message(WARNING "`prove' is not found, so tarantool-tests target is not generated")
  return()
endif()

macro(BuildTestLib lib sources)
  add_library(${lib} SHARED EXCLUDE_FROM_ALL ${sources})
  target_include_directories(${lib} PRIVATE
    ${LUAJIT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  set_target_properties(${lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    PREFIX ""
  )

  # XXX: This change affects current cmake variable scope and so
  # a user should care to don't use it in a top level scope.
  # The dynamic libraries are loaded with LuaJIT binary and use
  # symbols from it. So it is totally OK to have unresolved
  # symbols at build time.
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(${lib} PROPERTIES
      LINK_FLAGS "-undefined dynamic_lookup"
    )
  else()
    # FIXME: Unfortunately there is no another way to suppress
    # this linker option, so just strip it out from the flags.
    string(REPLACE "-Wl,--no-undefined" ""
      CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}"
    )
  endif()
  # XXX: Append the lib to be built to the dependecy list.
  # Unfortunately, CMake is a crap and there is no other way to
  # extend the list in parent scope but join two strings with
  # semicolon. If one finds the normal way to make it work, feel
  # free to reach me.
  set(TESTLIBS "${lib};${TESTLIBS}" PARENT_SCOPE)
  # Add the directory where the lib is built to the LUA_CPATH
  # environment variable, so interpreter can find and load it.
  # XXX: Here we see the other side of the coin. If one joins two
  # strings with semicolon, the value automatically becomes the
  # list. I have no idea what is wrong with this tool, but I found
  # a single working solution to make LUA_CPATH be a string via
  # "escaping" the semicolon right in string interpolation.
  set(LUA_CPATH "${CMAKE_CURRENT_BINARY_DIR}/?${CMAKE_SHARED_LIBRARY_SUFFIX}\;${LUA_CPATH}" PARENT_SCOPE)
  # Also add this directory to LD_LIBRARY_PATH environment
  # variable, so FFI machinery can find and load it.
  set(LD_LIBRARY_PATH "${CMAKE_CURRENT_BINARY_DIR}:${LD_LIBRARY_PATH}" PARENT_SCOPE)
endmacro()

add_subdirectory(gh-4427-ffi-sandwich)
add_subdirectory(lj-flush-on-trace)
add_subdirectory(misclib-getmetrics-capi)

# The part of memory profiler toolchain is located in tools directory, so
# LUA_PATH need to be updated.
set(LUA_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/?.lua\;${PROJECT_SOURCE_DIR}/tools/?.lua"
)

# LUA_CPATH and LD_LIBRARY_PATH variables and also TESTLIBS list
# with dependecies are set in scope of BuildTestLib macro.
add_custom_command(
  COMMENT "Running Tarantool tests"
  OUTPUT tests.ok
  DEPENDS ${LUAJIT_TEST_BINARY} ${TESTLIBS}
  COMMAND
  env
    LUA_PATH="${LUA_PATH}\;\;"
    LUA_CPATH="${LUA_CPATH}\;\;"
    LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"
    ${PROVE} ${CMAKE_CURRENT_SOURCE_DIR}
      --exec ${LUAJIT_TEST_BINARY}
      --ext .test.lua
      --failures --shuffle
    && touch tests.ok
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(tarantool-tests DEPENDS tests.ok)
