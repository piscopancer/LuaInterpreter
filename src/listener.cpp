#include "listener.h"


Lua55Listener::Lua55Listener() = default;
Lua55Listener::~Lua55Listener() = default;

Lua55Listener::State::Frame::Frame(): parent(nullptr), last(nullptr) {}

void Lua55Listener::exitProg(Lua55GrammarParser::ProgContext * ctx){
    // std::cout << "Lua55Listener::exitProg" << std::endl;
    assert(state.stack.size() == 1);
    assert(state.frames.size() == 0);

    ast = std::shared_ptr<Block>((Block*)state.stack.top());
    state.stack.pop();
}

void Lua55Listener::enterBlock(Lua55GrammarParser::BlockContext * ctx){
    // std::cout << "enterBlock" << std::endl; 
    Block* block = new Block;
    State::Frame frame;
    frame.parent = block;
    state.frames.push(frame);
}
void Lua55Listener::exitBlock(Lua55GrammarParser::BlockContext * ctx){
    // std::cout << "Lua55Listener::exitBlock" << std::endl; 
    State::Frame frame = state.frames.top();
    state.frames.pop();
    Block* block = frame.parent;

    size_t n = ctx->statement().size();
    if (ctx->returnStatement()) n += 1;

    block->statements = std::vector< std::shared_ptr<Statement> >(n, nullptr);
    for (size_t i=0; i<n; i++) {
        block->statements[n-1-i] = std::shared_ptr<Statement>( (Statement*)state.stack.top() );
        state.stack.pop();
    }

    state.stack.push(block);
}

void Lua55Listener::exitStatement(Lua55GrammarParser::StatementContext * ctx){
    // std::cout << "Lua55Listener::exitStatement" << std::endl; 
    Statement* st = (Statement*) state.stack.top();
    
    State::Frame& frame = state.frames.top();
    

    st->parent = frame.parent;
    if (frame.last) {
        frame.last->next = st;
    }
    
    st->prev = frame.last;
    frame.last = st;

    // std::cout << "statement ";
    // st->print(std::cout);
    // std::cout << std::endl;
}

void Lua55Listener::exitDoBlockStatement(Lua55GrammarParser::DoBlockStatementContext * ctx){
    // std::cout << "Lua55Listener::exitDoBlockStatement" << std::endl; 
    DoBlockSt* st = new DoBlockSt;

    st->block = std::shared_ptr<Block>((Block*) state.stack.top());
    state.stack.pop();
    st->block->parent = st;

    state.stack.push(st);
}

void Lua55Listener::exitAssignmentStatement(Lua55GrammarParser::AssignmentStatementContext * ctx){
    // std::cout << "Lua55Listener::exitAssignmentStatement" << std::endl; 
    size_t exp_n = ctx->explist()->exp().size();
    size_t var_n = ctx->varlist()->var().size();
    
    AssignSt* st = new AssignSt;

    st->rhs = std::vector< std::shared_ptr <Expression> >(exp_n, nullptr);
    for (size_t i=0; i<exp_n; i++) {
        st->rhs[exp_n-1-i] = std::shared_ptr<Expression>((Expression*) state.stack.top());
        state.stack.pop();
    }

    st->lhs = std::vector< std::shared_ptr <Var> >(var_n, nullptr);
    for (size_t i=0; i<var_n; i++) {
        st->lhs[var_n-1-i] = std::shared_ptr<Var>((Var*) state.stack.top());
        state.stack.pop();
    }

    state.stack.push(st);
}
        
void Lua55Listener::exitDeclarationStatement(Lua55GrammarParser::DeclarationStatementContext * ctx){
    // std::cout << "Lua55Listener::exitDeclarationStatement" << std::endl; 
    DeclareSt* st = new DeclareSt;

    if (ctx->scopeSpec()->GLOBAL()) st->scope = ScopeSpec::GLOBAL;
    else st->scope = ScopeSpec::LOCAL;

    // if (ctx->attrib()) st->common_attr = std::shared_ptr<Attribute>(new Attribute(ctx->attrib()->ATTRIBUTES_DEFINED()->toString()));

    size_t attr_n = ctx->attnamelist()->nameattr().size();
    for (size_t i=0; i<attr_n; i++) {
        auto nameattr = ctx->attnamelist()->nameattr(i);
        std::string name = nameattr->name()->ID()->toString();
        Attribute* attr = nullptr;
        if (nameattr->ATTRIB()) attr = new Attribute(nameattr->ATTRIB()->toString());
        st->lhs.push_back(
            {name, std::shared_ptr<Attribute>(attr)}
        );
    }

    size_t exp_n = ctx->explist() ? ctx->explist()->exp().size() : 0;
    st->rhs = std::vector< std::shared_ptr<Expression> > (exp_n, nullptr);
    for (size_t i=0; i<exp_n; i++) {
        st->rhs[exp_n-1-i] = std::shared_ptr<Expression>( (Expression*) state.stack.top() );
        state.stack.pop();
    }

    state.stack.push(st);
}

void Lua55Listener::exitGlobalAttribStatement(Lua55GrammarParser::GlobalAttribStatementContext * ctx){
    // std::cout << "Lua55Listener::exitGlobalAttribStatement" << std::endl; 
    AttribSt* st = new AttribSt;
    st->attr = std::shared_ptr<Attribute>(new Attribute(ctx->ATTRIB()->toString()));
    state.stack.push(st);
}



void Lua55Listener::exitFuncdefStatement(Lua55GrammarParser::FuncdefStatementContext * ctx){
    // std::cout << "Lua55Listener::exitFuncdefStatement" << std::endl;
    }

void Lua55Listener::exitDefaultFuncdefStatement(Lua55GrammarParser::DefaultFuncdefStatementContext * ctx){
    // std::cout << "Lua55Listener::exitDefaultFuncdefStatement" << std::endl; 
    DefaultFuncdefSt *st = new DefaultFuncdefSt;

    st->body = std::shared_ptr<FuncBody>((FuncBody*) state.stack.top());
    state.stack.pop();
    
    st->name = std::shared_ptr<FuncName>((FuncName*) state.stack.top());
    state.stack.pop();

    state.stack.push(st);
}

void Lua55Listener::exitScopedFuncdefStatement(Lua55GrammarParser::ScopedFuncdefStatementContext * ctx){
    // std::cout << "Lua55Listener::exitScopedFuncdefStatement" << std::endl; 
    ScopedFuncdefSt *st = new ScopedFuncdefSt;

    st->body = std::shared_ptr<FuncBody>((FuncBody*) state.stack.top());
    state.stack.pop();

    if (ctx->scopeSpec()->GLOBAL()) st->scope = ScopeSpec::GLOBAL;
    else st->scope = ScopeSpec::LOCAL;

    st->name = ctx->name()->ID()->toString();

    state.stack.push(st);
}

void Lua55Listener::exitFuncname(Lua55GrammarParser::FuncnameContext * ctx){
    // std::cout << "Lua55Listener::exitFuncname" << std::endl; 
    FuncName * name = new FuncName;
    
    name->base = ctx->namespec()->name(0)->ID()->toString();
    size_t n = ctx->namespec()->name().size();
    for (size_t i=1; i<n; i++) {
        name->specifications.push_back(
            ctx->namespec()->name(i)->ID()->toString()
        );
    }
    if (ctx->name()) name->kind = ctx->name()->ID()->toString();

    state.stack.push(name);
}

void Lua55Listener::exitFuncbody(Lua55GrammarParser::FuncbodyContext * ctx){
    // std::cout << "Lua55Listener::exitFuncbody" << std::endl;
    FuncBody * body = new FuncBody;
    
    body->block = std::shared_ptr<Block>((Block*) state.stack.top());
    state.stack.pop();

    if (ctx->paramlist()) {
        if (ctx->paramlist()->vararg()) {
            body->variadic = true;
            if (ctx->paramlist()->vararg()->name()) body->variadic_param = ctx->paramlist()->vararg()->name()->ID()->toString();
        }

        if (ctx->paramlist()->namelist()) {
            for (auto& name_ctx: ctx->paramlist()->namelist()->name()) {
                body->params.push_back(name_ctx->ID()->toString());
            }
        }
    }

    state.stack.push(body);
}

void Lua55Listener::exitWhileStatement(Lua55GrammarParser::WhileStatementContext * ctx){
    // std::cout << "Lua55Listener::exitWhileStatement" << std::endl; 
    WhileSt * st = new WhileSt;

    st->block = std::shared_ptr<Block>((Block*) state.stack.top());
    state.stack.pop();
    st->block->parent = st;
    
    st->cond = std::shared_ptr<Expression>((Expression*) state.stack.top());
    state.stack.pop();

    state.stack.push(st);
}

void Lua55Listener::exitRepeatStatement(Lua55GrammarParser::RepeatStatementContext * ctx){
    // std::cout << "Lua55Listener::exitRepeatStatement" << std::endl; 
    RepeatSt * st = new RepeatSt;

    st->un_cond = std::shared_ptr<Expression>((Expression*) state.stack.top());
    state.stack.pop();

    st->block = std::shared_ptr<Block>((Block*) state.stack.top());
    state.stack.pop();
    st->block->parent = st;

    state.stack.push(st);
}

void Lua55Listener::exitIfStatement(Lua55GrammarParser::IfStatementContext * ctx){
    // std::cout << "Lua55Listener::exitIfStatement" << std::endl; 
    IfSt * st = new IfSt;

    size_t elif_n = ctx->ELSEIF().size();

    std::vector<Expression*> exps;
    std::vector<Block*> blocks;
    bool else_branch = false;

    if (ctx->ELSE()) {
        Block* block = (Block*) state.stack.top();
        state.stack.pop();
        block->parent = st;
        blocks.push_back(block);
        else_branch = true;
    }
    for (size_t i=0; i<elif_n+1; i++) {
        Block* block = (Block*) state.stack.top();
        state.stack.pop();
        block->parent = st;
        blocks.push_back(block);

        exps.push_back((Expression*) state.stack.top());
        state.stack.pop();
    }

    IfSt::CondBlock condblock;

    condblock.first = std::shared_ptr<Expression>( exps[elif_n] );
    condblock.second = std::shared_ptr<Block>( blocks[elif_n + else_branch] );
    st->branch_if = condblock;

    for (size_t i=0; i<elif_n; i++) {
        condblock.first = std::shared_ptr<Expression>( exps[elif_n - 1 - i] );
        condblock.second = std::shared_ptr<Block>( blocks[elif_n - 1 - i + else_branch] );
        st->branch_elseif.push_back(condblock);
    }

    if (else_branch) {
        st->branch_else = std::shared_ptr<Block>( blocks[0] );
    }

    state.stack.push(st);
}

void Lua55Listener::exitNumericForStatement(Lua55GrammarParser::NumericForStatementContext * ctx){
    // std::cout << "Lua55Listener::exitNumericForStatement" << std::endl; 
    Num_forSt * st = new Num_forSt;

    st->block = std::shared_ptr<Block>((Block*) state.stack.top());
    state.stack.pop();
    st->block->parent = st;

    size_t exp_n = ctx->exp().size();
    std::vector<Expression*> exps(exp_n, nullptr);
    for (size_t i=0; i<exp_n; i++) {
        exps[exp_n-1-i] = (Expression*) state.stack.top();
        state.stack.pop();
    }

    if (exp_n == 3) st->step = std::shared_ptr<Expression>(exps[2]);
    st->to   = std::shared_ptr<Expression>(exps[1]);
    st->from = std::shared_ptr<Expression>(exps[0]);

    st->var = ctx->name()->ID()->toString();

    state.stack.push(st);
}

void Lua55Listener::exitGenericForStatement(Lua55GrammarParser::GenericForStatementContext * ctx){
    // std::cout << "Lua55Listener::exitGenericForStatement" << std::endl; 
    Gen_forSt * st = new Gen_forSt;

    st->block = std::shared_ptr<Block>((Block*) state.stack.top());
    state.stack.pop();
    st->block->parent = st;

    size_t name_n = ctx->namelist()->name().size();
    
    st->vars = std::vector< std::string > (name_n, "");
    st->exp = std::shared_ptr<Expression>((Expression*) state.stack.top());
    state.stack.pop();
    
    for (size_t i=0; i<name_n; i++) {
        st->vars[i] = ctx->namelist()->name(i)->ID()->toString();
    }

    state.stack.push(st);
}

void Lua55Listener::exitGotoStatement(Lua55GrammarParser::GotoStatementContext * ctx){
    // std::cout << "Lua55Listener::exitGotoStatement" << std::endl; 
    GotoSt * st = new GotoSt;
    st->label = ctx->name()->ID()->toString();
    state.stack.push(st);
}

void Lua55Listener::exitLabelStatement(Lua55GrammarParser::LabelStatementContext * ctx){
    // std::cout << "Lua55Listener::exitLabelStatement" << std::endl; 
    LabelSt * st = new LabelSt;
    st->label = ctx->name()->ID()->toString();
    state.stack.push(st);
}

void Lua55Listener::exitBreakStatement(Lua55GrammarParser::BreakStatementContext * ctx){
    // std::cout << "Lua55Listener::exitBreakStatement" << std::endl; 
    BreakSt * st = new BreakSt;
    state.stack.push(st);
}

void Lua55Listener::exitReturnStatement(Lua55GrammarParser::ReturnStatementContext * ctx){
    // std::cout << "Lua55Listener::exitReturnStatement" << std::endl; 
    ReturnSt * st = new ReturnSt;

    size_t n = ctx->explist()->exp().size();
    st->values = std::vector< std::shared_ptr<Expression> >(n, nullptr);
    for (size_t i=0; i<n; i++) {
        st->values[n-1-i] = std::shared_ptr<Expression>( (Expression*) state.stack.top() );
        state.stack.pop();
    }

    state.stack.push(st);
}

void Lua55Listener::exitFuncCallStatement(Lua55GrammarParser::FuncCallStatementContext * ctx){
    // std::cout << "Lua55Listener::exitFuncCallStatement" << std::endl; 
    FunccallSt *st = new FunccallSt;

    st->funccall = std::shared_ptr<FuncCall>((FuncCall*) state.stack.top());
    state.stack.pop();

    state.stack.push(st);
}



void Lua55Listener::exitFuncAnon(Lua55GrammarParser::FuncAnonContext * ctx){
    // std::cout << "Lua55Listener::exitFuncAnon" << std::endl; 
    FuncAnon* func = new FuncAnon;
    
    func->body = std::shared_ptr<FuncBody>( (FuncBody*) state.stack.top() );
    state.stack.pop();

    state.stack.push(func);
}

void Lua55Listener::exitTableConstructor(Lua55GrammarParser::TableConstructorContext * ctx){
    // std::cout << "Lua55Listener::exitTableConstructor" << std::endl; 
    TableConstr* table = new TableConstr;
    if (ctx->fieldlist()) {
        size_t n = ctx->fieldlist()->field().size();
        table->fields = std::vector< std::shared_ptr<Field> >(n, nullptr);
        for (size_t i=0; i<n; i++) {
            table->fields[n-1-i] = std::shared_ptr<Field>((Field*) state.stack.top());
            state.stack.pop();
        }
    }
    state.stack.push(table);
}
        
void Lua55Listener::exitField(Lua55GrammarParser::FieldContext * ctx){
    // std::cout << "Lua55Listener::exitField" << std::endl;
    Field* field = new Field;
    if (ctx->exp().size() == 2) {
        // [exp] = exp
        field->kind = Field::Kind::INDEXED;
        
        field->rhs = std::shared_ptr<Expression>((Expression*) state.stack.top());
        state.stack.pop();
        
        field->lhs = std::shared_ptr<Expression>((Expression*) state.stack.top());
        state.stack.pop();
    } else if (ctx->name()) {
        // name = exp
        field->kind = Field::Kind::NAMED;
        field->name = ctx->name()->ID()->toString();
        field->rhs = std::shared_ptr<Expression>((Expression*) state.stack.top());
        state.stack.pop();
    }
    else {
        // exp
        field->kind = Field::Kind::SINGLE;
        
        field->rhs = std::shared_ptr<Expression>((Expression*) state.stack.top());
        state.stack.pop();
    }
    state.stack.push(field);
}

void Lua55Listener::exitExp(Lua55GrammarParser::ExpContext * ctx){
    // std::cout << "Lua55Listener::exitExp" << std::endl; 
    
    // std::cout << "expression ";
    // state.stack.top()->print(std::cout);
    // std::cout << std::endl;
}

void Lua55Listener::exitOrExp(Lua55GrammarParser::OrExpContext * ctx){ 
    if (ctx->andExp().size() > 1) {
        size_t n = ctx->andExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = ctx->OR(i)->toString();

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitAndExp(Lua55GrammarParser::AndExpContext * ctx){ 
    if (ctx->compExp().size() > 1) {
        size_t n = ctx->compExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = ctx->AND(i)->toString();

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitCompExp(Lua55GrammarParser::CompExpContext * ctx){ 
    if (ctx->compop()) {
        Expression* rhs = (Expression*) state.stack.top();
        state.stack.pop();

        Expression* lhs = (Expression*) state.stack.top();
        state.stack.pop();

        Operation* opexp = new Operation;
        opexp->kind = Operation::Kind::BINOP;
        opexp->operat = ctx->compop()->getText();
        opexp->lhs = std::shared_ptr<Expression>(lhs);
        opexp->rhs = std::shared_ptr<Expression>(rhs);

        state.stack.push(opexp);
    }
}

void Lua55Listener::exitBitorExp(Lua55GrammarParser::BitorExpContext * ctx){ 
    if (ctx->bitxorExp().size() > 1) {
        size_t n = ctx->bitxorExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = "|";

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitBitxorExp(Lua55GrammarParser::BitxorExpContext * ctx){ 
    if (ctx->bitandExp().size() > 1) {
        size_t n = ctx->bitandExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = "~";

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitBitandExp(Lua55GrammarParser::BitandExpContext * ctx){ 
    if (ctx->shiftExp().size() > 1) {
        size_t n = ctx->shiftExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = "&";

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitShiftExp(Lua55GrammarParser::ShiftExpContext * ctx){ 
    if (ctx->shiftop().size() > 0) {
        size_t n = ctx->concatExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = ctx->shiftop(i)->getText();

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitConcatExp(Lua55GrammarParser::ConcatExpContext * ctx){ 
    if (ctx->plusExp().size() > 1) {
        size_t n = ctx->plusExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = "..";

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitPlusExp(Lua55GrammarParser::PlusExpContext * ctx){ 
    if (ctx->plusop().size() > 0) {
        size_t n = ctx->mulExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = ctx->plusop(i)->getText();

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitMulExp(Lua55GrammarParser::MulExpContext * ctx){ 
    if (ctx->mulop().size() > 0) {
        size_t n = ctx->unaryExp().size();

        std::vector<Expression*> operands(n, nullptr);

        for (size_t i=0; i<n; i++) {
            operands[n-1-i] = (Expression*) state.stack.top();
            state.stack.pop();
        }

        
        for (size_t i=0; i<n-1; i++) {
            Operation* opexp = new Operation;

            Expression* lhs = operands[i];
            Expression* rhs = operands[i+1];

            opexp->kind = Operation::Kind::BINOP;
            opexp->operat = ctx->mulop(i)->getText();

            opexp->lhs = std::shared_ptr<Expression>(lhs);
            opexp->rhs = std::shared_ptr<Expression>(rhs);

            operands[i+1] = opexp;
        }

        state.stack.push(operands[n-1]);
    }
}

void Lua55Listener::exitUnaryExp(Lua55GrammarParser::UnaryExpContext * ctx){ 
    if (ctx->unop()) {
        Expression* lhs = (Expression*) state.stack.top();
        state.stack.pop();

        Operation* opexp = new Operation;
        opexp->kind = Operation::Kind::UNOP;
        opexp->operat = ctx->unop()->getText();
        opexp->lhs = std::shared_ptr<Expression>(lhs);

        state.stack.push(opexp);
    }
}

void Lua55Listener::exitPowExp(Lua55GrammarParser::PowExpContext * ctx){
    if (ctx->powExp()) {
        Expression* rhs = (Expression*) state.stack.top();
        state.stack.pop();

        Expression* lhs = (Expression*) state.stack.top();
        state.stack.pop();

        Operation* opexp = new Operation;
        opexp->kind = Operation::Kind::BINOP;
        opexp->operat = "^";
        opexp->lhs = std::shared_ptr<Expression>(lhs);
        opexp->rhs = std::shared_ptr<Expression>(rhs);

        state.stack.push(opexp);
    }
}

void Lua55Listener::exitLiteral(Lua55GrammarParser::LiteralContext * ctx){
    // std::cout << "Lua55Listener::exitLiteral" << std::endl; 
    Literal* lit = new Literal;
    
    if (ctx->NIL()) {
        lit->kind = Literal::Kind::NIL;
    } else 

    if (ctx->FALSE()) {
        lit->kind = Literal::Kind::BOOLEAN;
        lit->value = false;
    } else 
    if (ctx->TRUE()) {
        lit->kind = Literal::Kind::BOOLEAN;
        lit->value = true;
    } else

    if (ctx->STRING()) {
        lit->kind = Literal::Kind::STRING;
        lit->value = ctx->STRING()->toString();
    } else

    if (ctx->NUMBER()) {
        lit->kind = Literal::Kind::NUMBER;
        std::string number = ctx->NUMBER()->toString();
        Number num;
        int sign = 1;
        if (number[0] == '-') { 
            sign = -1;
            number = number.substr(1);
        }
        if (number.contains('.')) {
            // double
            num.kind = Number::Kind::FLOAT;
            num.f = std::stod(number)*sign;
        } else {
            // int
            num.kind = Number::Kind::INT;
            num.i = std::stoll(number)*sign;
        }
        lit->value = num;
    } else {
        std::cerr << "UNEXPECTED LITERAL:" << ctx->toString() << std::endl;
    }

    // lit->print(std::cout);
    
    state.stack.push(lit);
}
    
void Lua55Listener::exitFuncCall(Lua55GrammarParser::FuncCallContext * ctx){
    // std::cout << "Lua55Listener::exitFuncCall" << std::endl;
    FuncCall* funccall = new FuncCall;

    std::stack<FuncCallTail*> tails;
    size_t n = ctx->funcCall_tail().size();
    for (size_t i=0; i<n; i++) {
        tails.push((FuncCallTail*) state.stack.top());
        state.stack.pop();
    }
    for (size_t i=0; i<n; i++) {
        funccall->tails.push_back(
            std::shared_ptr<FuncCallTail>((FuncCallTail*) tails.top())
        );
        tails.pop();
    }
    
    // var inherits from expr
    funccall->function = std::shared_ptr<Expression>((Expression*) state.stack.top());
    state.stack.pop();

    
    // funccall->print(std::cout);
    
    state.stack.push(funccall);
}

void Lua55Listener::exitFuncCall_tail(Lua55GrammarParser::FuncCall_tailContext * ctx){
    // std::cout << "Lua55Listener::exitFuncCall_tail" << std::endl;
    FuncCallTail* tail = new FuncCallTail;
    if (ctx->name()){
        tail->name = ctx->name()->ID()->toString();
    }
    if (ctx->args()->STRING()) {
        Literal* lit = new Literal;
        lit->kind = Literal::Kind::STRING;
        lit->value = ctx->args()->STRING()->toString();
        tail->args.push_back(std::shared_ptr<Expression>(lit));
    } else if (ctx->args()->explist()) {
        size_t n = ctx->args()->explist()->exp().size();
        std::stack<Expression*> temp;
        for (size_t i=0; i<n; i++) {
            temp.push((Expression*)state.stack.top());
            state.stack.pop();
        }
        for (size_t i=0; i<n; i++) {
            tail->args.push_back(
                std::shared_ptr<Expression>((Expression*)temp.top())
            );
            temp.pop();
        }
    }

    state.stack.push(tail);
}
    
void Lua55Listener::exitVar(Lua55GrammarParser::VarContext * ctx){
    // std::cout << "Lua55Listener::exitVar" << std::endl;
    Var* var = new Var;

    size_t n = ctx->var_tail().size();
    std::stack< VarPart* > parts;
    for (size_t i=0; i<n; i++) {
        parts.push( (VarPart*) state.stack.top() );
        state.stack.pop();
    }
    for (size_t i=0; i<n; i++) {
        var->specifications.push_back(
            std::shared_ptr<VarPart>(parts.top())
        );
        parts.pop();
    }
    
    if (ctx->name()) {
        var->base.reset(new VarPartName(ctx->name()->ID()->toString()));
        var->base->kind = VarPart::Kind::NAME;
    } else {
        std::shared_ptr<Expression> exp( (Expression*) state.stack.top() ); 
        state.stack.pop();

        var->base.reset(new VarPartExp(exp));
        var->base->kind = VarPart::Kind::EXP;
    }

    
    // var->print(std::cout);
    
    state.stack.push(var);
}

void Lua55Listener::exitVar_tail(Lua55GrammarParser::Var_tailContext * ctx){
    // std::cout << "Lua55Listener::exitVar_tail" << std::endl;
    VarPart* part;

    if (ctx->name()) {
        part = new VarPartName(ctx->name()->ID()->toString());
        
        part->kind = VarPart::Kind::NAME;
    } else {
        std::shared_ptr<Expression> exp( (Expression*) state.stack.top() ); 
        state.stack.pop();
        
        // exp->print(std::cout);
        
        
        part = new VarPartExp(exp);
        part->kind = VarPart::Kind::EXP;
    }
    state.stack.push(part);
}

void Lua55Listener::exitName(Lua55GrammarParser::NameContext * ctx){ 
    // std::cout << "Lua55Listener::exitName(" << ctx->ID()->toString() << ")" << std::endl;
}

void ErrorCountListener::syntaxError(antlr4::Recognizer* recognizer, 
                     antlr4::Token* offendingSymbol, 
                     size_t line, 
                     size_t charPositionInLine, 
                     const std::string& msg, 
                     std::exception_ptr e) {
    errorCount++;
    errors.push_back("line " + std::to_string(line) + ":" + 
                        std::to_string(charPositionInLine) + " " + msg);
}

bool ErrorCountListener::hasErrors() const { return errorCount > 0; }
size_t ErrorCountListener::getErrorCount() const { return errorCount; }
const std::vector<std::string>& ErrorCountListener::getErrors() const { return errors; }
