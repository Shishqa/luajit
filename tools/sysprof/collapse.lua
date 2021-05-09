local parse = require "sysprof.parse"
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

-- merge lua and host callchains into one callchain representing
-- transfer of control
local function merge(event, lua_symtab, host_symtab, sep_vmst)
  local cc = {}

  if sep_vmst == true then
    table.insert(cc, { name = VMST_NAMES[event.lua.vmstate] })
  end

  for _,h_fr in pairs(event.host.callchain) do
    local name_host = sotab.demangle(host_symtab, h_fr.addr)
    table.insert(cc, { name = name_host })

    -- We assume that usually the transfer of control
    -- looks like:
    --    HOST -> LUA -> HOST
    -- so for now, lua callchain starts from lua_pcall() call
    if name_host == 'lua_pcall' then
      for i=#event.lua.callchain,1,-1 do
        local l_fr = event.lua.callchain[i]
        local name_lua = ''
        if l_fr.type == FRAME.FFUNC then
          name_lua = vmdef.ffnames[l_fr.ffid]
        else
          name_lua = symtab.demangle(lua_symtab, {
            addr = l_fr.addr,
            line = l_fr.line
          })
        end
        table.insert(cc, { name = name_lua })
      end
    end
  end

  return cc
end

-- Collapse all the events into call tree
function M.collapse(events, host_symtab, lua_symtab, sep_vmst)
  local root = new_node('root', false)

  for _,ev in pairs(events) do
    local callchain = merge(ev, lua_symtab, host_symtab, sep_vmst)
    local curr_node = root
    for _,frame in pairs(callchain) do
      curr_node = insert(frame.name, curr_node, false)
    end
    insert('', curr_node, true)
  end

  return root
end

return M
