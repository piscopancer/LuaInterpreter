#include "Compiler.h"

std::string unescapeString(const std::string& input) {
    static const std::unordered_map<char, char> escapeChars = {
        {'n', '\n'},
        {'t', '\t'},
        {'r', '\r'},
        {'e', '\e'},
        {'\\', '\\'},
        {'\'', '\''},
        {'\"', '\"'},
        {'0', '\0'}
    };
    
    std::string result;
    result.reserve(input.size());
    
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[i + 1];
            auto it = escapeChars.find(next);
            if (it != escapeChars.end()) {
                result.push_back(it->second);
                ++i;
            } else {
                result.push_back(input[i]);
                result.push_back(next);
                ++i;
            }
        } else {
            result.push_back(input[i]);
        }
    }
    
    return result;
}

size_t Compiler::get_uid() { return uid++; }
std::string Compiler::get_ustr() { 
    return "uid_" + std::to_string(get_uid()) + "_"; 
}

std::string Compiler::get_prefix() { 
    std::stringstream ss;
    ss << "";
    for (auto &str: prefixes) {
        ss << str << "_";
    }
    return ss.str();
}

Compiler::Compiler(std::shared_ptr< LuaAST::Block > ast): statan(ast) {
}

std::vector< Instruction > Compiler::compile(
    std::shared_ptr<LuaAST::Block> block,
    const std::string& entry_name,
    const std::string& initial_prefix
) {
    if (!initial_prefix.empty()) prefixes.push_back(initial_prefix);

    LuaAST::FuncBody entry;
    entry.block = block;
    functions_to_compile.push( {
        .funcname = entry_name, 
        .body = &entry,
    } );

    std::vector<Instruction> result;

    while (!functions_to_compile.empty()) {
        auto func = functions_to_compile.front();
        functions_to_compile.pop();

        auto compiled = compile_function(func);
        result.insert(result.end(), compiled.begin(), compiled.end());
    }

    return result;
}

std::string Compiler::get_func_label(const std::string& funcname) {
    return get_prefix() + funcname;
}

std::vector< Instruction > Compiler::compile_function(FunctionToCompile &func) {
    // считаем, что CALL в runtime всё подготовит сам:
    // создаст новый стек
    // переложит все аргументы на стек
    // если мало => заполнит до N с nil
    // если много => переложит в varg

    std::vector< Instruction > result;

    break_labels = std::stack< std::string >();
    scopes_put = std::stack< size_t > (); scopes_put.push(0);
    std::string funclabel = get_func_label(func.funcname);

    result.push_back( 
        Instruction { 
            .type = Instruction::Type::LABEL,
            .label = funclabel,
        } 
    );

    prefixes.push_back(func.funcname);

    // std::cout << "point 2" << std::endl;
    size_t N = func.body->params.size();

    // result.push_back( Instruction {.type = Instruction::Type::PUT_SCOPE} );
    // scopes_put.top()++;
    // std::cout << "point 3" << std::endl;

    if (N > 0) {
        for (int i = N-1; i >= 0; i--) {
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = func.body->params[i]
                } 
            );
        }
    }
    if (func.is_method) {
        result.push_back( 
            Instruction { 
                .type = Instruction::Type::SET_LOCAL,
                .name = "self",
            }
        );
    }
    // std::cout << "point 4" << std::endl;

    auto block = compile_block(func.body->block.get());
    result.insert(result.end(), block.begin(), block.end());

    // result.push_back( Instruction {.type = Instruction::Type::POP_SCOPE} );
    // scopes_put.top()--;

    // на случай, если нет return
    result.push_back( Instruction {.type = Instruction::Type::PUT_STACK} );
    result.push_back( Instruction {.type = Instruction::Type::PUT_NIL} );
    result.push_back( Instruction {.type = Instruction::Type::RET, .N = 1} );

    prefixes.pop_back();
    return result;
}

std::vector< Instruction > Compiler::compile_block(LuaAST::Block *block) {

    std::vector< Instruction > result;
    for (auto st: block->statements) {
        auto compiled = compile_statement(st.get());
        result.insert(result.end(), compiled.begin(), compiled.end());
    }
    return result;
}

std::vector< Instruction > Compiler::compile_statement(LuaAST::Statement *statement) {
    std::vector< Instruction > result;


    switch (statement->type()) {
        case LuaAST::Statement::Type::DO_BLOCK: {
            LuaAST::DoBlockSt* st = (LuaAST::DoBlockSt*) statement;
            // DO_BLOCK,        
            /*
                PUT_SCOPE
                [Block]
                POP_SCOPE
            */
            result.push_back( Instruction {.type = Instruction::Type::PUT_SCOPE} );
            scopes_put.top()++;
            
            auto block = compile_block(st->block.get());
            result.insert(result.end(), block.begin(), block.end());

            result.push_back( Instruction {.type = Instruction::Type::POP_SCOPE} );
            scopes_put.top()--;
        } break;

        case LuaAST::Statement::Type::ASSIGN: {
            LuaAST::AssignSt* st = (LuaAST::AssignSt*) statement;
            // ASSIGN,
            // var = exp
            /*

            */
            // var.field1.field2....fieldn = exp 
            /*
                [Eval exp]
                LOAD var
                INDEX field1
                ...
                INDEX field_n-1
                ASSIGN_WHAT_WHOM fieldn
            */

            // v1, v2, .. vn = e1, e2, ..., em
            /*
                PUT_BARRIER
                [Eval em]
                ...
                [Eval e1]

                [Preindex v1]
                ASSIGN_WHAT_WHOM
                
                ... 

                [Preindex vn]
                ASSIGN_WHAT_WHOM
                
                POP_BARRIER
            */
            size_t left_N = st->lhs.size();
            size_t right_N = st->rhs.size();

            if (right_N > left_N) {
                std::cerr << "Assignment: RHS is longer than LHS" << std::endl;
                st->print(std::cerr);
                throw std::runtime_error("Compilation: Assignment");
            }

            result.push_back(Instruction { .type = Instruction::Type::PUT_BARRIER });

            for (int i=right_N-1; i>=0; i--) {
                auto exp = compile_expression(st->rhs[i].get(), -1, true);
                result.insert(result.end(), exp.begin(), exp.end());
            }

            for (size_t i=0; i<left_N; i++) {
                auto var = st->lhs[i].get();
                if (var->specifications.size() == 0) {
                    if (var->base->kind != LuaAST::VarPart::Kind::NAME) {
                        std::cerr << "Assignment: cannot assign to expression" << std::endl;
                        st->print(std::cerr);
                        throw std::runtime_error("Compilation: Assignment");
                    }
                    LuaAST::VarPartName* varname = (LuaAST::VarPartName*) var->base.get();
                    result.push_back(
                        Instruction { .type = Instruction::Type::STORE, .name = varname->name }
                    );
                } else {
                    LuaAST::Var copy = *var;
                    copy.specifications.pop_back();

                    // preindex
                    auto exp = compile_expression(&copy, 1);
                    result.insert(result.end(), exp.begin(), exp.end());

                    auto last = var->specifications.back().get();
                    if (last->kind == LuaAST::VarPart::Kind::NAME) {
                        result.push_back(
                            Instruction { 
                                .type = Instruction::Type::ASSIGN_WHAT_WHOM, 
                                .field = ((LuaAST::VarPartName*)last)->name 
                            }
                        );   
                    } else {
                        LuaAST::VarPartExp* varexp = (LuaAST::VarPartExp*) last;
                        exp = compile_expression(varexp->exp.get(), 1);
                        result.insert(result.end(), exp.begin(), exp.end());
                        result.push_back(
                            Instruction { 
                                .type = Instruction::Type::ASSIGN_WHAT_WHOM_WHERE, 
                            }
                        );
                    }
                }
            }

            result.push_back(Instruction { .type = Instruction::Type::POP_BARRIER });
        } break;

        case LuaAST::Statement::Type::DECLARE: {
            LuaAST::DeclareSt* st = (LuaAST::DeclareSt*)statement;
            // DECLARE,
            // local common_attr var1 attr1, ... varn attrn = exp1, ... expm
            /*
                PUT_BARRIER
                [Eval expm]
                ...
                [Eval exp1]

                SET_LOCAL var1
                SET_ATTR var1 attr1
                SET_ATTR var1 common_attr
                ...
                SET_LOCAL varn
                SET_ATTR varn attrn
                SET_ATTR varn common_attr

                POP_BARRIER
            */
            
            size_t left_N = st->lhs.size();
            size_t right_N = st->rhs.size();

            result.push_back(Instruction { .type = Instruction::Type::PUT_BARRIER });

            for (int i=right_N-1; i>=0; i--) {
                auto exp = compile_expression(st->rhs[i].get(), -1, true);
                result.insert(result.end(), exp.begin(), exp.end());
            }

            for (size_t i = 0; i < left_N; i++) {
                auto pair = st->lhs[i];
                if (st->scope == LuaAST::ScopeSpec::LOCAL) {
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::SET_LOCAL,
                            .name = pair.first
                        }
                    );
                } else {
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::SET_GLOBAL,
                            .name = pair.first
                        }
                    );
                }

                if (pair.second) {
                    if (pair.second->kind == LuaAST::Attribute::Kind::CONST) {
                        result.push_back(
                            Instruction {
                                .type = Instruction::Type::SET_ATTR,
                                .var = pair.first,
                                .attr = "const",
                            }
                        );
                    } else {
                        std::cerr << "Unsupported attribute " << (int)pair.second->kind << std::endl;
                        st->print(std::cerr);
                        throw std::runtime_error("Compilation: Declaration");
                    }
                }

                if (st->common_attr) {
                    if (st->common_attr->kind == LuaAST::Attribute::Kind::CONST) {
                        result.push_back(
                            Instruction {
                                .type = Instruction::Type::SET_ATTR,
                                .var = pair.first,
                                .attr = "const",
                            }
                        );
                    } else {
                        std::cerr << "Unsupported attribute " << (int)st->common_attr->kind << std::endl;
                        st->print(std::cerr);
                        throw std::runtime_error("Compilation: Declaration");
                    }
                }
            }
            result.push_back(Instruction { .type = Instruction::Type::POP_BARRIER });
        } break;

        case LuaAST::Statement::Type::ATTRIB: {
            LuaAST::AttribSt* st = (LuaAST::AttribSt*) statement;
            // ATTRIB,
            /*
                GLOBATR attribute
            */
            if (st->attr->kind == LuaAST::Attribute::Kind::CONST) {
                result.push_back(
                    Instruction {
                        .type = Instruction::Type::GLOBATTR,
                        .attr = "const",
                    }
                );
            }
        } break;

        case LuaAST::Statement::Type::FUNCDEF: {
            LuaAST::FuncdefSt* st = (LuaAST::FuncdefSt*)statement;
            if (st->kind == LuaAST::FuncdefSt::Kind::DEFAULT) {
                LuaAST::DefaultFuncdefSt* def = (LuaAST::DefaultFuncdefSt*)st;
                auto name = def->name.get();
                
                std::string funcname = get_prefix() + get_ustr() + name->base;
                for (auto& spec: name->specifications) {
                    funcname += "." + spec;
                }
                if (!name->kind.empty()) funcname += ":" + name->kind;
                
                functions_to_compile.push( { 
                    .funcname = funcname, 
                    .body = st->body.get(),
                    .is_method = !name->kind.empty(),
                } );

                result.push_back(
                    Instruction {
                        .type = Instruction::Type::PUT_FUNC,
                        .name = st->body->variadic_param.value_or("varg"),
                        .label = funcname,
                        .N = st->body->params.size() + !name->kind.empty(),
                    }
                );

                // f
                if (name->specifications.empty() && name->kind.empty()) {
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::STORE,
                            .name = name->base,
                        }
                    );
                    break;
                }

                // f.x
                if (!name->specifications.empty() && name->kind.empty()) {
                    size_t N = name->specifications.size();
                    
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::LOAD,
                            .name = name->base,
                        }
                    );
                    for (size_t i=0; i<N-1; i++) {
                        result.push_back(
                            Instruction {
                                .type = Instruction::Type::INDEX,
                                .field = name->specifications[i],
                            }
                        );
                    }
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::ASSIGN_WHAT_WHOM,
                            .field = name->specifications[N-1],
                        }
                    );
                    break;
                }

                // f:met
                if (name->specifications.empty() && !name->kind.empty()) {
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::LOAD,
                            .name = name->base,
                        }
                    );

                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::METAASSIGN_WHAT_WHOM,
                            .metafield = name->kind,
                        }
                    );
                    break;
                }

                // f.x:met
                if (!name->specifications.empty() && !name->kind.empty()) {
                    size_t N = name->specifications.size();
                    
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::LOAD,
                            .name = name->base,
                        }
                    );
                    for (size_t i=0; i<N; i++) {
                        result.push_back(
                            Instruction {
                                .type = Instruction::Type::INDEX,
                                .field = name->specifications[i],
                            }
                        );
                    }

                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::METAASSIGN_WHAT_WHOM,
                            .metafield = name->kind,
                        }
                    );
                    break;
                }
            } else {
                LuaAST::ScopedFuncdefSt* sc = (LuaAST::ScopedFuncdefSt*)st;
                std::string funcname = get_prefix() + get_ustr() + sc->name;
                functions_to_compile.push( { funcname, st->body.get() } );

                result.push_back(
                    Instruction {
                        .type = Instruction::Type::PUT_FUNC,
                        .name = st->body->variadic_param.value_or("varg"),
                        .label = funcname,
                        .N = st->body->params.size(),
                    }
                );

                if (sc->scope == LuaAST::ScopeSpec::LOCAL) {
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::SET_LOCAL,
                            .name = sc->name,
                        }
                    );
                } else {
                    result.push_back(
                        Instruction {
                            .type = Instruction::Type::SET_GLOBAL,
                            .name = sc->name,
                        }
                    );
                }
            }
        } break;

        case LuaAST::Statement::Type::WHILE: {
            LuaAST::WhileSt* st = (LuaAST::WhileSt*) statement;
            // WHILE,        
            /*
            __while_start
                [Eval condition]
                NOT
                BRANCH __while_end
                
                PUT_SCOPE
                [Block]
                POP_SCOPE
                GOTO __while_start
            __while_end

            */
            std::string prefix = get_prefix();
            std::string ustr = get_ustr();
            std::string while_start = prefix + ustr + "while_start"; 
            std::string while_end = prefix + ustr + "while_end";
            break_labels.push(while_end);
            scopes_put.push(0);

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = while_start,
                } 
            );
            auto cond = compile_expression(st->cond.get(), 1);
            result.insert(result.end(), cond.begin(), cond.end());
            result.push_back( Instruction { .type = Instruction::Type::NOT } );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::BRANCH,
                    .label = while_end,
                } 
            );

            result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
            scopes_put.top()++;

            auto block = compile_block(st->block.get());
            result.insert(result.end(), block.begin(), block.end());
            
            result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
            scopes_put.top()--;

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::GOTO,
                    .label = while_start,
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = while_end,
                } 
            );

            break_labels.pop();
            scopes_put.pop();
        } break;

        case LuaAST::Statement::Type::REPEAT: {
            LuaAST::RepeatSt* st = (LuaAST::RepeatSt*) statement;
            // REPEAT,
            /* 
            __repeat_start
                [Eval condition]
                BRANCH __repeat_end
                
                PUT_SCOPE
                [Block]
                POP_SCOPE
                GOTO __repeat_start
            __repeat_end
            */

            std::string prefix = get_prefix();
            std::string ustr = get_ustr();
            std::string repeat_start = prefix + ustr + "repeat_start"; 
            std::string repeat_end = prefix + ustr + "repeat_end";
            break_labels.push(repeat_end);
            scopes_put.push(0);

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = repeat_start,
                } 
            );
            auto cond = compile_expression(st->un_cond.get(), 1);
            result.insert(result.end(), cond.begin(), cond.end());
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::BRANCH,
                    .label = repeat_end,
                } 
            );

            result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
            scopes_put.top()++;

            auto block = compile_block(st->block.get());
            result.insert(result.end(), block.begin(), block.end());
            
            result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
            scopes_put.top()--;

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::GOTO,
                    .label = repeat_start,
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = repeat_end,
                } 
            );

            break_labels.pop();
            scopes_put.pop();
        } break;

        case LuaAST::Statement::Type::IF: {
            LuaAST::IfSt* st = (LuaAST::IfSt*) statement;
            // IF,
            // if E1 then B1 elseif Ei then Bi else Bn
            /*
                [Eval E1]
                NOT
                BRANCH __skip_0
                PUT_SCOPE
                [BLOCK B1]
                POP_SCOPE
                GOTO __skip_all
            __skip_0

                [Eval Ei]
                NOT
                BRANCH __skip_i
                PUT_SCOPE
                [BLOCK Bi]
                POP_SCOPE
                GOTO __skip_all
            __skip_i

                PUT_SCOPE
                [BLOCK Bn]
                POP_SCOPE

            __skip_all
            */

            std::string prefix = get_prefix();
            std::string ustr = get_ustr();
            std::string skip = prefix + ustr + "if_skip_";

            // if
            auto exp = compile_expression( st->branch_if.first.get() , 1);
            result.insert(result.end(), exp.begin(), exp.end());
            result.push_back( Instruction { .type = Instruction::Type::NOT } );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::BRANCH,
                    .label = skip + std::to_string(0),
                } 
            );
            result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
            scopes_put.top()++;

            auto block = compile_block( st->branch_if.second.get() );
            result.insert(result.end(), block.begin(), block.end());
            
            result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
            scopes_put.top()--;

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::GOTO,
                    .label = skip + "all",
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = skip + std::to_string(0),
                } 
            );

            for (size_t i=0; i < st->branch_elseif.size(); i++) {
                auto& branch = st->branch_elseif[i];

                exp = compile_expression( branch.first.get() , 1);
                result.insert(result.end(), exp.begin(), exp.end());
                result.push_back( Instruction { .type = Instruction::Type::NOT } );
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::BRANCH,
                        .label = skip + std::to_string(i + 1),
                    } 
                );
                result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
                scopes_put.top()++;

                block = compile_block( branch.second.get() );
                result.insert(result.end(), block.begin(), block.end());
                
                result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
                scopes_put.top()--;

                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::GOTO,
                        .label = skip + "all",
                    } 
                );
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::LABEL,
                        .label = skip + std::to_string(i + 1),
                    } 
                );
            }

            if (st->branch_else.has_value()) {
                auto brelse = st->branch_else.value();

                result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
                scopes_put.top()++;

                block = compile_block( brelse.get() );
                result.insert(result.end(), block.begin(), block.end());
                
                result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
                scopes_put.top()--;
            }

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = skip + "all",
                } 
            );
        } break;

        case LuaAST::Statement::Type::NUM_FOR: {
            LuaAST::Num_forSt* st = (LuaAST::Num_forSt*) statement; 
            // NUM_FOR,
            // for Var = Estart, Eend, Estep do BLOCK end
            /*
                [Eval Estart]
                SET_LOCAL __v
                
                [Eval Eend]
                SET_LOCAL __v_end
                
                [Eval Estep]
                SET_LOCAL __v_step

            __for_condition
                LOAD __v
                LOAD __v_stop
                [Eval __v > __v_stop]
                BRANCH __for_end
                
                PUT_SCOPE
                LOAD __v
                SET_LOCAL Var 
                [Block BLOCK]
                POP_SCOPE

                LOAD __v
                LOAD __v_step
                [Eval __v + __v_step]
                STORE __v
                GOTO __for_condition
            __for_end
            */

            std::string prefix = get_prefix();
            std::string ustr = get_ustr();

            std::string var = prefix + ustr + "numfor" + "__var";
            std::string var_end = prefix + ustr + "numfor" + "__var" + "__end";
            std::string var_step = prefix + ustr + "numfor" + "__var" + "__step";

            std::string cond_label = prefix + ustr + "numfor_cond";
            std::string end_label = prefix + ustr + "numfor_end";
            break_labels.push(end_label);
            scopes_put.push(0);

            auto exp = compile_expression(st->from.get(), 1);
            result.insert(result.end(), exp.begin(), exp.end());
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = var
                } 
            );

            exp = compile_expression(st->to.get(), 1);
            result.insert(result.end(), exp.begin(), exp.end());
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = var_end
                } 
            );

            if (st->step.has_value()) {
                exp = compile_expression(st->step.value().get(), 1);
                result.insert(result.end(), exp.begin(), exp.end());
            } else {
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::PUT_NUM,
                        .num = (int64_t) 1,
                    } 
                );
            }
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = var_step
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = cond_label
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = var_end
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::GT
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::BRANCH,
                    .label = end_label,
                } 
            );


            result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
            scopes_put.top()++;

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = st->var
                } 
            );

            auto block = compile_block(st->block.get());
            result.insert(result.end(), block.begin(), block.end());

            result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
            scopes_put.top()--;

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = var_step
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::ADD,
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::STORE,
                    .name = var
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::GOTO,
                    .label = cond_label,
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = end_label,
                } 
            );

            break_labels.pop();
            scopes_put.pop();
        } break;

        case LuaAST::Statement::Type::GEN_FOR: {
            LuaAST::Gen_forSt* st = (LuaAST::Gen_forSt*) statement;
            // GEN_FOR,
            // for var1, ...varn in exp do ...
            /*
                f, container, key = [exp]
                while true {
                    var1, ... varn = f(container, key)
                    key = var1
                    if (key == nil) break
                    key = var1
                    [block]
                }
            */
            /*
                PUT_SCOPE
                [Eval exp]
                -- f, s, key = exp
                STORE __var
                STORE __s
                STORE __f

            __genfor_cond
                PUT_SCOPE
                [Eval f(s, key)]
                STORE varn
                ...
                STORE var1

                LOAD var1
                PUT_NIL
                EQ
                BRANCH __genfor_end
                
                LOAD var1
                STORE key
                [Block]
                POP_SCOPE
                GOTO __genfor_cond
            __genfor_end
                POP_SCOPE
            */

            std::string prefix = get_prefix();
            std::string ustr = get_ustr();

            std::string cond_label = prefix + ustr + "genfor" + "_cond";
            std::string end_label = prefix + ustr + "genfor" + "_end";

            std::string var_var = prefix + ustr + "genfor" + "__var";
            std::string s_var = prefix + ustr + "genfor" + "__s";
            std::string f_var = prefix + ustr + "genfor" + "__f";
            
            result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
            scopes_put.top()++;
            
            auto exp = compile_expression(st->exp.get(), 3);
            result.insert(result.end(), exp.begin(), exp.end());

            break_labels.push(end_label);
            scopes_put.push(0);

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = var_var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = s_var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::SET_LOCAL,
                    .name = f_var
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = cond_label
                } 
            );

            result.push_back( Instruction { .type = Instruction::Type::PUT_SCOPE } );
            scopes_put.top()++;
            
            // f(s, var)
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = s_var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = var_var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = f_var
                } 
            );
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::CALL,
                    .N = 2,
                    .M = st->vars.size(),
                } 
            );

            for (auto it = st->vars.rbegin(); it != st->vars.rend(); it++) {
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::SET_LOCAL,
                        .name = *it
                    } 
                );
            }

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = st->vars[0]
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::PUT_NIL,
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::EQ,
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::BRANCH,
                    .label = end_label
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LOAD,
                    .name = st->vars[0]
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::STORE,
                    .name = var_var
                } 
            );

            auto block = compile_block(st->block.get());
            result.insert(result.end(), block.begin(), block.end());

            result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
            scopes_put.top()--;

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::GOTO,
                    .label = cond_label
                } 
            );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::LABEL,
                    .label = end_label
                } 
            );
            
            break_labels.pop();
            scopes_put.pop();

            result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
            scopes_put.top()--;
        } break;

        case LuaAST::Statement::Type::GOTO: {
            throw std::runtime_error("Gotos are not supported yet");
        } break;

        case LuaAST::Statement::Type::LABEL: {
            throw std::runtime_error("Labels are not supported yet");
        } break;
        
        case LuaAST::Statement::Type::BREAK: {
            auto label = break_labels.top();
            size_t N = scopes_put.top();
            for (size_t i=0; i<N; i++) {
                result.push_back( Instruction { .type = Instruction::Type::POP_SCOPE } );
            }
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::GOTO,
                    .label = label, 
                } 
            );
        } break;

        case LuaAST::Statement::Type::FUNCCALL: {
            LuaAST::FunccallSt* st = (LuaAST::FunccallSt*) statement;
            auto exp = compile_expression(st->funccall.get(), -1);
            result.insert(result.end(), exp.begin(), exp.end());
        } break;

        case LuaAST::Statement::Type::RETURN: {
            // RETURN e1, e2, en
            /*
                PUT_STACK
                [Eval e1]
                [Eval e2]
                ...
                [Eval en]
                RET WHOLE_STACK
            */ 
            LuaAST::ReturnSt* st = (LuaAST::ReturnSt*) statement;

            result.push_back( Instruction { .type = Instruction::Type::PUT_STACK } );
            size_t N = st->values.size();
            for (size_t i=0; i<N; i++) {
                auto exp = compile_expression(st->values[i].get(), -1);
                result.insert(result.end(), exp.begin(), exp.end());
            }
            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::RET,
                    .whole_stack = true,
                } 
            );
        } break;

        default:
        case LuaAST::Statement::Type::NONE: {
        } break;
    }


    // clear stack after statement
    result.push_back( 
        Instruction { 
            .type = Instruction::Type::CLEAR_STACK,
        } 
    );

    return result;
}

std::vector< Instruction > Compiler::compile_expression(
    LuaAST::Expression *expression,
    int expect_stack_size, // -1 for any size
    bool reversed
) {
    std::vector< Instruction > result;
    int stack_size = 0; 

    switch (expression->type()) {
        case LuaAST::Expression::Type::LITERAL: {
            LuaAST::Literal* exp = (LuaAST::Literal*) expression;
            stack_size = 1;
            switch (exp->kind) {
                case LuaAST::Literal::Kind::NIL: {
                    result.push_back( 
                        Instruction { 
                            .type = Instruction::Type::PUT_NIL,
                        } 
                    );
                } break;

                case LuaAST::Literal::Kind::BOOLEAN: {
                    if (std::get<bool>(exp->value)) {
                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::PUT_TRUE,
                            } 
                        );
                    } else {
                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::PUT_FALSE,
                            } 
                        );
                    }
                } break;

                case LuaAST::Literal::Kind::NUMBER: {
                    auto& num = std::get<LuaAST::Number>(exp->value);
                    if (num.kind == LuaAST::Number::Kind::INT) {
                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::PUT_NUM,
                                .num = (int64_t) num.i
                            } 
                        );
                    } else {
                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::PUT_NUM,
                                .num = (std::float64_t) num.f
                            } 
                        );
                    }
                } break;
                
                case LuaAST::Literal::Kind::STRING: {
                    std::string str = std::get<std::string> (exp->value);
                    str = unescapeString(str.substr(1, str.size()-2));

                    result.push_back( 
                        Instruction { 
                            .type = Instruction::Type::PUT_STR,
                            .str = str
                        } 
                    );
                } break;
                
                default: {
                    throw std::runtime_error("Unknown literal type: " + std::to_string((int)exp->kind));
                } break;
            }
        } break;

        case LuaAST::Expression::Type::FUNC_ANON: {
            LuaAST::FuncAnon* exp = (LuaAST::FuncAnon*) expression;
            stack_size = 1;

            std::string funcname = get_prefix() + get_ustr() + "lambda";
            functions_to_compile.push( { funcname, exp->body.get() } );

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::PUT_FUNC,
                    .name = exp->body->variadic_param.value_or("varg"),
                    .label = funcname,
                    .N = exp->body->params.size(),
                } 
            );
        } break;

        case LuaAST::Expression::Type::TABLE_CONSTR: {
            LuaAST::TableConstr* exp = (LuaAST::TableConstr*) expression;
            stack_size = 1;
            // TABLE_CONSTR,
            // { ... }
            /*
                PUT_TABLE 
                [Fields]
            */

            result.push_back( 
                Instruction { 
                    .type = Instruction::Type::PUT_TABLE,
                } 
            );

            // [key] = value
            /*
                DUP
                [Eval key]
                [Eval value]    
                ASSIGN_WHOM_WHERE_WHAT
            */

            // name = value
            /*
                DUP
                [Eval value]
                ASSIGN_WHOM_WHAT name
            */

            // value
            /*
                DUP
                PUT_NUM [Index]
                [Eval value]
                ASSIGN_WHOM_WHERE_WHAT
            */
            size_t N = 0;
            for (auto field: exp->fields) {
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::DUP,
                    } 
                );

                switch (field->kind) {
                    case LuaAST::Field::Kind::INDEXED: {
                        auto sub = compile_expression(field->lhs.get(), 1);
                        result.insert(result.end(), sub.begin(), sub.end());

                        sub = compile_expression(field->rhs.get(), 1);
                        result.insert(result.end(), sub.begin(), sub.end());

                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::ASSIGN_WHOM_WHERE_WHAT,
                            } 
                        );
                    } break;

                    case LuaAST::Field::Kind::NAMED: {
                        auto sub = compile_expression(field->rhs.get(), 1);
                        result.insert(result.end(), sub.begin(), sub.end());

                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::ASSIGN_WHOM_WHAT,
                                .field = field->name,
                            } 
                        );
                    } break;
                    
                    case LuaAST::Field::Kind::SINGLE: {
                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::PUT_NUM,
                                .num = (int64_t) N++,
                            } 
                        );

                        auto sub = compile_expression(field->rhs.get(), 1);
                        result.insert(result.end(), sub.begin(), sub.end());

                        result.push_back( 
                            Instruction { 
                                .type = Instruction::Type::ASSIGN_WHOM_WHERE_WHAT,
                            } 
                        );
                    } break;

                    default: {
                        throw std::runtime_error("Unexpected field type");
                    } break;
                }
            }

        } break;

        case LuaAST::Expression::Type::OPERATION: {
            LuaAST::Operation* exp = (LuaAST::Operation*) expression;
            stack_size = 1;

            if (exp->kind == LuaAST::Operation::Kind::UNOP) {
                auto sub = compile_expression(exp->lhs.get(), 1);
                result.insert(result.end(), sub.begin(), sub.end());

                if (exp->operat == "#") {
                    result.push_back( Instruction { .type = Instruction::Type::HASH } );
                } else
                if (exp->operat == "-") {
                    result.push_back( Instruction { .type = Instruction::Type::NEG } );
                } else
                if (exp->operat == "not") {
                    result.push_back( Instruction { .type = Instruction::Type::NOT } );
                } else
                if (exp->operat == "~") {
                    result.push_back( Instruction { .type = Instruction::Type::BITNOT } );
                } else {
                    throw std::runtime_error("Unknown operator " + exp->operat);
                }
            } else {
                auto sub = compile_expression(exp->lhs.get(), 1);
                result.insert(result.end(), sub.begin(), sub.end());

                sub = compile_expression(exp->rhs.get(), 1);
                result.insert(result.end(), sub.begin(), sub.end());

                if (exp->operat == "<=") {
                    result.push_back( Instruction { .type = Instruction::Type::LE } );
                } else
                if (exp->operat == "<") {
                    result.push_back( Instruction { .type = Instruction::Type::LT } );
                } else
                if (exp->operat == ">=") {
                    result.push_back( Instruction { .type = Instruction::Type::GE } );
                } else
                if (exp->operat == ">") {
                    result.push_back( Instruction { .type = Instruction::Type::GT } );
                } else
                if (exp->operat == "==") {
                    result.push_back( Instruction { .type = Instruction::Type::EQ } );
                } else
                if (exp->operat == "~=") {
                    result.push_back( Instruction { .type = Instruction::Type::NEQ } );
                } else
                if (exp->operat == "..") {
                    result.push_back( Instruction { .type = Instruction::Type::CONCAT } );
                } else
                if (exp->operat == "+") {
                    result.push_back( Instruction { .type = Instruction::Type::ADD } );
                } else
                if (exp->operat == "-") {
                    result.push_back( Instruction { .type = Instruction::Type::SUB } );
                } else
                if (exp->operat == "*") {
                    result.push_back( Instruction { .type = Instruction::Type::MUL } );
                } else
                if (exp->operat == "%") {
                    result.push_back( Instruction { .type = Instruction::Type::MOD } );
                } else
                if (exp->operat == "//") {
                    result.push_back( Instruction { .type = Instruction::Type::DIV } );
                } else
                if (exp->operat == "/") {
                    result.push_back( Instruction { .type = Instruction::Type::TRUEDIV } );
                } else
                if (exp->operat == "<<") {
                    result.push_back( Instruction { .type = Instruction::Type::SHLEFT } );
                } else
                if (exp->operat == ">>") {
                    result.push_back( Instruction { .type = Instruction::Type::SHRIGHT } );
                } else
                if (exp->operat == "and") {
                    result.push_back( Instruction { .type = Instruction::Type::AND } );
                } else
                if (exp->operat == "or") {
                    result.push_back( Instruction { .type = Instruction::Type::OR } );
                } else
                if (exp->operat == "&") {
                    result.push_back( Instruction { .type = Instruction::Type::BITAND } );
                } else
                if (exp->operat == "|") {
                    result.push_back( Instruction { .type = Instruction::Type::BITOR } );
                } else
                if (exp->operat == "~") {
                    result.push_back( Instruction { .type = Instruction::Type::BITXOR } );
                } else
                if (exp->operat == "^") {
                    result.push_back( Instruction { .type = Instruction::Type::POW } );
                } else {
                    throw std::runtime_error("Unknown operator " + exp->operat);
                }
            }
        } break;

        case LuaAST::Expression::Type::VAR: {
            LuaAST::Var* var = (LuaAST::Var*)expression;
            stack_size = 1;

            LuaAST::VarPart* base = var->base.get();
            if (base->kind == LuaAST::VarPart::Kind::NAME) {
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::LOAD,
                        .name = ((LuaAST::VarPartName*)base)->name,
                    } 
                );
            } else {
                auto sub = compile_expression(((LuaAST::VarPartExp*)base)->exp.get(), 1);
                result.insert(result.end(), sub.begin(), sub.end());
            }

            for (auto &spec: var->specifications) {
                if (spec->kind == LuaAST::VarPart::Kind::NAME) {
                    result.push_back( 
                        Instruction { 
                            .type = Instruction::Type::INDEX,
                            .field = ((LuaAST::VarPartName*)spec.get())->name,
                        } 
                    );
                } else {
                    auto sub = compile_expression(((LuaAST::VarPartExp*)spec.get())->exp.get(), 1);
                    result.insert(result.end(), sub.begin(), sub.end());

                    result.push_back( 
                        Instruction { 
                            .type = Instruction::Type::DYN_INDEX,
                        } 
                    );
                }
            }
        } break;

        case LuaAST::Expression::Type::FUNCCALL: {
            LuaAST::FuncCall* fc = (LuaAST::FuncCall*) expression;
            stack_size = expect_stack_size;
            // FUNCCALL,
            // obj:method(a1, a2)
            // => obj.method(obj, a1, a2)
            // (e1)(a11, a12)
            /*
                [Eval e1]
                METAINDEX __call
                [Eval a11]
                [Eval a12]
                
                CALL
            */
            // (e1):method(a11, a12)
            /*
                [Eval e1]
                METAINDEX method
                [Eval a11]
                [Eval a12]
                
                CALL
            */
            // func(a11, a12)
            /*
                [Eval a11]
                [Eval a12]

                LOAD func
                CALL
            */

            // CALL
            /*
                PUT_STACK
                [Eval args]
                function
                CALL stack
                MOVE_STACK expected_size
            */
            // REV_CALL
            /*
                PUT_STACK
                function
                [Eval args]
                CALL stack
                MOVE_STACK expected_size
            */

            result.push_back( 
                Instruction { .type = Instruction::Type::PUT_STACK} 
            );

            auto sub = compile_expression(fc->function.get(), 1);
            result.insert(result.end(), sub.begin(), sub.end());

            size_t I = fc->tails.size();
            for (size_t t = 0; t < I; t++) {
                auto& tail = fc->tails[t];
                if (tail->name.has_value()) {
                    result.push_back( 
                        Instruction { 
                            .type = Instruction::Type::METAINDEX,
                            .metafield = tail->name.value()
                        } 
                    );
                }
                
                size_t N = tail->args.size();
                for (size_t i=0; i<N; i++) {
                    sub = compile_expression(tail->args[i].get(), -1);
                    result.insert(result.end(), sub.begin(), sub.end());
                }
                

                if (t != I-1) {
                    result.push_back( 
                        Instruction { 
                            .type = Instruction::Type::REV_CALL,
                            // .N = N,
                            .whole_stack = true,
                            .M = 1,
                        }
                    );
                } else {
                    result.push_back( 
                        Instruction { 
                            .type = Instruction::Type::REV_CALL,
                            // .N = N,
                            .whole_stack = true,
                            .any_output = true,
                        }
                    );
                }
            }

            if (expect_stack_size < 0) {
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::MOVE_STACK,
                        .reverse_stack = reversed,
                        .whole_stack = true,
                    } 
                );
            } else {
                result.push_back( 
                    Instruction { 
                        .type = Instruction::Type::MOVE_STACK,
                        .reverse_stack = reversed,
                        .N = (size_t) expect_stack_size,
                    } 
                );
            }
        } break;

        default: {
            throw std::runtime_error("Unknown expression type: " + std::to_string((int)expression->type()));
        } break;
    }

    if (expect_stack_size != -1) {
        while (expect_stack_size < stack_size) {
            result.push_back( Instruction { .type = Instruction::Type::DISCARD } );
            stack_size--;
        }
        while (expect_stack_size > stack_size) {
            result.push_back( Instruction { .type = Instruction::Type::PUT_NIL } );
            stack_size++;
        }
    }

    return result;
}
