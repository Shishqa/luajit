#! /usr/bin/lua
--
-- lua-Harness : <https://fperrad.frama.io/lua-Harness/>
--
-- Copyright (C) 2009-2020, Perrad Francois
--
-- This code is licensed under the terms of the MIT/X11 license,
-- like Lua itself.
--

--[[

=head1 Lua scope

=head2 Synopsis

    % prove 211-scope.t

=head2 Description

See section "Visibility Rules" in "Reference Manual"
L<https://www.lua.org/manual/5.1/manual.html#2.6>,
L<https://www.lua.org/manual/5.2/manual.html#3.5>,
L<https://www.lua.org/manual/5.3/manual.html#3.5>,
L<https://www.lua.org/manual/5.4/manual.html#3.5>

See section "Local Variables and Blocks" in "Programming in Lua".

=cut

--]]

require'tap_harness'

plan(10)

--[[ scope ]]
x = 10
do
    local x = x
    is(x, 10, "scope")
    x = x + 1
    do
        local x = x + 1
        is(x, 12)
    end
    is(x, 11)
end
is(x, 10)

--[[ scope ]]
x = 10
local i = 1

while i<=x do
    local x = i*2
--    print(x)
    i = i + 1
end

if i > 20 then
    local x
    x = 20
    nok("scope")
else
    is(x, 10, "scope")
end

is(x, 10)

--[[ scope ]]
local a, b = 1, 10
if a < b then
    is(a, 1, "scope")
    local a
    is(a, nil)
end
is(a, 1)
is(b, 10)

-- Local Variables:
--   mode: lua
--   lua-indent-level: 4
--   fill-column: 100
-- End:
-- vim: ft=lua expandtab shiftwidth=4:
