local parse = require "sysprof.sysprof_parse"
local vmdef = require "jit.vmdef"
local symtab = require "utils.symtab"
local sotab = require "utils.sotab"

local VMST_NAMES = {
  [VMST.INTERP] = "VMST_INTERP",
  [VMST.LFUNC]  = "VMST_LFUNC",
  [VMST.FFUNC]  = "VMST_FFUNC",
  [VMST.CFUNC]  = "VMST_CFUNC",
  [VMST.GC]     = "VMST_GC",
  [VMST.EXIT]   = "VMST_EXIT",
  [VMST.RECORD] = "VMST_RECORD",
  [VMST.OPT]    = "VMST_OPT",
  [VMST.ASM]    = "VMST_ASM",
  [VMST.TRACE]  = "VMST_TRACE",
}

local M = {}

local function new_node(name, is_leaf)
  return {
    name = name,
    count = 0,
    is_leaf = is_leaf,
    children = {}
  }
end

-- insert new child into a node (or increase counter in existing one)
local function insert(name, node, is_leaf)
  if node.children[name] == nil then
    node.children[name] = new_node(name, is_leaf)
  end

  local child = node.children[name]
  child.count = child.count + 1

  return child
end

local function insert_lua_callchain(chain, lua, lua_symtab)
  local trace_inserted = false	
  for _,fr in pairs(lua.callchain) do
    local name_lua = nil

    if fr.type == FRAME.FFUNC then
      name_lua = vmdef.ffnames[fr.ffid]
    else
      name_lua = symtab.demangle(lua_symtab, {
        addr = fr.addr,
        line = fr.line
      })
      if lua.trace.id ~= nil and lua.trace.addr == fr.addr then
        name_lua = 'TRACE_'..tostring(lua.trace.id)..'_'..name_lua
	trace_inserted = true
      end
    end

    table.insert(chain, { name = name_lua })
  end

  if lua.trace.id ~= nil and not trace_inserted then
    table.insert(chain, { name = 'TRACE_'..tostring(lua.trace.id) })
  end
end

-- merge lua and host callchains into one callchain representing
-- transfer of control
local function merge(event, lua_symtab, host_symtab, sep_vmst)
  local cc = {}
  local lua_inserted = false

  for _,h_fr in pairs(event.host.callchain) do
    local name_host = sotab.demangle(host_symtab, h_fr.addr)

    -- We assume that usually the transfer of control
    -- looks like:
    --    HOST -> LUA -> HOST
    -- so for now, lua callchain starts from lua_pcall() call
    if name_host == 'lua_pcall' then
      insert_lua_callchain(cc, event.lua, lua_symtab)
      lua_inserted = true
    end

    table.insert(cc, { name = name_host })
  end

  if lua_inserted == false then
    insert_lua_callchain(cc, event.lua, lua_symtab)
  end

  if sep_vmst == true then
    table.insert(cc, { name = VMST_NAMES[event.lua.vmstate] })
  end

  return cc
end

-- Collapse all the events into call tree
function M.collapse(events, host_symtab, lua_symtab, sep_vmst)
  local root = new_node('root', false)

  for _,ev in pairs(events) do
    local callchain = merge(ev, lua_symtab, host_symtab, sep_vmst)
    local curr_node = root
    for i=#callchain,1,-1 do
      curr_node = insert(callchain[i].name, curr_node, false)
    end
    insert('', curr_node, true)
  end

  return root
end

return M
