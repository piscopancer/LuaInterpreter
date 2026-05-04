#include "Core.h"

using namespace LuaInterpreter;


std::shared_ptr<Value> Scope::get(const std::string &name) {
    if (!locals.contains(name)) {
        return nullptr;
    }
    return locals[name];
}
void Scope::set(const std::string &name, std::shared_ptr<Value> value) {
    locals[name] = value;
}




std::shared_ptr<Value> Interpreter::get(const std::string &name) {
    return global.get(name);
}
void Interpreter::set(const std::string &name, std::shared_ptr<Value> value) {
    global.set(name, value);
}

void Interpreter::collect_labels(const std::vector< Instruction >& pg, size_t start) {
    size_t N = pg.size();
    for (size_t i=start; i<N; i++) {
        const Instruction &inst = pg[i];
        if (inst.type == Instruction::Type::LABEL) {
            labels[inst.label] = i;
        } else if (inst.type == Instruction::Type::PUT_FUNC) {
            func_args[inst.label] = {inst.N, inst.name};
        }
    }
}

Interpreter::Interpreter(
    const std::vector< Instruction >& program
) {
    this->program = program;
    LuaLibs::IO::include(this);
    LuaLibs::Iterators::include(this);
    LuaLibs::Map::include(this);
    LuaLibs::Typing::include(this);
    LuaLibs::Error::include(this);
    LuaLibs::Math::include(this);
    LuaLibs::StringLib::include(this);
    LuaLibs::Coroutine::include(this);
    LuaLibs::Module::include(this);
    
    collect_labels(program);
    auto entry = std::make_shared<LuaValue::Function>("_start", 0, "varg");
    workers.push_back(
        std::make_unique<Executioner>(
            this, 
            next_tid++,
            entry, 
            std::vector< std::shared_ptr<Value> > ()
        )
    );
}

bool Interpreter::run() {
    bool end = false;
    while (!end) { 
        end = true;
        for (auto &w: workers) {
            if (w && w->running && !w->suspended_coroutine) {
                end = false;
                try {
                    w->execute();
                } catch (std::runtime_error& e) {
                    if (w->pcalls.empty()) {
                        std::cout << "Uncaught exception in thread " << w->tid << ":" << std::endl;
                        std::cout << e.what() << std::endl;
                        while (!w->callstack.empty()) {
                            std::cout << "From " << w->callstack.top() << std::endl;
                            w->callstack.pop();
                        }
                        return false;
                    } else {
                        Executioner::Catch c = w->pcalls.top(); w->pcalls.pop();
                        #ifdef INTERPRETER_DEBUG
                        std::cout << "pcall caught exception: " << e.what() << std::endl;
                        #endif
                        while (w->ret_addr.size()    != c.ret_addr_size  ) w->ret_addr.pop();
                        while (w->scopes.size()      != c.scopes_size    ) w->scopes.pop();
                        while (w->stacks.size()      != c.stacks_size    ) w->stacks.pop();
                        while (w->callstack.size()   != c.callstack_size ) w->callstack.pop();
                        while (w->to_return.size()   != c.to_return_size ) w->to_return.pop();
                        w->ip = c.ip;

                        w->stacks.top().push( std::make_shared<LuaValue::Boolean>(false) );
                        w->stacks.top().push( std::make_shared<LuaValue::String>(e.what()) );
                    }
                }
                break;
            }
        }
        if (end) break;
    }
    return true;
}





std::shared_ptr<Value> Executioner::get(const std::string &name) {
    for (auto it = scopes.top().rbegin(); it != scopes.top().rend(); it++) {
        if (auto res = it->get(name)) return res;
    }
    return g->get(name);
}
void Executioner::set(const std::string &name, std::shared_ptr<Value> value) {
    for (auto it = scopes.top().rbegin(); it != scopes.top().rend(); it++) {
        if (auto res = it->get(name)) {
            it->set(name, value);
            return;
        }
    }
    g->set(name, value);
}

Executioner::Executioner(
    Interpreter *g,
    size_t tid,
    std::shared_ptr<LuaValue::Function> entry,
    std::vector< std::shared_ptr<Value> > args
) {
    this->g = g;
    this->running = true;
    this->tid = tid;
    
    if (entry->func) {
        rets = entry->func(this, args);
        running = false;
        stop = true;
        release_turn();
    } else {
        // 1 scope
        scopes.push( { Scope{} } );
        stacks.push({});
        this->ip = g->labels.at(entry->label);
        callstack.push(entry->label);
        for (auto &arg: args) {
            stacks.top().push(arg);
        }
    }
}

void Executioner::execute() {
    stop = false;
    while (running && !suspended_coroutine && !stop) {
        Instruction* inst = fetch_instruction();
        execute(inst);
    }
}

void Executioner::release_turn() {
    stop = true;
    if (tied_to) {
        for (auto waiter: tied_to->waiters) {
            // true - success
            waiter->stacks.top().push( std::make_shared<LuaValue::Boolean>( true ) );
            
            for (auto &r: rets) {
                waiter->stacks.top().push( r );
            }
            
            if (waiter->suspended_coroutine) waiter->suspended_coroutine = false;
        }
        tied_to->waiters.clear();
    }
}

Instruction* Executioner::fetch_instruction() {
    if (ip++ >= g->program.size()) {
        throw std::runtime_error("Indexing instruction outside of program");
    }
    return &g->program[ip];
}

void Executioner::execute(Instruction* inst) {
    #ifdef INTERPRETER_DEBUG
    std::cout << std::endl << ip << " : " << *inst << std::endl;
    #endif
    bool unknown_inst_error = false;

    // try {
    switch (inst->type) {
        case Instruction::Type::NOP: {
            NOP(inst);
        } break;
        case Instruction::Type::PUT_SCOPE: {
            PUT_SCOPE(inst);
        } break;
        case Instruction::Type::POP_SCOPE: {
            POP_SCOPE(inst);
        } break;
        case Instruction::Type::PUT_STACK: {
            PUT_STACK(inst);
        } break;
        case Instruction::Type::POP_STACK: {
            POP_STACK(inst);
        } break;
        case Instruction::Type::CLEAR_STACK: {
            CLEAR_STACK(inst);
        } break;
        case Instruction::Type::MOVE_STACK: {
            MOVE_STACK(inst);
        } break;
        case Instruction::Type::LOAD: {
            LOAD(inst);
        } break;
        case Instruction::Type::STORE: {
            STORE(inst);
        } break;
        case Instruction::Type::SET_LOCAL: {
            SET_LOCAL(inst);
        } break;
        case Instruction::Type::SET_GLOBAL: {
            SET_GLOBAL(inst);
        } break;
        case Instruction::Type::SET_ATTR: {
            SET_ATTR(inst);
        } break;
        case Instruction::Type::GLOBATTR: {
            GLOBATTR(inst);
        } break;
        case Instruction::Type::INDEX: {
            INDEX(inst);
        } break; 
        case Instruction::Type::DYN_INDEX: {
            DYN_INDEX(inst);
        } break;
        case Instruction::Type::METAINDEX: {
            METAINDEX(inst);
        } break;
        case Instruction::Type::METAASSIGN_WHAT_WHOM: {
            METAASSIGN_WHAT_WHOM(inst);
        } break;
        case Instruction::Type::ASSIGN_WHAT_WHOM: {
            ASSIGN_WHAT_WHOM(inst);
        } break;
        case Instruction::Type::ASSIGN_WHOM_WHAT: {
            ASSIGN_WHOM_WHAT(inst);
        } break;
        case Instruction::Type::ASSIGN_WHAT_WHOM_WHERE: {
            ASSIGN_WHAT_WHOM_WHERE(inst);
        } break;
        case Instruction::Type::ASSIGN_WHOM_WHERE_WHAT: {
            ASSIGN_WHOM_WHERE_WHAT(inst);
        } break;
        case Instruction::Type::BRANCH: {
            BRANCH(inst);
        } break;
        case Instruction::Type::GOTO: {
            GOTO(inst);
        } break;
        case Instruction::Type::LABEL: {
            LABEL(inst);
        } break;
        case Instruction::Type::CALL: {
            CALL(inst);
        } break;
        case Instruction::Type::REV_CALL: {
            REV_CALL(inst);
        } break;
        case Instruction::Type::RET: {
            RET(inst);
        } break;
        case Instruction::Type::PUT_NIL: {
            PUT_NIL(inst);
        } break;
        case Instruction::Type::PUT_TRUE: {
            PUT_TRUE(inst);
        } break;
        case Instruction::Type::PUT_FALSE: {
            PUT_FALSE(inst);
        } break;
        case Instruction::Type::PUT_STR: {
            PUT_STR(inst);
        } break;
        case Instruction::Type::PUT_NUM: {
            PUT_NUM(inst);
        } break;
        case Instruction::Type::PUT_TABLE: {
            PUT_TABLE(inst);
        } break;
        case Instruction::Type::PUT_FUNC: {
            PUT_FUNC(inst);
        } break;
        case Instruction::Type::PUT_BARRIER: {
            PUT_BARRIER(inst);
        } break;
        case Instruction::Type::POP_BARRIER: {
            POP_BARRIER(inst);
        } break;
        case Instruction::Type::DISCARD: {
            DISCARD(inst);
        } break;
        case Instruction::Type::DUP: {
            DUP(inst);
        } break;
        case Instruction::Type::LE: {
            LE(inst);
        } break; 
        case Instruction::Type::LT: {
            LT(inst);
        } break;
        case Instruction::Type::GE: {
            GE(inst);
        } break; 
        case Instruction::Type::GT: {
            GT(inst);
        } break;
        case Instruction::Type::EQ: {
            EQ(inst);
        } break; 
        case Instruction::Type::NEQ: {
            NEQ(inst);
        } break;
        case Instruction::Type::CONCAT: {
            CONCAT(inst);
        } break;
        case Instruction::Type::ADD: {
            ADD(inst);
        } break; 
        case Instruction::Type::SUB: {
            SUB(inst);
        } break;
        case Instruction::Type::MUL: {
            MUL(inst);
        } break; 
        case Instruction::Type::MOD: {
            MOD(inst);
        } break;
        case Instruction::Type::DIV: {
            DIV(inst);
        } break; 
        case Instruction::Type::TRUEDIV: {
            TRUEDIV(inst);
        } break;
        case Instruction::Type::SHLEFT: {
            SHLEFT(inst);
        } break; 
        case Instruction::Type::SHRIGHT: {
            SHRIGHT(inst);
        } break;
        case Instruction::Type::AND: {
            AND(inst);
        } break; 
        case Instruction::Type::OR: {
            OR(inst);
        } break;
        case Instruction::Type::BITAND: {
            BITAND(inst);
        } break; 
        case Instruction::Type::BITOR: {
            BITOR(inst);
        } break; 
        case Instruction::Type::BITXOR: {
            BITXOR(inst);
        } break;
        case Instruction::Type::POW: {
            POW(inst);
        } break;
        case Instruction::Type::HASH: {
            HASH(inst);
        } break;
        case Instruction::Type::NEG: {
            NEG(inst);
        } break; 
        case Instruction::Type::NOT: {
            NOT(inst);
        } break; 
        case Instruction::Type::BITNOT: {
            BITNOT(inst);
        } break;

        default: {
            unknown_inst_error = true;
        }
    }
    
    if (unknown_inst_error) {
        throw std::runtime_error("Unknown instruction type: " + std::to_string((int)inst->type));
    }

    #ifdef INTERPRETER_DEBUG
    std::cout << "tid: " << tid << std::endl;
    std::cout << "funccall: " << callstack.top() << std::endl;
    std::cout << "#stacks: " << stacks.size() << std::endl;
    if (!stacks.empty()) {
        auto topstack = stacks.top();
        if (topstack.empty()) std::cout << "empty" << std::endl;
        while (!topstack.empty()) {
            auto v = topstack.top();
            std::cout << v << " " << type_of(v);
            if (v->type() == Value::Type::BOOLEAN) {
                std::cout << " " << std::static_pointer_cast<LuaValue::Boolean>(v)->value;
            } else
            if (v->type() == Value::Type::NUMBER) {
                auto num = std::static_pointer_cast<LuaValue::Number>(v);
                if (num->kind == LuaValue::Number::Kind::INT) std::cout << " " << num->integer << "i";
                else std::cout << " " << num->floating << "f";
            }
            std::cout << std::endl;
            topstack.pop();
        }
    }
    #endif
    
}

std::string Executioner::type_of(std::shared_ptr< Value > value) {
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

bool Executioner::is_type_of(std::shared_ptr< Value > value, const std::string& type) {
    return type_of(value) == type;
}

bool Executioner::is_barrier(const std::stack< std::shared_ptr<Value> > st) const {
    return st.top()->type() == Value::Type::BARRIER;
}    

std::shared_ptr< Value > Executioner::pop_top() {
    auto &top_stack = stacks.top();

    auto top_elem = top_stack.top();

    if (top_elem->type() != Value::Type::BARRIER) {
        top_stack.pop();
        return top_elem;
    }

    return std::shared_ptr<Value> (new LuaValue::Nil);
}

std::pair<
    std::shared_ptr<LuaValue::Function>,    // function
    std::shared_ptr<Value>   // reference
> Executioner::to_function(
    std::shared_ptr<Value> func_arg,
    int max_recursion
) {
    auto type = func_arg->type();
    if (!func_arg) {
        return {nullptr, nullptr};
    }

    std::shared_ptr<LuaValue::Function> function;
    if (type == Value::Type::FUNCTION)  {
        return {std::static_pointer_cast<LuaValue::Function>(func_arg), nullptr};
    }

    if (type == Value::Type::TABLE) {
        auto res = ((LuaValue::Table*) func_arg.get())->meta.at("__call");
        if (!res || (max_recursion <= 0 && max_recursion != ALL)) {
            return {nullptr, nullptr};
        }
        if (res->type() != Value::Type::FUNCTION) {
            if (max_recursion == ALL) return to_function(res, max_recursion); 
            return to_function(res, max_recursion-1); 
        }
        return {std::static_pointer_cast<LuaValue::Function>(res), func_arg};
    }

    if (type == Value::Type::USERDATA) {
        auto res = ((LuaValue::Userdata*) func_arg.get())->meta.at("__call");
        if (!res || (max_recursion <= 0 && max_recursion != ALL)) {
            return {nullptr, nullptr};
        }
        if (res->type() != Value::Type::FUNCTION) {
            if (max_recursion == ALL) return to_function(res, max_recursion); 
            return to_function(res, max_recursion-1); 
        }
        return {std::static_pointer_cast<LuaValue::Function>(res), func_arg};
    }
    return {nullptr, nullptr};
}

std::shared_ptr< LuaValue::Boolean > Executioner::to_bool(std::shared_ptr< Value > value) {
    if (value->type() == Value::Type::NIL) {
        return std::make_shared<LuaValue::Boolean>(false);
    }
    if (value->type() == Value::Type::BOOLEAN)  {
        return std::make_shared<LuaValue::Boolean>(std::static_pointer_cast<LuaValue::Boolean>(value)->value);
    }
    return std::make_shared<LuaValue::Boolean>(true);
}

std::shared_ptr< LuaValue::Number > Executioner::to_num(std::shared_ptr< Value > value) {
    if (value->type() == Value::Type::NUMBER) {
        std::shared_ptr<LuaValue::Number> num = std::static_pointer_cast<LuaValue::Number> (value);

        return std::shared_ptr<LuaValue::Number>( 
            new LuaValue::Number(*num) 
        );
    }
    if (value->type() == Value::Type::NIL) {
        return std::shared_ptr<LuaValue::Number>( 
            new LuaValue::Number((int64_t) 0) 
        );
    }
    
    if (value->type() == Value::Type::BOOLEAN) {
        std::shared_ptr<LuaValue::Boolean> bl = std::static_pointer_cast<LuaValue::Boolean> (value);
        return std::shared_ptr<LuaValue::Number>( new LuaValue::Number((int64_t) bl->value) );
    }

    return nullptr;
}

bool Executioner::are_equal(std::shared_ptr<Value> arg1, std::shared_ptr<Value> arg2) {
    if (arg1->type() != arg2->type()) {
        return false;
    }

    auto type = arg1->type();
    switch (type) {
        case Value::Type::NIL: {
            return true;
        } break;

        case Value::Type::BOOLEAN: {
            auto b1 = ((LuaValue::Boolean*)arg1.get());
            auto b2 = ((LuaValue::Boolean*)arg2.get());
            return b1->value == b2->value;
        } break;

        case Value::Type::NUMBER: {
            auto n1 = ((LuaValue::Number*)arg1.get());
            auto n2 = ((LuaValue::Number*)arg2.get());
            return *n1 == *n2;
        } break;

        case Value::Type::STRING: {
            auto s1 = ((LuaValue::String*)arg1.get());
            auto s2 = ((LuaValue::String*)arg2.get());
            return s1->value == s2->value;
        } break;

        case Value::Type::FUNCTION: {
            auto f1 = ((LuaValue::Function*)arg1.get());
            auto f2 = ((LuaValue::Function*)arg2.get());
            return f1->key() == f2->key();
        } break;

        case Value::Type::THREAD: {
            return false;
        } break;

        case Value::Type::USERDATA: {
            return false;
        } break;

        case Value::Type::TABLE: {
            return false;
        } break;

        default:
        case Value::Type::BARRIER: {
            throw std::runtime_error("KYS");
        } break;
    }
}

void Executioner::NOP(Instruction *inst) {
    (void) (inst);
}

void Executioner::PUT_SCOPE(Instruction *inst) {
    scopes.top().push_back( Scope{} );
}

void Executioner::POP_SCOPE(Instruction *inst) {
    scopes.top().pop_back();
}

void Executioner::PUT_STACK(Instruction *inst) {
    stacks.push({});
}

void Executioner::POP_STACK(Instruction *inst) {
    stacks.pop();
}

void Executioner::CLEAR_STACK(Instruction *inst) {
    stacks.top() = std::stack< std::shared_ptr<Value> >();
}

void Executioner::MOVE_STACK(Instruction *inst) {
    // N
    // ...
    // 0
    auto top = stacks.top();
    stacks.pop();

    // N ... 0
    std::vector< std::shared_ptr<Value> > values;
    if (inst->whole_stack) {
        while (!top.empty()) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            } 
            top.pop();
        }
    }
    else {
        for (size_t i=0; i<inst->N; i++) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            } 
            top.pop();
        }
    }

    if (inst->reverse_stack) {
        for (auto it=values.begin(); it != values.end(); it++) {
            stacks.top().push(*it);
        }            
    } else {
        for (auto it=values.rbegin(); it != values.rend(); it++) {
            stacks.top().push(*it);
        }
    }
}

void Executioner::LOAD(Instruction *inst) {
    auto v = get(inst->name);
    if (v) stacks.top().push( v );
    else stacks.top().push( std::make_shared<LuaValue::Nil>() );
}

void Executioner::STORE(Instruction *inst) {
    auto value = pop_top();
    set(inst->name, value);
}

void Executioner::SET_LOCAL(Instruction *inst) {
    auto value = pop_top();
    scopes.top().back().set(inst->name, value);
}

void Executioner::SET_GLOBAL(Instruction *inst) {
    auto value = pop_top();
    g->global.set(inst->name, value);
}

void Executioner::SET_ATTR(Instruction *inst) {
    throw std::runtime_error("Interpretator: Attributes are not supported yet");
}

void Executioner::GLOBATTR(Instruction *inst) {
    throw std::runtime_error("Interpretator: Attributes are not supported yet");
}

void Executioner::INDEX(Instruction *inst) {
    auto value = pop_top();
    if (value->type() != Value::Type::TABLE) {
        throw std::runtime_error("Interpretator: Indexing of non-table type");
    }
    auto table = std::static_pointer_cast<LuaValue::Table>(value);
    if (table->string_table.contains(inst->field)) {
        stacks.top().push( table->at(inst->field) );
    }
    else stacks.top().push( std::make_shared<LuaValue::Nil>() );
}

void Executioner::DYN_INDEX(Instruction *inst) {
    auto exp = pop_top();
    auto table = pop_top();

    if (table->type() != Value::Type::TABLE) {
        throw std::runtime_error("Interpretator: Indexing of non-table type");
    }

    auto v = ((LuaValue::Table*) table.get())->at(exp);
    if (v) stacks.top().push(v);
    else stacks.top().push( std::make_shared<LuaValue::Nil>() );
}

void Executioner::METAINDEX(Instruction *inst) {
    auto object = pop_top();
    auto type = object->type();
    if (
        type != Value::Type::TABLE &&
        type != Value::Type::USERDATA &&
        type != Value::Type::STRING
    ) {
        throw std::runtime_error("Interpretator: Metaindexing of incompatible type");
    }

    if (type == Value::Type::STRING) {
        auto string = get("string");
        if (!string || string->type() != Value::Type::TABLE) {
            throw std::runtime_error("Interpretator: string must be a library");
        }
        auto table = std::static_pointer_cast<LuaValue::Table>(string);
        
        auto v = table->at(inst->metafield);
        if (v) stacks.top().push(v);
        else stacks.top().push( std::make_shared<LuaValue::Nil>() );
    } else
    if (type == Value::Type::TABLE) {
        auto v = ((LuaValue::Table*) object.get())->meta.at(inst->metafield);
        if (v) stacks.top().push(v);
        else stacks.top().push( std::make_shared<LuaValue::Nil>() );
    } else 
    {
        auto v = ((LuaValue::Userdata*) object.get())->meta.at(inst->metafield);
        if (v) stacks.top().push(v);
        else stacks.top().push( std::make_shared<LuaValue::Nil>() );
    }

    stacks.top().push(object);
}

void Executioner::METAASSIGN_WHAT_WHOM(Instruction *inst) {
    auto whom = pop_top();
    auto what = pop_top();

    auto type = whom->type();

    if (
        type != Value::Type::TABLE &&
        type != Value::Type::USERDATA
    ) {
        throw std::runtime_error("Interpretator: Metaassigning of incompatible type");
    }

    if (type == Value::Type::TABLE) {
        ((LuaValue::Table*) whom.get())->meta.set(inst->metafield, what);
    } else {
        ((LuaValue::Userdata*) whom.get())->meta.set(inst->metafield, what);
    }
}

void Executioner::ASSIGN_WHAT_WHOM(Instruction *inst) {
    auto whom = pop_top();
    auto what = pop_top();

    auto type = whom->type();

    if (type != Value::Type::TABLE) {
        throw std::runtime_error("Interpretator: Assigning of non-table type");
    }

    ((LuaValue::Table*) whom.get())->set(inst->field, what);        
}

void Executioner::ASSIGN_WHOM_WHAT(Instruction *inst) {
    auto what = pop_top();
    auto whom = pop_top();

    auto type = whom->type();

    if (type != Value::Type::TABLE) {
        throw std::runtime_error("Interpretator: Assigning of non-table type");
    }

    ((LuaValue::Table*) whom.get())->set(inst->field, what);    
}

void Executioner::ASSIGN_WHAT_WHOM_WHERE(Instruction *inst) {
    auto where = pop_top();
    auto whom = pop_top();
    auto what = pop_top();

    auto type = whom->type();

    if (type != Value::Type::TABLE) {
        throw std::runtime_error("Interpretator: Assigning of non-table type");
    }

    ((LuaValue::Table*) whom.get())->set(where, what); 
}

void Executioner::ASSIGN_WHOM_WHERE_WHAT(Instruction *inst) {
    auto what = pop_top();
    auto where = pop_top();
    auto whom = pop_top();

    auto type = whom->type();

    if (type != Value::Type::TABLE) {
        throw std::runtime_error("Interpretator: Assigning of non-table type");
    }

    ((LuaValue::Table*) whom.get())->set(where, what); 
}

void Executioner::BRANCH(Instruction *inst) {
    auto cond = to_bool( pop_top() );
    if (cond->value) {
        ip = g->labels.at(inst->label);
    }
}

void Executioner::GOTO(Instruction *inst) {
    ip = g->labels.at(inst->label);
}

void Executioner::LABEL(Instruction *inst) {
    (void) (inst);
}

void Executioner::raw_lua_call(
    std::shared_ptr<LuaValue::Function> func,
    std::vector< std::shared_ptr<Value> > & reversed_args,
    int return_size
) {
    #ifdef INTERPRETER_DEBUG
    std::cout << "[raw_lua_call]" << std::endl;
    size_t i = 0;
    for (auto it = reversed_args.rbegin(); it != reversed_args.rend(); it++) {
        std::cout << i << ": " << *it << " " << type_of(*it) << std::endl;
        i++;
    }
    std::cout << std::endl;
    #endif

    if (func->func) {
        throw std::runtime_error("Calling raw_lua_call on Cxx function");
    } 

    callstack.push(func->label);
    to_return.push(return_size);
    // stacks.push({});
    scopes.push( { Scope{} } );
    ret_addr.push(ip);
    try {
        ip = g->labels.at(func->label);
    } catch (std::out_of_range &e) {
        throw std::runtime_error("Cannot find label " + func->label);
    }

    int argsize = reversed_args.size();
    for (int i=0; i< (int)func->arg_N; i++) {
        // argsize - i - 1
        int idx = argsize - i - 1;
        if (idx < 0) {
            stacks.top().push(
                std::shared_ptr<Value>( new LuaValue::Nil )
            );
        } else {
            stacks.top().push(
                reversed_args[idx]
            );
        }
    }
    auto varg = std::make_shared<LuaValue::Table>();
    if (argsize > (int)func->arg_N) {
        int diff = argsize - func->arg_N;
        for (int i=0; i<diff; i++) {
            // argsize - N - i - 1
            int idx = diff - i - 1;
            varg->set(i, reversed_args[idx]);
        }
    }
    scopes.top().back().set(func->varg, varg);
}

void Executioner::raw_call(
    std::shared_ptr<LuaValue::Function> func,
    std::vector< std::shared_ptr<Value> > & reversed_args,
    int return_size
) {
    #ifdef INTERPRETER_DEBUG
    std::cout << "[" << std::endl;
    std::cout << "raw_call" << std::endl;
    size_t i = 0;
    for (auto it = reversed_args.rbegin(); it != reversed_args.rend(); it++) {
        std::cout << i << ": " << *it << " " << type_of(*it) << std::endl;
        i++;
    }
    std::cout << "]" << std::endl;
    #endif

    if (func->func) {
        // dont put pcall on callstack
        if (g->cxx_funcnames.contains(func->func)) {
            callstack.push( g->cxx_funcnames.at(func->func) );
        } else {
            callstack.push( func->key() );
        }

        // built-in function
        std::reverse(reversed_args.begin(), reversed_args.end());
        auto result = func->func(this, reversed_args);

        if (result.empty()) {
            // for coroutine & pcall compliance
            // functions that dont put anything on stack right away
            if (
                func->func != &LuaLibs::Module::require     &&
                func->func != &LuaLibs::Error::pcall        &&
                func->func != &LuaLibs::Coroutine::resume   &&
                func->func != &LuaLibs::Coroutine::yield    &&
                true
            ) {
                 result.push_back( std::make_shared<LuaValue::Nil>() );
            }
        }
        if (return_size == ALL) {
            for (auto &r: result) {
                stacks.top().push(r);
            }
        } else {
            int size = result.size();
            for (int i=0; i<return_size; i++) {
                if (i >= size) {
                    stacks.top().push(
                        std::shared_ptr<Value> (new LuaValue::Nil)
                    );
                } else {
                    stacks.top().push(result[i]);
                }
            }
        }

        if (
            // require && result.empty => skip
            // require, but result is not empty => pop
            (func->func != &LuaLibs::Module::require || !result.empty()) &&
            func->func != &LuaLibs::Error::pcall    &&
            true
        ) {
            callstack.pop( );
        }
    } else {
        raw_lua_call(func, reversed_args, return_size);
    }
}

void Executioner::CALL(Instruction *inst) {
    auto func_arg = pop_top();

    auto pair = to_function(func_arg, ALL);
    if (!pair.first) throw std::runtime_error("Interpreter: Cannot call non-callable object");
    std::shared_ptr<LuaValue::Function> function = pair.first; 

    auto &top = stacks.top();

    // Arg_N ... Arg_1
    std::vector< std::shared_ptr<Value> > values;
    if (inst->whole_stack) {
        while (!top.empty()) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            }  
            top.pop();
        }
    }
    else {
        for (size_t i=0; i<inst->N; i++) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            } 
            top.pop();
        }
    }

    // method
    if (pair.second) values.push_back(pair.second);

    if (inst->any_output) raw_call(function, values, ALL);
    else raw_call(function, values, inst->M);
}

void Executioner::REV_CALL(Instruction *inst) {
    auto &top = stacks.top();

    // Arg_N ... Arg_1 func
    std::vector< std::shared_ptr<Value> > values;
    if (inst->whole_stack) {
        while (!top.empty()) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            }  
            top.pop();
        }
    }
    else {
        for (size_t i=0; i<inst->N; i++) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            } 
            top.pop();
        }
    }

    auto func_arg = values.back();
    values.pop_back();

    auto pair = to_function(func_arg, ALL);
    if (!pair.first) throw std::runtime_error("Interpreter: Cannot call non-callable object");
    std::shared_ptr<LuaValue::Function> function = pair.first; 

    // method
    if (pair.second) values.push_back(pair.second);

    if (inst->any_output) raw_call(function, values, ALL);
    else raw_call(function, values, inst->M);
}

void Executioner::RET(Instruction *inst) {
    // v_n
    // ...
    // v_1
    auto top = stacks.top();
    stacks.pop();
    

    // v_n ... v_1
    std::vector< std::shared_ptr<Value> > values;
    
    if (inst->whole_stack) {
        while (!top.empty()) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            }  
            top.pop();
        }
    } else {
        for (size_t i=0; i<inst->N; i++) {
            if (!is_barrier(top)) {
                values.push_back(top.top());
            } 
            top.pop();
        }
    }

    

    if (to_return.empty()) {
        // end of given function
        rets = values;
        running = false;
        stop = true;
        
        release_turn();
        return;
    }
    callstack.pop();
    if (g->cxx_funcnames.contains( &LuaLibs::Error::pcall )) {
        auto pcall_name = g->cxx_funcnames.at(&LuaLibs::Error::pcall);
        if (callstack.top() == pcall_name) {
            // return out of pcall

            callstack.pop();

            // success
            values.push_back( std::make_shared<LuaValue::Boolean>(true) );
        }
    }
    if (g->cxx_funcnames.contains( &LuaLibs::Module::require )) {
        auto requre_name = g->cxx_funcnames.at(&LuaLibs::Module::require);
        if (callstack.top() == requre_name) {
            // return out of require

            callstack.pop();
            // first return - return from module
            auto module_ = values.back();
            auto modname = g->loading_modules.top(); g->loading_modules.pop();
            g->loaded_modules.insert( { modname, module_ } );
        }
    }
    scopes.pop();
    ip = ret_addr.top(); ret_addr.pop();

    // v_1 ... v_n
    std::reverse(values.begin(), values.end());

    int tr = to_return.top(); to_return.pop();
    if (tr == ALL) {
        for (auto &v: values) {
            stacks.top().push(v);
        }
    } else {
        for (int i=0; i<tr; i++) {
            stacks.top().push(values[i]);
        }
    }

    stop = true;
}

void Executioner::PUT_NIL(Instruction *inst) {
    stacks.top().push(
        std::shared_ptr<Value>( new LuaValue::Nil )
    );
}

void Executioner::PUT_TRUE(Instruction *inst) {
    stacks.top().push(
        std::shared_ptr<Value>( new LuaValue::Boolean(true) )
    );
}

void Executioner::PUT_FALSE(Instruction *inst) {
    stacks.top().push(
        std::shared_ptr<Value>( new LuaValue::Boolean(false) )
    );
}

void Executioner::PUT_STR(Instruction *inst) {
    stacks.top().push(
        std::make_shared<LuaValue::String>(inst->str)
    );
}

void Executioner::PUT_NUM(Instruction *inst) {
    std::shared_ptr<Value> val;
    if (std::holds_alternative<int64_t> (inst->num)) {
        val = std::shared_ptr<Value>( new LuaValue::Number(std::get<int64_t>(inst->num)) );
    } else {
        val = std::shared_ptr<Value>( new LuaValue::Number(std::get<std::float64_t>(inst->num)) );
    }

    stacks.top().push(val);
}

void Executioner::PUT_TABLE(Instruction *inst) {
    stacks.top().push(
        std::shared_ptr<Value>( new LuaValue::Table )
    );
}

void Executioner::PUT_FUNC(Instruction *inst) {
    stacks.top().push(
        std::shared_ptr<Value>( new LuaValue::Function(inst->label, inst->N, inst->name) )
    );
}

void Executioner::PUT_BARRIER(Instruction *inst) {
    stacks.top().push(
        std::shared_ptr<Value>( new LuaValue::Barrier )
    );
}

void Executioner::POP_BARRIER(Instruction *inst) {
    auto &top = stacks.top();
    while (!is_barrier(top)) {
        std::cout << "WARNING: POP_BARRIER had to pop a value!" << std::endl;
        top.pop();
    }
    top.pop();
}

void Executioner::DISCARD(Instruction *inst) {
    stacks.top().pop();
}

void Executioner::DUP(Instruction *inst) {
    auto val = pop_top();
    stacks.top().push(val);
    stacks.top().push(val);
}

void Executioner::LE(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary conversion not supported yet");
    }

    stacks.top().push(
        std::shared_ptr<Value> ( new LuaValue::Boolean(*num1 <= *num2) )
    );
} 

void Executioner::LT(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary conversion not supported yet");
    }

    stacks.top().push(
        std::shared_ptr<Value> ( new LuaValue::Boolean(*num1 < *num2) )
    );
}

void Executioner::GE(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary conversion not supported yet");
    }

    stacks.top().push(
        std::shared_ptr<Value> ( new LuaValue::Boolean(*num1 >= *num2) )
    );
} 
void Executioner::GT(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary conversion not supported yet");
    }

    stacks.top().push(
        std::shared_ptr<Value> ( new LuaValue::Boolean(*num1 > *num2) )
    );
}

void Executioner::EQ(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    stacks.top().push(
        std::make_shared<LuaValue::Boolean>( are_equal(arg1, arg2) )
    );
} 

void Executioner::NEQ(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    stacks.top().push(
        std::make_shared<LuaValue::Boolean>( !are_equal(arg1, arg2) )
    );    
}

void Executioner::CONCAT(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    std::shared_ptr<LuaValue::String> str1 = nullptr;
    std::shared_ptr<LuaValue::String> str2 = nullptr;

    switch (arg1->type()) {
        case Value::Type::STRING: {
            str1 = std::static_pointer_cast<LuaValue::String>(arg1);
        } break;
        case Value::Type::NUMBER: {
            auto num = std::static_pointer_cast<LuaValue::Number>(arg1);
            if (num->kind == LuaValue::Number::Kind::INT) {
                str1 = std::make_shared<LuaValue::String>( std::to_string(num->integer) );
            } else {
                str1 = std::make_shared<LuaValue::String>( std::to_string((double) num->floating) );
            }
        } break;
        case Value::Type::BOOLEAN: {
            auto bl = std::static_pointer_cast<LuaValue::Boolean>(arg1);
            if (bl->value) {
                str1 = std::make_shared<LuaValue::String>( "true" );
            } else {
                str1 = std::make_shared<LuaValue::String>( "false" );
            }
        } break;
        case Value::Type::NIL: {
            str1 = std::make_shared<LuaValue::String>( "nil" );
        } break;

        default: break;
    }

    switch (arg2->type()) {
        case Value::Type::STRING: {
            str2 = std::static_pointer_cast<LuaValue::String>(arg2);
        } break;
        case Value::Type::NUMBER: {
            auto num = std::static_pointer_cast<LuaValue::Number>(arg2);
            if (num->kind == LuaValue::Number::Kind::INT) {
                str2 = std::make_shared<LuaValue::String>( std::to_string(num->integer) );
            } else {
                str2 = std::make_shared<LuaValue::String>( std::to_string((double) num->floating) );
            }
        } break;
        case Value::Type::BOOLEAN: {
            auto bl = std::static_pointer_cast<LuaValue::Boolean>(arg1);
            if (bl->value) {
                str2 = std::make_shared<LuaValue::String>( "true" );
            } else {
                str2 = std::make_shared<LuaValue::String>( "false" );
            }
        } break;
        case Value::Type::NIL: {
            str2 = std::make_shared<LuaValue::String>( "nil" );
        } break;

        default: break;
    }
    
    if (!str1 || !str2) {
        throw std::runtime_error("Interpreter: Arbitary concat not supported yet");
    }

    stacks.top().push(
        std::make_shared<LuaValue::String>( str1->value + str2->value )
    ); 
}

void Executioner::ADD(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary addition is not supported yet");
    }

    stacks.top().push(
        std::make_shared<LuaValue::Number> (*num1 + *num2)
    );
} 

void Executioner::SUB(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary subtraction is not supported yet");
    }

    stacks.top().push(
        std::make_shared<LuaValue::Number> (*num1 - *num2)
    );
}

void Executioner::MUL(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary multiplication is not supported yet");
    }

    stacks.top().push(
        std::make_shared<LuaValue::Number> (*num1 * *num2)
    );
} 

void Executioner::MOD(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary modulus is not supported yet");
    }
    if (num1->kind != LuaValue::Number::Kind::INT || num2->kind != LuaValue::Number::Kind::INT) {
        throw std::runtime_error("Interpreter: Modulus only for integers");
    }

    stacks.top().push(
        std::make_shared<LuaValue::Number> (num1->integer % num2->integer)
    );
}

void Executioner::DIV(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary division is not supported yet");
    }
    if (num1->kind != LuaValue::Number::Kind::INT || num2->kind != LuaValue::Number::Kind::INT) {
        throw std::runtime_error("Interpreter: integer division only for integers");
    }

    stacks.top().push(
        std::make_shared<LuaValue::Number> (num1->integer / num2->integer)
    );
} 

void Executioner::TRUEDIV(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto num1 = to_num(arg1);
    auto num2 = to_num(arg2);

    if (!num1 || !num2) {
        throw std::runtime_error("Interpreter: Arbitary division is not supported yet");
    }

    stacks.top().push(
        std::make_shared<LuaValue::Number> (*num1 / *num2)
    );
}

void Executioner::SHLEFT(Instruction *inst) {
    throw std::runtime_error("Interpreter: Shift operations are not supported yet");
} 
void Executioner::SHRIGHT(Instruction *inst) {
    throw std::runtime_error("Interpreter: Shift operations are not supported yet");
}

void Executioner::AND(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto b1 = to_bool(arg1);
    auto b2 = to_bool(arg2);

    stacks.top().push(
        std::make_shared<LuaValue::Boolean> (b1->value && b2->value)
    );
} 

void Executioner::OR(Instruction *inst) {
    auto arg2 = pop_top();
    auto arg1 = pop_top();

    auto b1 = to_bool(arg1);
    auto b2 = to_bool(arg2);

    stacks.top().push(
        std::make_shared<LuaValue::Boolean> (b1->value || b2->value)
    );
}

void Executioner::BITAND(Instruction *inst) {
    throw std::runtime_error("Interpreter: Bit operations are not supported yet");
} 
void Executioner::BITOR(Instruction *inst) {
    throw std::runtime_error("Interpreter: Bit operations are not supported yet");
} 
void Executioner::BITXOR(Instruction *inst) {
    throw std::runtime_error("Interpreter: Bit operations are not supported yet");
}
void Executioner::POW(Instruction *inst) {
    throw std::runtime_error("Interpreter: Pow operation is not supported yet");
}

void Executioner::HASH(Instruction *inst) {
    auto arg = pop_top();

    size_t result = 0;
    if (arg->type() == Value::Type::STRING) {
        result = std::static_pointer_cast<LuaValue::String>(arg)->value.size();
    } else
    if (arg->type() == Value::Type::TABLE) {
        auto table = std::static_pointer_cast<LuaValue::Table>(arg);
        while (table->int_table.contains(result)) result++;
    } else {
        throw std::runtime_error("Interpreter: Cannot perform hash operation on arbitary value");
    }

    stacks.top().push(
        std::make_shared<LuaValue::Number>((int64_t)result)
    );
}

void Executioner::NEG(Instruction *inst) {
    auto arg1 = pop_top();

    auto n1 = to_num(arg1);

    if (!n1) {
        throw std::runtime_error("Interpreter: Arbitary negation is not supported yet");
    }

    if (n1->kind == LuaValue::Number::Kind::INT) {
        stacks.top().push(
            std::make_shared<LuaValue::Number>(-n1->integer)
        );
    } else {
        stacks.top().push(
            std::make_shared<LuaValue::Number>(-n1->floating)
        );
    }
} 

void Executioner::NOT(Instruction *inst) {
    auto arg1 = pop_top();

    auto b1 = to_bool(arg1);
    b1->value = !b1->value;

    stacks.top().push(b1);
}

void Executioner::BITNOT(Instruction *inst) {
    throw std::runtime_error("Interpreter: Bit operations are not supported yet");
}