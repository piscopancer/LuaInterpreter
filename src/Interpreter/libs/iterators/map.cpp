#include "map.h"

using namespace LuaInterpreter;
using namespace LuaLibs;


std::string imap_src = R"(
if (type(table) ~= "Table") then
    error("Map: table must be type of \"Table\"")
end
if (not callable(func)) then
    error("Map: func must be callable")
end

result = {}
for i, v in ipairs(table) do
    result[i] = func(v)
end

return result
)";

std::string map_src = R"(
if (type(table) ~= "Table") then
    error("Map: table must be type of \"Table\"")
end
if (not callable(func)) then
    error("Map: func must be callable")
end

result = {}
for k, v in pairs(table) do
    result[k] = func(v)
end

return result
)";


void Map::include(Interpreter* interp) {
    compile_and_add_extension_function(
        interp,

        imap_src,

        "__imap",
        {"table", "func"},
        "varg",
        false,

        "imap",
        "imap"
    );

    compile_and_add_extension_function(
        interp,

        map_src,

        "__map",
        {"table", "func"},
        "varg",
        false,

        "map",
        "map"
    );
}