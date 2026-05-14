local stat, lib = pcall(function (...)
    return dynlib.open("dll.dll")
end) 

if (not stat) then
    print("Cannot load dll: ", lib)
    return 1
end

local func = lib:get("my_dll_function")

local res = func(1, 2, 3, "Ya yebal tvoy rot", nil, {})
print("Result:", res)

lib:close()

print("Result (after closing dll):", res)

print("Success")