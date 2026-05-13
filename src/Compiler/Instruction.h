#ifndef COMPILER_INTRUCTION_H
#define COMPILER_INTRUCTION_H

#include <string>
#include <variant>
#include <fstream>
#include <cstdint>
#include <stdfloat>

struct Instruction {
    enum class Type {
        NOP = 0,

        PUT_SCOPE,
        POP_SCOPE,

        PUT_STACK,
        POP_STACK,
        CLEAR_STACK,
        MOVE_STACK,

        LOAD,
        STORE,
        SET_LOCAL,
        SET_GLOBAL,
        SET_ATTR,
        GLOBATTR,

        INDEX, DYN_INDEX,
        METAINDEX,

        METAASSIGN_WHAT_WHOM,

        ASSIGN_WHAT_WHOM,
        ASSIGN_WHOM_WHAT,
        ASSIGN_WHAT_WHOM_WHERE,
        ASSIGN_WHOM_WHERE_WHAT,
        
        BRANCH,
        GOTO,
        LABEL,

        CALL,
        REV_CALL,
        RET,

        PUT_NIL,
        PUT_TRUE,
        PUT_FALSE,
        PUT_STR,
        PUT_NUM,
        PUT_TABLE,
        PUT_FUNC,

        PUT_BARRIER,
        POP_BARRIER,

        DISCARD,
        DUP,

        LE, LT,
        GE, GT,
        EQ, NEQ,
        CONCAT,
        ADD, SUB,
        MUL, MOD,
        DIV, TRUEDIV,
        SHLEFT, SHRIGHT,
        AND, OR,
        BITAND, BITOR, BITXOR,
        POW,

        // unary
        HASH, NEG, NOT, BITNOT
    } type;

    
    std::string name;

    std::string var;
    std::string attr;

    std::string field;
    std::string metafield;

    std::string label;

    bool reverse_stack = false;
    // input size
    bool whole_stack = false;
    size_t N;

    // output size
    bool any_output = false;
    size_t M;

    std::string str;

    std::variant<std::int64_t, std::float64_t> num;
};

std::ostream& operator<<(std::ostream& os, Instruction& inst);

#endif // COMPILER_INTRUCTION_H