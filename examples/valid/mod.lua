print("This should be printed only once when first load")

local function adder(x, y)
    local res = x + y
    if (res > 10) then print("res > 10!")
    else print("res <= 10") end
    return res
end

local mod = {
    ["adder"] = adder
}

return mod