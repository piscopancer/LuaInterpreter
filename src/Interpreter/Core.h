#ifndef INTERPRETER_CORE_H
#define INTERPRETER_CORE_H

#include "Base.h"
#include "Value.h"
#include "Compiler/Instruction.h"

#include <unordered_map>
#include <memory>
#include <stack>
#include <algorithm>

// libs
#include "libs/io/io.h"
#include "libs/iterators/iterators.h"
#include "libs/iterators/map.h"
#include "libs/typing/type.h"
#include "libs/error/error.h"
#include "libs/math/math.h"
#include "libs/string/string.h"
#include "libs/coroutine/coroutine.h"
#include "libs/module/module.h"

namespace LuaInterpreter {

struct Scope {
    std::unordered_map< std::string, std::shared_ptr<Value> > locals;

    std::shared_ptr<Value> get(const std::string &name);
    void set(const std::string &name, std::shared_ptr<Value> value);
};

class Interpreter {
public:
    size_t next_tid = 0;

    Scope global;
    std::shared_ptr<Value> get(const std::string &name);
    void set(const std::string &name, std::shared_ptr<Value> value);

    std::vector<Instruction> program;

    std::vector< std::unique_ptr< Executioner > > workers;

    std::unordered_map< cxx_func, std::string > cxx_funcnames;

    std::unordered_map< std::string, std::shared_ptr<Value> > loaded_modules;
    std::stack< std::string > loading_modules;

    std::unordered_map< std::string, size_t > labels;
    std::unordered_map< std::string, std::pair<int, std::string> > func_args;
    
    void collect_labels(const std::vector< Instruction >& program, size_t start=0);

    Interpreter(
        const std::vector< Instruction >& program
    );

    bool run();
};

struct Executioner {
public:
    Interpreter *g; 
    size_t tid;
    size_t ip;
    std::stack < size_t > ret_addr;

    // 0 - begining
    std::stack <
        std::vector< Scope > 
    > scopes;
    std::shared_ptr<Value> get(const std::string &name);
    void set(const std::string &name, std::shared_ptr<Value> value);

    std::stack <
        std::stack < std::shared_ptr<Value> >
    > stacks;
    
    std::stack < std::string > callstack;
    std::stack < int > to_return;
    
    static constexpr int ALL = -1;

    bool running = true;
    bool suspended_coroutine = false;

    struct Catch {
        size_t ip;

        size_t ret_addr_size;
        size_t scopes_size;
        size_t stacks_size;
        size_t callstack_size;
        size_t to_return_size;
    };
    std::stack< Catch > pcalls;

    bool stop = false;
    std::shared_ptr< LuaValue::Thread > tied_to = nullptr;

    std::vector< std::shared_ptr<Value> > rets;

    Executioner(
        Interpreter *g,
        size_t tid,
        std::shared_ptr<LuaValue::Function> entry,
        std::vector< std::shared_ptr<Value> > args
    );

    void execute();

    void release_turn();

    Instruction* fetch_instruction();

    void execute(Instruction* inst);

    static std::string type_of(std::shared_ptr< Value > value);

    // Others: switch value->type()
    // Userdata: metatable[ "__type" ] = type
    static bool is_type_of(std::shared_ptr< Value > value, const std::string& type);

    void raw_lua_call(
        std::shared_ptr<LuaValue::Function> func,
        std::vector< std::shared_ptr<Value> > & reversed_args,
        int return_size
    );

    void raw_call(
        std::shared_ptr<LuaValue::Function> func,
        std::vector< std::shared_ptr<Value> > & reversed_args,
        int return_size
    );
// private:
    bool is_barrier(const std::stack< std::shared_ptr<Value> > st) const;

    std::shared_ptr< Value > pop_top();

    std::pair<
        std::shared_ptr<LuaValue::Function>,    // function
        std::shared_ptr<Value>   // reference
    > to_function(
        std::shared_ptr<Value> func_arg,
        int max_recursion
    );

    std::shared_ptr< LuaValue::Boolean > to_bool(std::shared_ptr< Value > value);

    std::shared_ptr< LuaValue::Number > to_num(std::shared_ptr< Value > value);
    std::shared_ptr< LuaValue::Number > to_int(std::shared_ptr< Value > value);
    std::shared_ptr< LuaValue::Number > to_float(std::shared_ptr< Value > value);

    bool are_equal(std::shared_ptr<Value> arg1, std::shared_ptr<Value> arg2);

    void NOP(Instruction *inst);

    void PUT_SCOPE(Instruction *inst);
    
    void POP_SCOPE(Instruction *inst);

    void PUT_STACK(Instruction *inst);

    void POP_STACK(Instruction *inst);

    void CLEAR_STACK(Instruction *inst);
    
    void MOVE_STACK(Instruction *inst);
    
    void LOAD(Instruction *inst);

    void STORE(Instruction *inst);
    
    void SET_LOCAL(Instruction *inst);

    void SET_GLOBAL(Instruction *inst);

    void SET_ATTR(Instruction *inst);
    
    void GLOBATTR(Instruction *inst);
    
    void INDEX(Instruction *inst);

    void DYN_INDEX(Instruction *inst);

    void METAINDEX(Instruction *inst);
    void METAASSIGN_WHAT_WHOM(Instruction *inst);

    void ASSIGN_WHAT_WHOM(Instruction *inst);
    void ASSIGN_WHOM_WHAT(Instruction *inst);

    void ASSIGN_WHAT_WHOM_WHERE(Instruction *inst);

    void ASSIGN_WHOM_WHERE_WHAT(Instruction *inst);

    void BRANCH(Instruction *inst);

    void GOTO(Instruction *inst);
    void LABEL(Instruction *inst);

    void CALL(Instruction *inst);

    void REV_CALL(Instruction *inst);

    void RET(Instruction *inst);

    void PUT_NIL(Instruction *inst);

    void PUT_TRUE(Instruction *inst);
    
    void PUT_FALSE(Instruction *inst);

    void PUT_STR(Instruction *inst);

    void PUT_NUM(Instruction *inst);

    void PUT_TABLE(Instruction *inst);

    void PUT_FUNC(Instruction *inst);

    void PUT_BARRIER(Instruction *inst);

    void POP_BARRIER(Instruction *inst);

    void DISCARD(Instruction *inst);
    
    void DUP(Instruction *inst);

    void LE(Instruction *inst);

    void LT(Instruction *inst);

    void GE(Instruction *inst);
    void GT(Instruction *inst);

    void EQ(Instruction *inst);
    void NEQ(Instruction *inst);

    void CONCAT(Instruction *inst);

    void ADD(Instruction *inst);

    void SUB(Instruction *inst);

    void MUL(Instruction *inst);

    void MOD(Instruction *inst);

    void DIV(Instruction *inst);

    void TRUEDIV(Instruction *inst);

    void SHLEFT(Instruction *inst);
    void SHRIGHT(Instruction *inst);

    void AND(Instruction *inst);

    void OR(Instruction *inst);
    
    void BITAND(Instruction *inst);
    void BITOR(Instruction *inst);
    void BITXOR(Instruction *inst);
    void POW(Instruction *inst);

    void HASH(Instruction *inst);
    
    void NEG(Instruction *inst);
    
    void NOT(Instruction *inst);

    void BITNOT(Instruction *inst);
};




} // LuaInterpreter

#endif // INTERPRETER_CORE_H