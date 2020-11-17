--- tap.lua internal file.
---
--- The Test Anything Protocol vesion 13 producer.
---

-- Initializer FFI for <iscdata> check.
local ffi = require('ffi')
local NULL = ffi.new('void *')

local function indent(chars)
  return (' '):rep(chars)
end

local function traceback(level)
  local trace = { }
  level = level or 3
  while true do
    local info = debug.getinfo(level, 'nSl')
    if not info then break end
    table.insert(trace, {
      source   = info.source,
      src      = info.short_src,
      line     = info.linedefined or 0,
      what     = info.what,
      name     = info.name,
      namewhat = info.namewhat,
      filename = info.source:sub(1, 1) == '@' and info.source:sub(2) or 'eval',
    })
    level = level + 1
  end
  return trace
end

local function diag(test, fmt, ...)
  io.write(indent(4 * test.level), ('# %s\n'):format(fmt:format(...)))
end

local function ok(test, cond, message, extra)
  test.total = test.total + 1
  if cond then
    io.write(indent(4 * test.level), ('ok - %s\n'):format(message))
    return true
  end

  test.failed = test.failed + 1
  io.write(indent(4 * test.level), ('not ok - %s\n'):format(message))

  -- Dump extra contents added in outer space.
  for key, value in pairs(extra or { }) do
    io.write(indent(2 + 4 * test.level), ('%s:\t%s\n'):format(key, value))
  end

  if not test.trace then
    return false
  end

  local trace = traceback()
  local tindent = indent(4 + 4 * test.level)
  io.write(tindent, ('filename:\t%s\n'):format(trace[#trace].filename))
  io.write(tindent, ('line:\t%s\n'):format(trace[#trace].line))
  for frameno, frame in ipairs(trace) do
    io.write(tindent, ('frame #%d\n'):format(frameno))
    local findent = indent(2) .. tindent
    for key, value in pairs(frame) do
      io.write(findent, ('%s:\t%s\n'):format(key, value))
    end
  end
  return false
end

local function fail(test, message, extra)
  return ok(test, false, message, extra)
end

local function skip(test, message, extra)
  ok(test, true, message .. ' # skip', extra)
end

local function cmpdeeply(got, expected, extra)
  if type(expected) == 'number' or type(got) == 'number' then
    extra.got = got
    extra.expected = expected
    -- Handle NaN.
    if got ~= got and expected ~= expected then
        return true
    end
    return got == expected
  end

  if ffi.istype('bool', got) then got = (got == 1) end
  if ffi.istype('bool', expected) then expected = (expected == 1) end

  if extra.strict and type(got) ~= type(expected) then
    extra.got = type(got)
    extra.expected = type(expected)
    return false
  end

  if type(got) ~= 'table' or type(expected) ~= 'table' then
    extra.got = got
    extra.expected = expected
    return got == expected
  end

  local path = extra.path or '/'
  local visited_keys = {}

  for i, v in pairs(got) do
    visited_keys[i] = true
    extra.path = path .. '/' .. i
    if not cmpdeeply(v, expected[i], extra) then
      return false
    end
  end

  -- Check if expected contains more keys then got.
  for i, v in pairs(expected) do
    if visited_keys[i] ~= true and (extra.strict or v ~= NULL) then
      extra.expected = 'key ' .. tostring(i)
      extra.got = 'nil'
      return false
    end
  end

  extra.path = path

  return true
end

local function like(test, got, pattern, message, extra)
  extra = extra or { }
  extra.got = got
  extra.expected = pattern
  return ok(test, tostring(got):match(pattern) ~= nil, message, extra)
end

local function unlike(test, got, pattern, message, extra)
  extra = extra or { }
  extra.got = got
  extra.expected = pattern
  return ok(test, tostring(got):match(pattern) == nil, message, extra)
end

local function is(test, got, expected, message, extra)
  extra = extra or { }
  extra.got = got
  extra.expected = expected
  local rc = (test.strict == false or type(got) == type(expected))
             and got == expected
  return ok(test, rc, message, extra)
end

local function isnt(test, got, unexpected, message, extra)
  extra = extra or { }
  extra.got = got
  extra.unexpected = unexpected
  local rc = (test.strict == true and type(got) ~= type(unexpected))
             or got ~= unexpected
  return ok(test, rc, message, extra)
end

local function is_deeply(test, got, expected, message, extra)
  extra = extra or { }
  extra.got = got
  extra.expected = expected
  extra.strict = test.strict
  return ok(test, cmpdeeply(got, expected, extra), message, extra)
end

local function isnil(test, v, message, extra)
  return is(test, not v and v == nil and 'nil' or v, 'nil', message, extra)
end

local function isnumber(test, v, message, extra)
  return is(test, type(v), 'number', message, extra)
end

local function isstring(test, v, message, extra)
  return is(test, type(v), 'string', message, extra)
end

local function istable(test, v, message, extra)
  return is(test, type(v), 'table', message, extra)
end

local function isboolean(test, v, message, extra)
  return is(test, type(v), 'boolean', message, extra)
end

local function isfunction(test, v, message, extra)
  return is(test, type(v), 'function', message, extra)
end

local function isudata(test, v, utype, message, extra)
  extra = extra or { }
  extra.expected = ('userdata<%s>'):format(utype)
  if type(v) ~= 'userdata' then
    extra.got = type(v)
    return fail(test, message, extra)
  end
  extra.got = ('userdata<%s>'):format(getmetatable(v))
  return ok(test, getmetatable(v) == utype, message, extra)
end

local function iscdata(test, v, ctype, message, extra)
  extra = extra or { }
  extra.expected = ffi.typeof(ctype)
  if type(v) ~= 'cdata' then
    extra.got = type(v)
    return fail(test, message, extra)
  end
  extra.got = ffi.typeof(v)
  return ok(test, ffi.istype(ctype, v), message, extra)
end

local test_mt

local function new(parent, name, fun, ...)
  local level = parent ~= nil and parent.level + 1 or 0
  local test = setmetatable({
    parent  = parent,
    name    = name,
    level   = level,
    total   = 0,
    failed  = 0,
    planned = 0,
    trace   = parent == nil and true or parent.trace,
    strict  = false,
  }, test_mt)
  if fun == nil then
    return test
  end
  test:diag('%s', test.name)
  fun(test, ...)
  test:diag('%s: end', test.name)
  return test:check()
end

local function plan(test, planned)
  test.planned = planned
  io.write(indent(4 * test.level), ('1..%d\n'):format(planned))
end

local function check(test)
  if test.checked then
    error('check called twice')
  end
  test.checked = true
  if test.planned ~= test.total then
    if test.parent ~= nil then
      ok(test.parent, false, 'bad plan', {
        planned = test.planned,
        run = test.total,
      })
    else
      diag(test, ('bad plan: planned %d run %d')
        :format(test.planned, test.total))
    end
  elseif test.failed > 0 then
    if test.parent ~= nil then
      ok(test.parent, false, 'failed subtests', {
        failed = test.failed,
        planned = test.planned,
      })
    else
      diag(test, 'failed subtest: %d', test.failed)
    end
  else
    if test.parent ~= nil then
      ok(test.parent, true, test.name)
    end
  end
  return test.planned == test.total and test.failed == 0
end

test_mt = {
  __index = {
    test       = new,
    plan       = plan,
    check      = check,
    diag       = diag,
    ok         = ok,
    fail       = fail,
    skip       = skip,
    is         = is,
    isnt       = isnt,
    isnil      = isnil,
    isnumber   = isnumber,
    isstring   = isstring,
    istable    = istable,
    isboolean  = isboolean,
    isfunction = isfunction,
    isudata    = isudata,
    iscdata    = iscdata,
    is_deeply  = is_deeply,
    like       = like,
    unlike     = unlike,
  }
}

return {
  test = function(...)
    io.write('TAP version 13\n')
    return new(nil, ...)
  end
}
