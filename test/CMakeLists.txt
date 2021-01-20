# Running various test suites against LuaJIT.

# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_program(LUACHECK luacheck)
if(LUACHECK)
  set(LUACHECK_RC ${PROJECT_SOURCE_DIR}/.luacheckrc)
  set(LUACHECK_OK ${CMAKE_CURRENT_BINARY_DIR}/luacheck.ok)
  file(GLOB_RECURSE LUACHECK_DEPS ${PROJECT_SOURCE_DIR}/*.lua)
  add_custom_command(
    COMMENT "Running luacheck static analysis"
    OUTPUT ${LUACHECK_OK}
    DEPENDS ${LUACHECK} ${LUACHECK_RC} ${LUACHECK_DEPS}
    COMMAND
      ${LUACHECK} ${PROJECT_SOURCE_DIR}
        --codes
        --config ${LUACHECK_RC}
      && touch ${LUACHECK_OK}
      # XXX: Filenames in .luacheckrc are considered relative to
      # the working directory, hence luacheck should be run in the
      # project root directory.
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
else()
  add_custom_command(
    COMMENT "`luacheck' is not found, so ${PROJECT_NAME}-luacheck target is dummy"
    OUTPUT luacheck.ok
    COMMAND touch luacheck.ok
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()

add_custom_target(${PROJECT_NAME}-luacheck DEPENDS luacheck.ok)

add_subdirectory(tarantool-tests)
add_subdirectory(LuaJIT-test-cleanup)

add_custom_target(${PROJECT_NAME}-test DEPENDS
  tarantool-tests
  LuaJIT-test-cleanup
)

if(LUAJIT_USE_TEST)
  if(POLICY CMP0037)
    if(CMAKE_VERSION VERSION_LESS 3.11)
      # cmake below 3.11 reserves name test. Use old policy.
      # https://cmake.org/cmake/help/v3.11/release/3.11.html#other-changes
      cmake_policy(SET CMP0037 OLD)
    else()
      # Starting from cmake 3.11 name test reserved in special
      # cases and can be used as target name.
      cmake_policy(SET CMP0037 NEW)
    endif()
  endif(POLICY CMP0037)

  add_custom_target(test DEPENDS
    ${PROJECT_NAME}-test
    ${PROJECT_NAME}-luacheck
  )
endif()
