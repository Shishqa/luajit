local io = require "io"

local sysprof = require "sysprof.parse"
local symtab = require "utils.symtab"
local sotab = require "utils.sotab"

local M = {}

function M.collapse(events, host_symtab, lua_symtab)
  local cc = {}

  for id,ev in pairs(events) do
    print("EV #"..id)

    --[[
    for i=#ev.lua.callchain,1,-1 do
      io.write(symtab.demangle(lua_symtab, {
        addr = ev.lua.callchain[i].addr,
        line = ev.lua.callchain[i].line
      })..';')
    end
    io.write('\n')
    --]]

    local chain = ''

    -- host
    for _,h in pairs(ev.host.callchain) do
      local name = sotab.demangle(host_symtab, h.addr)
      --io.write(name..'$$$')
      chain = chain..name..';'
      if name == 'lua_pcall' then
        for i=#ev.lua.callchain,1,-1 do
          chain = chain..symtab.demangle(lua_symtab, {
            addr = ev.lua.callchain[i].addr,
            line = ev.lua.callchain[i].line
          })..';'
        end
      end
    end
    --io.write('\n')

    if cc[chain] == nil then
      cc[chain] = 0
    end
    cc[chain] = cc[chain] + 1
  end

  for k,v in pairs(cc) do
    print(k..' '..v)
  end
end

return M
