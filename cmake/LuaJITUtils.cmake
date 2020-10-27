function(LuaJITTestArch outvar strflags)
  # XXX: <execute_process> simply splits the COMMAND argument by
  # spaces with no further parsing. At the same time GCC is bad in
  # argument handling, so let's help it a bit.
  separate_arguments(TEST_C_FLAGS UNIX_COMMAND ${strflags})
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${TEST_C_FLAGS} -E lj_arch.h -dM
    WORKING_DIRECTORY ${LUAJIT_SOURCE_DIR}
    OUTPUT_VARIABLE TESTARCH
  )
  set(${outvar} ${TESTARCH} PARENT_SCOPE)
endfunction()

function(LuaJITArch outvar testarch)
  foreach(TRYARCH X64 X86 ARM ARM64 PPC MIPS64 MIPS)
    string(FIND "${testarch}" "LJ_TARGET_${TRYARCH}" FOUND)
    if(FOUND EQUAL -1)
      continue()
    endif()
    string(TOLOWER ${TRYARCH} LUAJIT_ARCH)
    set(${outvar} ${LUAJIT_ARCH} PARENT_SCOPE)
    return()
  endforeach()
  message(FATAL_ERROR "[LuaJITArch] Unsupported target architecture")
endfunction()

macro(AppendFlags flags)
  foreach(flag ${ARGN})
    set(${flags} "${${flags}} ${flag}")
  endforeach()
endmacro()
