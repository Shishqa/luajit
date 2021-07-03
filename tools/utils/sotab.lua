local bit = require "bit"
local math = require "math"

local ffi = require "ffi"
local elf = require "utils.elf"

ffi.cdef[[
unsigned long getauxval(unsigned long type);
]]

AT_SYSINFO_EHDR = 33

local SO_TYPE = {
  SO_BIN = 1,
  SO_VDSO = 2,
  SO_SHARED = 3
}

local band = bit.band
local string_format = string.format

local LJS_MAGIC = "ljso"
local LJS_CURRENT_VERSION = 1
local LJS_EPILOGUE_HEADER = 0x80
local LJS_SYMTYPE_MASK = 0x03

local SO_SHARED = 0

local M = {}

local function determine_obj_type(path)
  if path:match("vdso") then
    return SO_TYPE.SO_VDSO
  elseif path:match(".so") then
    return SO_TYPE.SO_SHARED
  else
    return SO_TYPE.SO_BIN
  end
end

-- Parse a single entry in a symtab: lfunc symbol.
local function parse_shared_obj(reader, symtab)
  local so_addr = reader:read_uleb128()
  local so_path = reader:read_string()

  local type = determine_obj_type(so_path)
  local so = {
    path = so_path,
    base = so_addr,
    symbols = {}
  }

  if type ~= SO_TYPE.SO_VDSO then
    elf.parse_file(so_path, so.symbols)
  else
    local vdso = ffi.cast("uint8_t*", ffi.C.getauxval(AT_SYSINFO_EHDR))
    elf.parse_mem(vdso, so.symbols)
  end

  for k,v in pairs(so.symbols) do
    table.insert(symtab, {
      name = v.name,
      addr = v.addr + so.base,
      size = v.size
    })
  end
end

local parsers = {
  [SO_SHARED] = parse_shared_obj,
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

  table.sort(symtab, function(a, b) return a.addr < b.addr end)

  return symtab
end

local function is_inside(frame, addr)
  return frame.addr <= addr and addr <= frame.addr + frame.size
end

function M.demangle_slow(symtab, addr)
  for _, frame in pairs(symtab) do
    if is_inside(frame, addr) then
      return frame.name
    end
  end
  return string_format("[%#x]", addr)
end


local function demangle_fast(symtab, addr, l, r)
  if l + 1 >= r then
    if is_inside(symtab[l], addr) then
      return symtab[l].name
    end
    return string_format("[%#x]", addr)
  end
  local m = math.floor((l + r) / 2)
  if addr < symtab[m].addr then
    return demangle_fast(symtab, addr, l, m)
  end
  return demangle_fast(symtab, addr, m, r)
end

function M.demangle(symtab, addr)
  return demangle_fast(symtab, addr, 1, #symtab + 1)
end

return M
