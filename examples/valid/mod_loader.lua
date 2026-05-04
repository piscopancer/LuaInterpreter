local success, mod = pcall( function ()
    return require("mod.lua")
end)

if (not success) then
    print("Cannot load module")
    return 1
end

local res
res = mod.adder(4, 5)
print("Res:", res)

res = mod.adder(5, 6)
print("Res:", res)

local second_mod = require("mod.lua")

res = second_mod.adder(5, 6)
print("Res:", res)

print("are_same:", mod.adder == second_mod.adder)