-- Simple human-readable renderer of LuaJIT's memprof profile.
--
-- Major portions taken verbatim or adapted from the LuaVela.
-- Copyright (C) 2015-2019 IPONWEB Ltd.

local symtab = require "utils.symtab"

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
    for _, heap_chunk in pairs(event.primary) do
      table.insert(prim_loc, symtab.demangle(symbols, heap_chunk.loc))
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

function M.all(events, symbols)
  print("ALLOCATIONS")
  M.render(events.alloc, symbols)
  print("")

  print("REALLOCATIONS")
  M.render(events.realloc, symbols)
  print("")

  print("DEALLOCATIONS")
  M.render(events.free, symbols)
  print("")
end

function M.leak_only(events, symbols)
  -- Auto resurrects source event lines for counting/reporting.
  local heap = setmetatable({}, {__index = function(t, line)
    t[line] = {
      size = 0,
      cnt_alloc = 0,
      cnt_free = 0,
    }
    return t[line]
  end})

  for _, event in pairs(events.alloc) do
    if event.loc then
      local ev_line = symtab.demangle(symbols, event.loc)

      heap[ev_line].size = heap[ev_line].size + event.alloc
      if (event.alloc > 0) then
        heap[ev_line].cnt_alloc = heap[ev_line].cnt_alloc + event.num
      end
    end
  end

  -- Realloc and free events are pretty the same.
  -- We don't interesting in aggregated alloc/free sizes for
  -- the event, but only for new and old size values inside
  -- alloc-realloc-free chain. Assuming that we have
  -- no collisions between different object addresses.
  local function process_non_alloc_events(events_by_type)
    for _, event in pairs(events_by_type) do
      -- Realloc and free events always have "primary" key
      -- that references table with rewrited memory
      -- (may be empty).
      for _, heap_chunk in pairs(event.primary) do
        local ev_line = symtab.demangle(symbols, heap_chunk.loc)

        heap[ev_line].size = heap[ev_line].size + heap_chunk.alloced
        if (heap_chunk.alloced > 0) then
          heap[ev_line].cnt_alloc = heap[ev_line].cnt_alloc + heap_chunk.cnt
        end

        heap[ev_line].size = heap[ev_line].size - heap_chunk.freed
        if (heap_chunk.freed > 0) then
          heap[ev_line].cnt_free = heap[ev_line].cnt_free + heap_chunk.cnt
        end
      end
    end
  end
  process_non_alloc_events(events.realloc)
  process_non_alloc_events(events.free)

  local rest_heap = {}
  for line, info in pairs(heap) do
    -- Report "INTERNAL" events inconsistencies for profiling
    -- with enabled jit.
    if info.size > 0 then
      table.insert(rest_heap, {line = line, hold_bytes = info.size})
    end
  end

  table.sort(rest_heap, function(h1, h2)
    return h1.hold_bytes > h2.hold_bytes
  end)

  print("HEAP SUMMARY:")
  for _, h in pairs(rest_heap) do
    print(string.format(
      "%s holds %d bytes %d allocs, %d frees",
      h.line, h.hold_bytes, heap[h.line].cnt_alloc, heap[h.line].cnt_free
    ))
  end
  print("")
end

return M
