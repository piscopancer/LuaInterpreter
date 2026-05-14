#ifndef DYNLIB_C_API
#define DYNLIB_C_API

#include "Interpreter/Base.h"
#include "Interpreter/Value.h"
#include "Interpreter/Core.h"

using namespace LuaInterpreter;

struct HostAllocator_t {
    std::shared_ptr<LuaValue::Nil> (*new_nil)();
    std::shared_ptr<LuaValue::Boolean> (*new_boolean)( bool val );
    std::shared_ptr<LuaValue::Number> (*new_number)( std::variant<int64_t, std::float64_t> val );
    std::shared_ptr<LuaValue::String> (*new_string)( const std::string& str );
    std::shared_ptr<LuaValue::Function> (*new_lua_function)( const std::string& label, size_t arg_N, const std::string& varg );
    std::shared_ptr<LuaValue::Function> (*new_cxx_function)( cxx_func func );
    std::shared_ptr<LuaValue::Thread> (*new_thread)( Executioner* exec );
    std::shared_ptr<LuaValue::Userdata> (*new_userdata)( void* );
    std::shared_ptr<LuaValue::Table> (*new_table)( );
};

// export
void register_function(cxx_func func, const std::string& name);

void inner_prepare();
void inner_insert_registred_functions(Interpreter* interp);
cxx_func inner_find_function(const char* name);


// Dll API
extern "C" {
    void set_allocator(HostAllocator_t host);
    void prepare();
    void* find_function(const char* name);
    void insert_registred_functions(void* interp);
}


#ifdef DYNLIB_C_API_IMPL

#include "Interpreter/Base.cpp"
#include "Interpreter/Value.cpp"

static HostAllocator_t host;
static std::unordered_map < std::string, cxx_func> registred;

void register_function(cxx_func func, const std::string& name) {
    registred[name] = func;
}

void inner_insert_registred_functions(Interpreter* interp) {
    for (auto &[name, func]: registred) {
        interp->cxx_funcnames.insert( {(cxx_func) func, name} );
    }
}

cxx_func inner_find_function(const char* name) {
    std::string strname(name);
    if (registred.contains(strname)) return registred[ strname ];
    return nullptr;
}


extern "C" {
    void set_allocator(HostAllocator_t host_) {
        host = host_;
    }

    void prepare() {
        inner_prepare();
    }

    void* find_function(const char* name) {
        return (void*) inner_find_function(name);
    }

    void insert_registred_functions(void* interp) {
        return inner_insert_registred_functions( ( Interpreter* ) interp );
    }
}

#endif //DYNLIB_C_API_IMPL
#endif // DYNLIB_C_API
