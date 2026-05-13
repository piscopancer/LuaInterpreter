#include "module.h"
#include <filesystem>

using namespace LuaInterpreter;
using namespace LuaLibs;

void Module::include(Interpreter* interp) {
    interp->global.set("require", std::make_shared<LuaValue::Function>((cxx_func) &require));
    interp->cxx_funcnames[(cxx_func) &require] = "Module::require";
}

std::vector< std::shared_ptr<Value> > Module::require (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() != 1) {
        throw std::runtime_error("Module::require - expected exactly 1 argument");
    }
    if (args[0]->type() != Value::Type::STRING) {
        throw std::runtime_error("Module::require - argument must be a string");
    }

    std::string filepath = std::static_pointer_cast<LuaValue::String>(args[0])->value;
    if (!std::filesystem::exists(filepath)) {
        throw std::runtime_error("Module:require - No file with such name exists: " + filepath);
    }
    if (!std::filesystem::is_regular_file(filepath)) {
        throw std::runtime_error("Module:require - Input must be a regular file: " + filepath);
    }

    std::error_code ec;
    std::string modname = std::filesystem::canonical(filepath, ec).string();
    if (ec) {
        throw std::runtime_error("Module:require - Cannot make absolute path from: " + filepath);
    }

    #ifdef INTERPRETER_DEBUG
    std::cout << "Loaded modules: " << std::endl;
    for (auto p: exec->g->loaded_modules) {
        std::cout << p.first << std::endl;
    }
    #endif

    if (exec->g->loaded_modules.contains(modname)) {
        #ifdef INTERPRETER_DEBUG
        std::cout << "Module is already loaded" << std::endl;
        #endif
        return { exec->g->loaded_modules[modname] };
    }
    #ifdef INTERPRETER_DEBUG
    std::cout << "New module" << std::endl;
    #endif

    std::string source;
    {
        std::ifstream fd(filepath);
    
        std::string line;
        while (std::getline(fd, line)) {
            source += line + "\n";
        }
    }

    auto entry_label = compile_and_add_extension_module(exec->g, source, modname);

    exec->g->loading_modules.push(modname);
    auto entry_func = std::make_shared<LuaValue::Function>(entry_label, 0, "varg");

    std::vector< std::shared_ptr<Value> > mock;
    exec->raw_lua_call(entry_func, mock, 1);

    return {};
}