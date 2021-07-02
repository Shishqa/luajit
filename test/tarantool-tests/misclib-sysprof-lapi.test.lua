local tap = require("tap")

local test = tap.test("misc-sysprof-lapi")
test:plan(8)

jit.off()
jit.flush()

local table_new = require "table.new"

local bufread = require "utils.bufread"
local sysprof = require "sysprof.parse"
local symtab = require "utils.symtab"
local sotab = require "utils.sotab"

local TMP_BINFILE = arg[0]:gsub(".+/([^/]+)%.test%.lua$", "%.%1.sysprofdata.tmp.bin")
local BAD_PATH = arg[0]:gsub(".+/([^/]+)%.test%.lua$", "%1/sysprofdata.tmp.bin")


local function payload()
  local function fib(n)
    if n <= 1 then
      return n
    end
    return fib(n - 1) + fib(n - 2)
  end
  return fib(15)
end

local function generate_output(filename)
  local res, err = misc.sysprof.start(filename)
  assert(res, err)

  payload()

  res,err = misc.sysprof.stop()
  assert(res, err)
end

--## GENERAL ###############################################################--

-- already running
res, err, errno = misc.sysprof.start("", "D")
assert(res, err)

res, err, errno = misc.sysprof.start("", "D")
test:ok(res == nil and err:match("profiler is running already"))
test:ok(type(errno) == "number")

res, err = misc.sysprof.stop()
assert(res, err)

-- not running
res, err, errno = misc.sysprof.stop()
test:ok(res == nil and err:match("profiler is not running"))
test:ok(type(errno) == "number")

-- bad path
res, err, errno = misc.sysprof.start(BAD_PATH, "C")
test:ok(res == nil and err:match("No such file or directory"))
test:ok(type(errno) == "number")

--## DEFAULT MODE ##########################################################--

local res, err, errno = misc.sysprof.start("", "D")
test:ok(res == true and err == nil and errno == nil)

res, err, errno = misc.sysprof.stop()
test:ok(res == true and err == nil and errno == nil)

-- TODO: dump data to table

--## CALLCHAIN MODE ########################################################--

res, err = pcall(generate_output, TMP_BINFILE)

if res == nil then
  os.remove(TMP_BINFILE)
  error(err)
end

local reader = bufread.new(TMP_BINFILE)

local lua_symtab = symtab.parse(reader)
local host_symtab = sotab.parse(reader)
local events = sysprof.parse(reader)

jit.on()
os.exit(test:check() and 0 or 1)
