-- Parser of LuaJIT's sysprof binary stream.
-- The format spec can be found in <src/lj_sysprof.h>.

local string_format = string.format

local LJP_MAGIC = "ljp"
local LJP_CURRENT_VERSION = 1

local CALLCHAIN = {
  HOST = 0,
  LUA = 1,
  TRACE = 2
}

VMST = {
  INTERP = 0,
  LFUNC  = 1,
  FFUNC  = 2,
  CFUNC  = 3,
  GC     = 4,
  EXIT   = 5,
  RECORD = 6,
  OPT    = 7,
  ASM    = 8,
  TRACE  = 9,
}

local STREAM_END = 0xBB

FRAME = {
  LFUNC  = 0,
  CFUNC  = 1,
  FFUNC  = 2,
  BOTTOM = 0x80
}

local M = {}

local function new_event()
  return {
    lua = {
      vmstate = 0,
      callchain = {},
      trace = {
        id = nil,
        addr = 0,
        line = 0
      }
    },
    host = {
      callchain = {}
    }
  }
end

local function parse_lfunc(reader, event)
  local addr = reader:read_uleb128()
  local line = reader:read_uleb128()
  table.insert(event.lua.callchain, {
    type = FRAME.LFUNC,
    addr = addr,
    line = line
  })
end

local function parse_ffunc(reader, event)
  local ffid = reader:read_uleb128()
  table.insert(event.lua.callchain, {
    type = FRAME.FFUNC,
    ffid = ffid,
  })
end

local function parse_cfunc(reader, event)
  local addr = reader:read_uleb128()
  table.insert(event.lua.callchain, {
    type = FRAME.CFUNC,
    addr = addr
  })
end

local frame_parsers = {
  [FRAME.LFUNC] = parse_lfunc,
  [FRAME.FFUNC] = parse_ffunc,
  [FRAME.CFUNC] = parse_cfunc
}

local function parse_lua_callchain(reader, event)
  local header = reader:read_octet()
  assert(header == CALLCHAIN.LUA, "expected lua callchain header, got "..header)

  while true do
    local frame_header = reader:read_octet()
    if frame_header == FRAME.BOTTOM then
      break
    end
    frame_parsers[frame_header](reader, event)
  end
end

--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~--

local function parse_host_callchain(reader, event)
  local header = reader:read_octet()
  assert(header == CALLCHAIN.HOST, "expected host callchain header, got "..header)

  local callchain_size = reader:read_uleb128()

  for i=1,callchain_size do
    local addr = reader:read_uleb128()
    event.host.callchain[i] = {
      addr = addr
    }
  end
end

--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~--

local function parse_trace_callchain(reader, event)
  local header = reader:read_octet()
  assert(header == CALLCHAIN.TRACE, "expected trace callchain header, got "..header)

  event.lua.trace.id   = reader:read_uleb128()
  event.lua.trace.addr = reader:read_uleb128()
  event.lua.trace.line = reader:read_uleb128()
end

--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~--

local function parse_host_only(reader, event)
  parse_host_callchain(reader, event)
end

local function parse_lua_host(reader, event)
  parse_lua_callchain(reader, event)
  parse_host_callchain(reader, event)
end

local function parse_trace(reader, event)
  parse_trace_callchain(reader, event)
  -- parse_lua_callchain(reader, event)
end

local event_parsers = {
  [VMST.INTERP] = parse_host_only,
  [VMST.LFUNC]  = parse_lua_host,
  [VMST.FFUNC]  = parse_lua_host,
  [VMST.CFUNC]  = parse_lua_host,
  [VMST.GC]     = parse_host_only,
  [VMST.EXIT]   = parse_host_only,
  [VMST.RECORD] = parse_host_only,
  [VMST.OPT]    = parse_host_only,
  [VMST.ASM]    = parse_host_only,
  [VMST.TRACE]  = parse_trace
}

local function parse_event(reader, events)
  local event = new_event()

  local vmstate = reader:read_octet()
  if vmstate == STREAM_END then
    -- TODO: samples & overruns
    return false
  end

  assert(0 <= vmstate and vmstate <= 9, "Vmstate "..vmstate.." is not valid")
  event.lua.vmstate = vmstate

  event_parsers[vmstate](reader, event)

  table.insert(events, event)
  return true
end

function M.parse(reader)
  local events = {}

  local magic = reader:read_octets(3)
  local version = reader:read_octets(1)
  -- Dummy-consume reserved bytes.
  local _ = reader:read_octets(3)

  if magic ~= LJP_MAGIC then
    error("Bad LJP format prologue: "..magic)
  end

  if string.byte(version) ~= LJP_CURRENT_VERSION then
    error(string_format(
      "LJP format version mismatch: the tool expects %d, but your data is %d",
      LJP_CURRENT_VERSION,
      string.byte(version)
    ))
  end

  while parse_event(reader, events) do
    -- Empty body.
  end

  return events
end

return M
