#ifndef COMPILER_COMPILER_H
#define COMPILER_COMPILER_H

#include "AST.h"
#include "Instruction.h"
#include "Static.h"

#include <sstream>
#include <stack>
#include <queue>
#include <iostream>


class Compiler {
    size_t uid = 0;
    size_t get_uid();
    std::string get_ustr();

    std::stack< std::string > break_labels;
    std::stack< size_t > scopes_put;

    std::vector< std::string > prefixes;
    std::string get_prefix();

    public:
    struct FunctionToCompile {
        std::string funcname;
        LuaAST::FuncBody* body;
        bool is_method = false;
    };
    private:
    
    std::queue< FunctionToCompile > functions_to_compile;

    StaticAnalyzer statan;
public:
    Compiler(std::shared_ptr< LuaAST::Block > ast);

    std::vector< Instruction > compile(
        std::shared_ptr<LuaAST::Block> block,
        const std::string& entry_name = "_start",
        const std::string& initial_prefix = ""
    );

    std::vector< Instruction > compile_function(FunctionToCompile &func);

    std::vector< Instruction > compile_block(LuaAST::Block *block);

    std::vector< Instruction > compile_statement(LuaAST::Statement *statement);

    std::vector< Instruction > compile_expression(
        LuaAST::Expression *expression,
        int expect_stack_size, // -1 for any size
        bool reversed = false
    );
};

#endif // COMPILER_COMPILER_H