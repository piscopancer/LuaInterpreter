#ifndef INTERPRETER_LIBS_DYNLIB_H
#define INTERPRETER_LIBS_DYNLIB_H

#include "Interpreter/Base.h"
#include "Interpreter/Value.h"
#include "Interpreter/Core.h"

#include "c_api.h"

namespace LuaInterpreter {
namespace LuaLibs {

class Dynlib {
public:
    static void include(Interpreter* interp);

    static std::shared_ptr<LuaValue::Nil> host_new_nil();
    static std::shared_ptr<LuaValue::Boolean> host_new_boolean( bool val );
    
    static std::shared_ptr<LuaValue::Number> host_new_number( std::variant<int64_t, std::float64_t> val );
    static std::shared_ptr<LuaValue::String> host_new_string( const std::string& str );
    
    static std::shared_ptr<LuaValue::Function> host_new_lua_function( const std::string& label, size_t arg_N, const std::string& varg );
    static std::shared_ptr<LuaValue::Function> host_new_cxx_function( cxx_func func );
    
    static std::shared_ptr<LuaValue::Thread> host_new_thread( Executioner* exec );
    
    static std::shared_ptr<LuaValue::Userdata> host_new_userdata( void* );
    
    static std::shared_ptr<LuaValue::Table> host_new_table( );

    struct DynHandle: public LuaValue::Userdata {
        void* handle = nullptr;
        void* (*finder)(const char*) = nullptr;
        static constexpr std::string typestr = "__dynlib_handle";

        DynHandle(const std::string& path);
        ~DynHandle();

        std::vector< std::shared_ptr<Value> > get (
            Executioner* exec,
            std::vector< std::shared_ptr<Value> > &args
        );

        static std::vector< std::shared_ptr<Value> > get_wrapper (
            Executioner* exec,
            std::vector< std::shared_ptr<Value> > &args
        );

        std::vector< std::shared_ptr<Value> > close (
            Executioner* exec,
            std::vector< std::shared_ptr<Value> > &args
        );
    };

    static std::vector< std::shared_ptr<Value> > open (
        Executioner* exec,
        std::vector< std::shared_ptr<Value> > &args
    );

    static std::vector< std::shared_ptr<Value> > close (
        Executioner* exec,
        std::vector< std::shared_ptr<Value> > &args
    );
};

}; // LuaLibs
}; // LuaInterpreter

#endif // #define INTERPRETER_LIBS_DYNLIB_H
