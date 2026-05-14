#define DYNLIB_C_API_IMPL
#include "../src/Interpreter/libs/dynlib/c_api.h"

std::string type(std::shared_ptr< Value > value) {
    switch (value->type()) {
        case Value::Type::BARRIER: {
            #ifdef INTERPRETER_DEBUG
            return "Barrier";
            #else 
            throw std::runtime_error("KYS 2");
            #endif
        } break;

        case Value::Type::NIL: {
            return "Nil";
        } break;
        case Value::Type::BOOLEAN: {
            return "Boolean";
        } break;
        case Value::Type::NUMBER: {
            return "Number";
        } break;
        case Value::Type::STRING: {
            return "String";
        } break;
        case Value::Type::FUNCTION: {
            return "Function";
        } break;
        case Value::Type::THREAD: {
            return "Thread";
        } break;
        case Value::Type::USERDATA: {
            auto ud = std::static_pointer_cast<LuaValue::Userdata>(value);
            auto typestr = ud->meta.at("__type");
            if (!typestr || typestr->type() != Value::Type::STRING) return "Userdata";
            return std::static_pointer_cast<LuaValue::String>(typestr)->value;
        } break;
        case Value::Type::TABLE: {
            return "Table";
        } break;
    }
    throw std::runtime_error("KYS 3");
}

std::vector< std::shared_ptr<Value> > my_dll_function (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args 
) {
    std::cout << "This is call to my function!" << std::endl;
    std::cout << "Argument count: " << args.size() << std::endl;
    for (auto arg: args) {
        std::cout << arg << " " << type(arg) << std::endl;
    }
    return { host.new_number((int64_t) args.size()) };
}

void inner_prepare() {
    register_function((cxx_func) &my_dll_function, "my_dll_function");
}