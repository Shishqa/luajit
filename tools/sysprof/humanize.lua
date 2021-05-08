-- Simple human-readable renderer of LuaJIT's memprof profile.
--
-- Major portions taken verbatim or adapted from the LuaVela.
-- Copyright (C) 2015-2019 IPONWEB Ltd.

local symtab = require "utils.symtab"
local sotab = require "utils.sotab"

local M = {}

function M.render(events, symbols)
  local ids = {}

  for id, _ in pairs(events) do
    table.insert(ids, id)
  end

  table.sort(ids, function(id1, id2)
    return events[id1].num > events[id2].num
  end)

  for i = 1, #ids do
    local event = events[ids[i]]
    print(string.format("%s: %d events\t+%d bytes\t-%d bytes",
      symtab.demangle(symbols, event.loc),
      event.num,
      event.alloc,
      event.free
    ))

    local prim_loc = {}
    for _, loc in pairs(event.primary) do
      table.insert(prim_loc, symtab.demangle(symbols, loc))
    end
    if #prim_loc ~= 0 then
      table.sort(prim_loc)
      print("\tOverrides:")
      for j = 1, #prim_loc do
        print(string.format("\t\t%s", prim_loc[j]))
      end
      print("")
    end
  end
end

function M.dump_flamegraph(outfile, events, lua_symbols, host_symbols)


end

return M
