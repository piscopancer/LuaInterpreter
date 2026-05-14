#include "dynlib.h"

using namespace LuaInterpreter;
using namespace LuaLibs;
using namespace LuaValue;

#ifdef _WIN32
    #include <windows.h>
    #define LIB_HANDLE HMODULE
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_FUNCTION(handle, name) GetProcAddress(handle, name)
    #define CLOSE_LIBRARY(handle) FreeLibrary(handle)
#else
    #include <dlfcn.h>
    #define LIB_HANDLE void*
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
    #define GET_FUNCTION(handle, name) dlsym(handle, name)
    #define CLOSE_LIBRARY(handle) dlclose(handle)
#endif

std::shared_ptr<Nil> Dynlib::host_new_nil() {
    return std::make_shared<Nil>();
}

std::shared_ptr<Boolean> Dynlib::host_new_boolean( bool val ) {
    return std::make_shared<Boolean>(val);
}

std::shared_ptr<Number> Dynlib::host_new_number( std::variant<int64_t, std::float64_t> val ) {
    if ( std::holds_alternative<int64_t>(val) ) return std::make_shared<Number>( std::get<int64_t>(val) );
    return std::make_shared<Number>( std::get<std::float64_t>(val) );
}

std::shared_ptr<String> Dynlib::host_new_string( const std::string& str ) {
    return std::make_shared<String>( str );
}

std::shared_ptr<Function> Dynlib::host_new_lua_function( const std::string& label, size_t arg_N, const std::string& varg ) {
    return std::make_shared<Function>( label, arg_N, varg );
}
std::shared_ptr<Function> Dynlib::host_new_cxx_function( cxx_func func ) {
    return std::make_shared<Function>( func );
}

std::shared_ptr<Thread> Dynlib::host_new_thread( Executioner* exec ) {
    return std::make_shared<Thread>( exec );
}

std::shared_ptr<Userdata> Dynlib::host_new_userdata( void* data ) {
    return std::make_shared<Userdata>( data );
}

std::shared_ptr<Table> Dynlib::host_new_table( ) {
    return std::make_shared<Table>( );
}


void Dynlib::include(Interpreter* interp) {
    auto mod = std::make_shared<Table>();

    mod->set("open",     std::make_shared<LuaValue::Function>((cxx_func) &open));
    interp->cxx_funcnames[(cxx_func) &open] = "Dynlib::open";

    mod->set("close",    std::make_shared<LuaValue::Function>((cxx_func) &close));
    interp->cxx_funcnames[(cxx_func) &close] = "Dynlib::close";

    interp->global.set("dynlib", mod);
}


Dynlib::DynHandle::DynHandle(const std::string& path): Userdata(typestr) {
    handle = LOAD_LIBRARY(path.c_str());
    if (!handle) throw std::runtime_error("Dynlib::DynHandle::DynHandle - cannot open lib");
    meta.set("get", std::make_shared<LuaValue::Function>((cxx_func) &get_wrapper));
    meta.set("close", std::make_shared<LuaValue::Function>((cxx_func) &Dynlib::close));
}

Dynlib::DynHandle::~DynHandle() {
    if (handle) {
        CLOSE_LIBRARY( (LIB_HANDLE) handle);
        handle = nullptr;
    }
}

std::vector< std::shared_ptr<Value> > Dynlib::DynHandle::get (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);
    // args[0] = this
    // args[1] = name

    if (!handle) throw std::runtime_error("Dynlib::DynHandle::get - Handle is not initialized");
    if (args.size() != 2) throw std::runtime_error("Dynlib::DynHandle::get - Expected one argument");
    if (args[1]->type() != Value::Type::STRING) throw std::runtime_error("Dynlib::DynHandle::get - Argument must be a string");

    auto str = std::static_pointer_cast<String>(args[1]);
    cxx_func func = (cxx_func) finder( str->value.c_str() );
    if (!func) throw std::runtime_error("Dynlib::DynHandle::get - No such function: " + str->value);
    
    return { std::make_shared<Function>(func) };
}

std::vector< std::shared_ptr<Value> > Dynlib::DynHandle::get_wrapper (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args[0]->type() != Value::Type::USERDATA) throw std::runtime_error("Dynlib::DynHandle::get_wrapper - expected handle");
    auto handle = std::static_pointer_cast<DynHandle>(args[0]);
    return handle->get(exec, args);
}

std::vector< std::shared_ptr<Value> > Dynlib::DynHandle::close (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);
    // args[0] = this

    if (!handle) throw std::runtime_error("Dynlib::DynHandle::close - Handle is not initialized");
    CLOSE_LIBRARY( (LIB_HANDLE) handle);
    handle = nullptr;

    return {};
}

std::vector< std::shared_ptr<Value> > Dynlib::open (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() != 1) throw std::runtime_error("Dynlib::open - Expected exactly 1 argument");
    if (args[0]->type() != Value::Type::STRING) throw std::runtime_error("Dynlib::open - Argument must be a string");
    
    auto handle = std::make_shared< DynHandle > ( std::static_pointer_cast<String> (args[0])->value );

    void (*setter)(HostAllocator_t) = (void (*)(HostAllocator_t)) GET_FUNCTION( (LIB_HANDLE)handle->handle, "set_allocator" );
    if (!setter) throw std::runtime_error("Dynlib::open - No entry `set_allocator` found");
    
    HostAllocator_t allocator = {
        .new_nil = &host_new_nil,
        .new_boolean = &host_new_boolean,
        .new_number = &host_new_number,
        .new_string = &host_new_string,
        .new_lua_function = &host_new_lua_function,
        .new_cxx_function = &host_new_cxx_function,
        .new_thread = &host_new_thread,
        .new_userdata = &host_new_userdata,
        .new_table = &host_new_table,
    };
    setter(allocator);

    void (*prep)() = (void (*)()) GET_FUNCTION( (LIB_HANDLE)handle->handle, "prepare" );
    if (!prep) throw std::runtime_error("Dynlib::open - No entry `prep` found");
    prep();

    void (*insert)(Interpreter*) = (void (*)(Interpreter*)) GET_FUNCTION( (LIB_HANDLE)handle->handle, "insert_registred_functions" );
    if (!insert) throw std::runtime_error("Dynlib::open - No entry `insert_registred_functions` found");
    insert(exec->g);

    handle->finder = (void* (*)(const char*)) GET_FUNCTION( (LIB_HANDLE)handle->handle, "find_function" );
    if (!handle->finder) throw std::runtime_error("Dynlib::open - No entry `find_function` found");

    return { handle };
}

std::vector< std::shared_ptr<Value> > Dynlib::close (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() != 1) throw std::runtime_error("Dynlib::close - expected exactly 1 argument");
    if (args[0]->type() != Value::Type::USERDATA) throw std::runtime_error("Dynlib::close - argument must be a handle");
    auto handle = std::static_pointer_cast<DynHandle>(args[0]);
    return handle->close(exec, args);
}