-- Parser of LuaJIT's memprof binary stream.
-- The format spec can be found in <src/lj_memprof.h>.
--
-- Major portions taken verbatim or adapted from the LuaVela.
-- Copyright (C) 2015-2019 IPONWEB Ltd.

local bit = require "bit"
local band = bit.band
local lshift = bit.lshift

local io = require('io')

local string_format = string.format

local LJP_MAGIC = "ljp"
local LJP_CURRENT_VERSION = 1

local LJP_EPILOGUE_HEADER = 0x80

local CALLCHAIN_NATIVE = 0 
local CALLCHAIN_LUA = 1
local CALLCHAIN_TRACE = 2 

local BOT_FRAME = 0
local LFUNC = 1
local CFUNC = 2
local FFUNC = 3

local M = {}

local function parse_native_callchain(reader)
  local callchain_header = reader:read_octet() 
  local callchain_size = reader:read_uleb128()
  print('native '..callchain_header..' '..callchain_size)

  while callchain_size ~= 0 do
    local addr = reader:read_uleb128()
    -- io.write(string.format('%d', addr))
    callchain_size = callchain_size - 1
    if callchain_size ~= 0 then
      -- io.write(";")
    end
  end
  -- io.write("\n")
end

local function parse_lua_callchain(reader)
  local callchain_header = reader:read_octet()
  print('lua '..callchain_header)
  while true do
    local frame_header = reader:read_octet()

    if frame_header == 0 then
      local addr = reader:read_uleb128()
      print('lua '..string.format('%d', addr))
    elseif frame_header == 1 then
      local addr = reader:read_uleb128()
      print('cfunc '..string.format('%d', addr))
    elseif frame_header == 2 then
      local addr = reader:read_uleb128()
      print('ffunc '..string.format('%d', addr))
    else
      break
    end

  end

end

local function parse_event(reader, state)
  if reader:eof() then
    return false
  end

  local vmstate = reader:read_octet()
  if state.counters[vmstate] == nil then
    state.counters[vmstate] = 0
  end
  state.counters[vmstate] = state.counters[vmstate] + 1

  --assert(vmstate <= 10)
  print(vmstate)

  if vmstate >= 1 and vmstate <= 3 then
    parse_lua_callchain(reader)
    parse_native_callchain(reader)
  elseif vmstate == 9 then
    parse_native_callchain(reader)
  end

  return true
end

function M.parse(reader)
  local state = {
    events = {},
    counters = {}
  }

  local magic = reader:read_octets(3)
  local version = reader:read_octets(1)
  -- Dummy-consume reserved bytes.
  local _ = reader:read_octets(3)

  if magic ~= LJP_MAGIC then
    error("Bad LJM format prologue: "..magic)
  end

  if string.byte(version) ~= LJP_CURRENT_VERSION then
    error(string_format(
      "LJM format version mismatch: the tool expects %d, but your data is %d",
      LJP_CURRENT_VERSION,
      string.byte(version)
    ))
  end

  while parse_event(reader, state) do
    -- Empty body.
  end

  print('states:')
  for k, v in pairs(state.counters) do
    print(k..' '..v)
  end

  return state
end

return M
