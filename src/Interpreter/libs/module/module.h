#ifndef INTERPRETER_LIBS_MODULE_H
#define INTERPRETER_LIBS_MODULE_H

#include "Interpreter/Base.h"
#include "Interpreter/Value.h"
#include "Interpreter/Core.h"
#include "Interpreter/libs/extension.h"

namespace LuaInterpreter {
namespace LuaLibs {

class Module {
public:
    static void include(Interpreter* interp);

    static std::vector< std::shared_ptr<Value> > require (
        Executioner* exec,
        std::vector< std::shared_ptr<Value> > &args
    );
};

}; // LuaLibs
}; // LuaInterpreter
#endif // INTERPRETER_LIBS_MODULE_H