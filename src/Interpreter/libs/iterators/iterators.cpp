#include "iterators.h"

using namespace LuaInterpreter;
using namespace LuaLibs;

void Iterators::include(Interpreter* interp) {
    interp->global.set("pairs",     std::make_shared<LuaValue::Function>((cxx_func) &pairs));
    interp->cxx_funcnames[(cxx_func) &pairs] = "Iterators::pairs";
    interp->global.set("ipairs",    std::make_shared<LuaValue::Function>((cxx_func) &ipairs));
    interp->cxx_funcnames[(cxx_func) &ipairs] = "Iterators::ipairs";
}

std::vector< std::shared_ptr<Value> > Iterators::ipairs(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() != 1) throw std::runtime_error("Iterators::ipairs - Expected exactly 1 argument");
    if (args[0]->type() != Value::Type::TABLE) throw std::runtime_error("Iterators::ipairs - Argument must be a table");
    auto table = std::static_pointer_cast<LuaValue::Table>(args[0]);
    auto it = std::make_shared<IIterator>( table );
    auto idx = std::make_shared<LuaValue::Number>((int64_t) 0);

    return {it, table, idx};
}

std::vector< std::shared_ptr<Value> > Iterators::pairs(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() != 1) throw std::runtime_error("Iterators::pairs - Expected exactly 1 argument");

    if (args[0]->type() == Value::Type::USERDATA) {
        throw std::runtime_error("Iterators::pairs - Arbitary pair is not available");

        auto ud = std::static_pointer_cast<LuaValue::Userdata>(args[0]);
        auto func_arg = ud->meta.at("__pairs");

        auto pair = exec->to_function(func_arg, Executioner::ALL);
        if (!pair.first) throw std::runtime_error("Iterators::pairs - Cannot call non-callable object");
        std::shared_ptr<LuaValue::Function> function = pair.first;

        std::vector< std::shared_ptr<Value> > values;
        if (pair.second) values.push_back(pair.second);
        else values.push_back(ud);

        exec->raw_call(function, values, 3);
        if (!function->func) {
            int end_point = exec->to_return.size() - 1;
            // user-defined func
            while ((int)exec->to_return.size() != end_point) {
                exec->execute();
            }
            // till first RET
        }
        auto v3 = exec->pop_top();
        auto v2 = exec->pop_top();
        auto v1 = exec->pop_top();
        return {v1, v2, v3};
    }
    if (args[0]->type() != Value::Type::TABLE) {
        throw std::runtime_error("Iterators::pairs - cannot iterate over non-table type");
    }
    auto table = std::static_pointer_cast<LuaValue::Table>(args[0]);
    auto func_arg = table->meta.at("__pairs");
    if (func_arg) {
        auto pair = exec->to_function(func_arg, Executioner::ALL);
        if (!pair.first) throw std::runtime_error("Iterators::pairs - Cannot call non-callable object");
        std::shared_ptr<LuaValue::Function> function = pair.first;

        std::vector< std::shared_ptr<Value> > values;
        if (pair.second) values.push_back(pair.second);
        else values.push_back(table);

        exec->raw_call(function, values, 3);
        if (!function->func) {
            int end_point = exec->to_return.size() - 1;
            // user-defined func
            while ((int)exec->to_return.size() != end_point) {
                exec->execute();
            }
            // till first RET
        }
        auto v3 = exec->pop_top();
        auto v2 = exec->pop_top();
        auto v1 = exec->pop_top();
        return {v1, v2, v3};
    }

    auto it = std::make_shared<KIterator>( table );
    auto idx = std::make_shared<LuaValue::Number>((int64_t) 0);

    return {it, table, idx};
}


Iterators::Iterator::Iterator(const std::string& typestr, std::shared_ptr<Value> container):
    Userdata(typestr),
    container(container)
{
    data = container.get();
    meta.set("__call", std::make_shared<LuaValue::Function>((cxx_func) &next_wrapper));
}


std::vector< std::shared_ptr<Value> > Iterators::Iterator::next_wrapper(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args[0]->type() != Value::Type::USERDATA) throw std::runtime_error("Iterators::Iterator::next_wrapper - Expected an iterator");
    auto it = std::static_pointer_cast<Iterator>(args[0]);
    return it->next(exec, args);
}




Iterators::IIterator::IIterator(std::shared_ptr<LuaValue::Table> container):
    Iterator(typestr, container),
    next_idx(0)
{}

std::vector< std::shared_ptr<Value> > Iterators::IIterator::next(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);

    // arg[0] = this
    // arg[1] = container
    // arg[2] = key

    if (args.size() != 3) throw std::runtime_error("Iterators::IIterator::next - Too few arguments");
    // ignore arguments
    auto table = std::static_pointer_cast<LuaValue::Table>(container);

    if (next_idx >= table->int_table.size()) {
        return { std::make_shared<LuaValue::Nil>() };
    }

    size_t idx = next_idx++;
    if (!table->int_table.contains(idx)) {
        return { std::make_shared<LuaValue::Nil>() };
    }
    auto v = table->int_table[idx];
    if (!v) return { std::make_shared<LuaValue::Nil>() };

    return {
        std::make_shared<LuaValue::Number>((int64_t) idx),
        v
    };
}

Iterators::KIterator::KIterator(std::shared_ptr<LuaValue::Table> container):
    Iterator(typestr, container),
    string_it(container->string_table.begin()),
    int_it(container->int_table.begin()),
    func_it(container->func_table.begin()),
    data_it(container->data_table.begin())
{}

std::vector< std::shared_ptr<Value> > Iterators::KIterator::next(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);


    auto table = std::static_pointer_cast<LuaValue::Table>(container);
    
    if (string_it != table->string_table.end()) {
        auto pair = *string_it;
        string_it++;
        return { std::make_shared<LuaValue::String>(pair.first), pair.second };
    }

    if (int_it != table->int_table.end()) {
        auto pair = *int_it;
        int_it++;
        return { std::make_shared<LuaValue::Number>(pair.first), pair.second };
    }

    if (func_it != table->func_table.end()) {
        auto pair = *func_it;
        func_it++;
        std::shared_ptr<LuaValue::Function> function;

        if (pair.first.starts_with("cxx_func__")) {
            void* address = nullptr;
            std::string addr_str = pair.first.substr(std::string("cxx_func__").size());
            
            char* endptr = nullptr;
            int64_t addr_value = std::strtoll(addr_str.c_str(), &endptr, 10);
            
            if (endptr == addr_str.c_str() + addr_str.size()) {
                address = reinterpret_cast<void*>(addr_value);
            }

            if (address) {
                cxx_func func = (cxx_func) address;
                function = std::make_shared<LuaValue::Function>(func);
            } else {
                function = std::make_shared<LuaValue::Function>(pair.first, 0, "varg");
            }
        } else {
            if (exec->g->func_args.contains(pair.first)) {
                auto p = exec->g->func_args[pair.first];
                function = std::make_shared<LuaValue::Function>(pair.first, p.first, p.second);
            } else {
                function = std::make_shared<LuaValue::Function>(pair.first, 0, "varg");
            }
        }
        return { function, pair.second };
    }

    if (data_it != table->data_table.end()) {
        auto pair = *data_it;
        data_it++;
        
        return { std::make_shared<LuaValue::Userdata>(pair.first), pair.second };
    }

    return { std::make_shared<LuaValue::Nil>() };
}