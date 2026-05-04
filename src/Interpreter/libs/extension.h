#ifndef INTERPRETER_LIBS_EXTENSION_H
#define INTERPRETER_LIBS_EXTENSION_H

#include "Interpreter/Base.h"
#include "Interpreter/Value.h"
#include "Interpreter/Core.h"

namespace LuaInterpreter {
namespace LuaLibs {

// returns entrypoint of module
std::string compile_and_add_extension_module(
    Interpreter* interp,

    const std::string& source,

    const std::string& modname
);

void compile_and_add_extension_function(
    Interpreter* interp,

    const std::string& source,

    const std::string& funcname,
    const std::vector< std::string >& params,
    const std::string& vararg,
    bool is_method,
    
    const std::string& globalname,
    const std::string& extensionname // for debug
);

}; // LuaLibs
}; // LuaInterpreter

#endif // INTERPRETER_LIBS_EXTENSION_H