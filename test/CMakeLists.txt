# Running various test suites against LuaJIT.

# See the rationale in the root CMakeLists.txt
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_subdirectory(tarantool-tests)
add_subdirectory(LuaJIT-test-cleanup)
add_subdirectory(lua-Harness)

add_custom_target(${PROJECT_NAME}-test DEPENDS
  tarantool-tests LuaJIT-test-cleanup lua-Harness
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

  add_custom_target(test DEPENDS ${PROJECT_NAME}-test)
endif()
