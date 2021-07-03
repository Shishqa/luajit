local ffi = require "ffi"
local math = require "math"
local bufread = require "utils.bufread"

-- All the definitions are taken from elf.h
ffi.cdef[[
/* Type for a 16-bit quantity.  */
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf64_Word;
typedef	int32_t  Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf64_Xword;
typedef	int64_t  Elf64_Sxword;

/* Type of addresses.  */
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint64_t Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf64_Section;

/* Type for version symbol information.  */
typedef Elf64_Half Elf64_Versym;

typedef struct
{
  unsigned char	e_ident[16]; /* Magic number and other info */
  Elf64_Half	  e_type;			 /* Object file type */
  Elf64_Half	  e_machine;	 /* Architecture */
  Elf64_Word	  e_version;	 /* Object file version */
  Elf64_Addr	  e_entry;		 /* Entry point virtual address */
  Elf64_Off	    e_phoff;		 /* Program header table file offset */
  Elf64_Off	    e_shoff;		 /* Section header table file offset */
  Elf64_Word	  e_flags;		 /* Processor-specific flags */
  Elf64_Half	  e_ehsize;		 /* ELF header size in bytes */
  Elf64_Half	  e_phentsize; /* Program header table entry size */
  Elf64_Half	  e_phnum;		 /* Program header table entry count */
  Elf64_Half	  e_shentsize; /* Section header table entry size */
  Elf64_Half	  e_shnum;		 /* Section header table entry count */
  Elf64_Half	  e_shstrndx;	 /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
  Elf64_Word	sh_name;		  /* Section name (string tbl index) */
  Elf64_Word	sh_type;		  /* Section type */
  Elf64_Xword	sh_flags;		  /* Section flags */
  Elf64_Addr	sh_addr;		  /* Section virtual addr at execution */
  Elf64_Off	  sh_offset;		/* Section file offset */
  Elf64_Xword	sh_size;		  /* Section size in bytes */
  Elf64_Word	sh_link;		  /* Link to another section */
  Elf64_Word	sh_info;		  /* Additional section information */
  Elf64_Xword	sh_addralign;	/* Section alignment */
  Elf64_Xword	sh_entsize;		/* Entry size if section holds table */
} Elf64_Shdr;

typedef struct
{
  Elf64_Word	  st_name;		/* Symbol name (string tbl index) */
  unsigned char	st_info;		/* Symbol type and binding */
  unsigned char st_other;		/* Symbol visibility */
  Elf64_Section	st_shndx;		/* Section index */
  Elf64_Addr	  st_value;		/* Symbol value */
  Elf64_Xword	  st_size;		/* Symbol size */
} Elf64_Sym;
]]

-- Fields in e_ident array

EI_MAG0 = 0
ELFMAG0 = 0x7f

EI_MAG1 = 1
ELFMAG1 = string.byte("E")

EI_MAG2 = 2
ELFMAG2 = string.byte("L")

EI_MAG3 = 3
ELFMAG3 = string.byte("F")

EI_CLASS = 4
ELFCLASS64 = 2 -- 64 bit objects

EI_DATA = 5
ELFDATA2LSB = 1 -- little endian

-- Actual values for e_type

ET_EXEC = 2
ET_DYN = 3

-- Constants for Elf64_Shdr

SHN_LORESERVE = 0xff00

SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_DYNSYM = 11

STT_FUNC = 2

local M = {}


local function check_file(buf)
  local hdr = ffi.cast("Elf64_Ehdr*", buf)

  if ELFMAG0 ~= hdr.e_ident[EI_MAG0] or   -- 0x7f
     ELFMAG1 ~= hdr.e_ident[EI_MAG1] or   -- 'E'
     ELFMAG2 ~= hdr.e_ident[EI_MAG2] or   -- 'L'
     ELFMAG3 ~= hdr.e_ident[EI_MAG3] then -- 'F'
    error("Wrong magic number")
  end

  if ET_DYN ~= hdr.e_type and ET_EXEC ~= hdr.e_type then
    error("Isn't a shared or executable object")
  end

  if ELFCLASS64 ~= hdr.e_ident[EI_CLASS] then
    error("Not a 64-bit object")
  end

  if ELFDATA2LSB ~= hdr.e_ident[EI_DATA] then
    error("Wrong endianness")
  end
end


local function read_section_field(buf, ehdr, idx)
  return ffi.cast("Elf64_Shdr*", buf + ehdr.e_shoff + idx * ehdr.e_shentsize)
end


local function get_section_name(buf, ehdr, sec)
  local shstr = read_section_field(buf, ehdr, ehdr.e_shstrndx)
  return ffi.string(buf + shstr.sh_offset + sec.sh_name)
end


local function get_strsect_offset(buf, lookup_name)
  local ehdr = ffi.cast("Elf64_Ehdr*", buf)

  for i=1,math.min(ehdr.e_shnum, SHN_LORESERVE) do
    local shdr = read_section_field(buf, ehdr, i)

    local s_name = get_section_name(buf, ehdr, shdr)
    if s_name == lookup_name then
      if SHT_STRTAB == shdr.sh_type then
        assert((buf + shdr.sh_offset)[0] == 0) -- TODO: find out, why
        return shdr.sh_offset
      end
    end
  end

  return nil
end


local function parse_symtab(buf, symbols, shdr, str_idx)
  local symtab = ffi.cast("uint8_t*", buf + shdr.sh_offset)

  -- XXX: how can we implement sizeof(*sym) paradigm?
  local num_of_entries = shdr.sh_size / ffi.sizeof("Elf64_Sym")
  assert(shdr.sh_size % ffi.sizeof("Elf64_Sym") == 0,
         'number of entries must be integer')

  for i=0,tonumber(num_of_entries) do
    local sym = ffi.cast("Elf64_Sym*", symtab + i * ffi.sizeof("Elf64_Sym"))

    if STT_FUNC == (sym.st_info % 0x10) and 0 ~= sym.st_size then
      local st_name = buf + str_idx + sym.st_name

      -- TODO: demangle
      local sym_name = ffi.string(buf + str_idx + sym.st_name)

      table.insert(symbols, {
        addr = sym.st_value,
        name = sym_name,
        size = sym.st_size
      })
    end
  end
end


local function iterate_sections(buf, symbols, sec_type, str_idx)
  if str_idx == nil then return end

  local ehdr = ffi.cast("Elf64_Ehdr*", buf)

  for i=1,math.min(ehdr.e_shnum, SHN_LORESERVE) do
    local shdr = read_section_field(buf, ehdr, i)
    if sec_type == shdr.sh_type then
      parse_symtab(buf, symbols, shdr, str_idx)
    end
  end
end


function M.parse_mem(buf, symbols)
  local strtab_idx = get_strsect_offset(buf, ".strtab")
  local dynstr_idx = get_strsect_offset(buf, ".dynstr")

  iterate_sections(buf, symbols, SHT_SYMTAB, strtab_idx)
  iterate_sections(buf, symbols, SHT_DYNSYM, dynstr_idx)
end


function M.parse_file(path, symbols)
  local reader = bufread.new(path)
  local buf_data = ffi.cast("uint8_t*", reader._buf)
  check_file(buf_data)
  M.parse_mem(buf_data, symbols)
end

return M
