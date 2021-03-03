-- Use the default LuaJIT globals.
std = 'luajit'
-- This fork also introduces a new global for misc API namespace.
read_globals = { 'misc' }

-- These files are inherited from the vanilla LuaJIT or different
-- test suites and need to be coherent with the upstream.
exclude_files = {
  'dynasm/',
  'src/',
  'test/PUC-Lua-5.1-tests/',
  'test/LuaJIT-tests/',
  'test/lua-Harness-tests/',
}