local bit = require "bit"

local band = bit.band
local string_format = string.format

local LJS_MAGIC = "ljso"
local LJS_CURRENT_VERSION = 1
local LJS_EPILOGUE_HEADER = 0x80
local LJS_SYMTYPE_MASK = 0x03

local SO_SHARED = 0

local ffi = require "ffi"
ffi.cdef[[
void demangle(const char* path, uint64_t base);
]]

local demangle = ffi.load("demangle")

local M = {}

-- Parse a single entry in a symtab: lfunc symbol.
local function parse_sym_lfunc(reader, symtab)
  local so_addr = reader:read_uleb128()
  local so_path = reader:read_string()

  print("so: "..so_path.." "..string.format('%d', so_addr))
  demangle.demangle(so_path, so_addr)  
    
  symtab[so_addr] = {
    path = so_path,
    symbols = {}
  }
end

local parsers = {
  [SO_SHARED] = parse_sym_lfunc,
}

function M.parse(reader)
  local symtab = {}
  local magic = reader:read_octets(4)
  local version = reader:read_octets(1)

  -- Dummy-consume reserved bytes.
  local _ = reader:read_octets(3)

  if magic ~= LJS_MAGIC then
    error("Bad LJS format prologue: "..magic)
  end

  if string.byte(version) ~= LJS_CURRENT_VERSION then
    error(string_format(
         "LJS format version mismatch:"..
         "the tool expects %d, but your data is %d",
         LJS_CURRENT_VERSION,
         string.byte(version)
    ))

  end

  while not reader:eof() do
    local header = reader:read_octet()
    local is_final = band(header, LJS_EPILOGUE_HEADER) ~= 0

    if is_final then
      break
    end

    local sym_type = band(header, LJS_SYMTYPE_MASK)
    if parsers[sym_type] then
      parsers[sym_type](reader, symtab)
    end
  end

  return symtab
end

function M.demangle(symtab, loc)
  local addr = loc.addr

  if addr == 0 then
    return "INTERNAL"
  end

  if symtab[addr] then
    return string_format("%s:%d", symtab[addr].source, loc.line)
  end

  return string_format("CFUNC %#x", addr)
end

return M
