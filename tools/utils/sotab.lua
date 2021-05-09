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
struct sym_info {
	uint64_t addr; /* Raw offset to symbol */
	uint64_t size; /* Physical size in the SO*/
	const char *name; /* Symbol */
};

struct vector {
	struct sym_info **elems;
	uint32_t size;
	uint32_t capacity;
};

struct shared_obj {
	const char *path; /* Full path to object */
	const char *short_name; /* Basename */
	uint64_t base; /* VA */
	uint64_t size; /* Size from stat() function */
	struct vector symbols; /* Symbols from nm */
	uint8_t found;
};

void ujpp_demangle_free_symtab(struct shared_obj *so);
struct shared_obj* load_so(const char* path, uint64_t base);
void free(void* ptr);
]]

local demangle = ffi.load("demangle")

local M = {}

-- Parse a single entry in a symtab: lfunc symbol.
local function parse_sym_lfunc(reader, symtab)
  local so_addr = reader:read_uleb128()
  local so_path = reader:read_string()

  -- print("so: "..so_path.." "..string.format('%d', so_addr))
  local so = demangle.load_so(so_path, so_addr)
  -- print(so[0].symbols.size)

  for i=0,so[0].symbols.size-1 do
    local addr = tonumber(so[0].symbols.elems[i][0].addr + so[0].base)
    local name = ffi.string(so[0].symbols.elems[i][0].name)
    local size = tonumber(so[0].symbols.elems[i][0].size)
    symtab[addr] = { name = name, size = size }
  end

  demangle.ujpp_demangle_free_symtab(so)
  ffi.C.free(so)
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

function M.demangle(symtab, addr)
  -- TODO: binary search + cache
  for k, v in pairs(symtab) do
    if k <= addr and addr <= k + v.size then
      return v.name
    end
  end
  return string_format("[%#x]", addr)
end

return M
