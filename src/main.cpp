#include "listener.h"
#include "generated/Lua55GrammarLexer.h"
#include "generated/Lua55GrammarParser.h"
#include "generated/Lua55GrammarListener.h"
#include "Compiler.h"
#include "Interpreter.h"
#include <iomanip>
#include <filesystem>

std::string red_color = "\e[31m";
std::string green_color = "\e[32m";
std::string blue_color = "\e[34m";
std::string normal_color = "\e[m";

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Filepath is expected" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "No file with such name exists" << std::endl;
        return 1;
    }
    if (!std::filesystem::is_regular_file(filepath)) {
        std::cerr << "Input must be a regular file" << std::endl;
        return 1;
    }
    std::ifstream fd(filepath);

    std::string input;
    std::string line;
    while (std::getline(fd, line)) {
        input += line + "\n";
    }

    antlr4::ANTLRInputStream stream(input);
    
    Lua55GrammarLexer lexer(&stream);    
    antlr4::CommonTokenStream tokens(&lexer);
    
    Lua55GrammarParser parser(&tokens);    

    parser.removeErrorListeners();
    ErrorCountListener errorer;
    parser.addErrorListener(&errorer);

    antlr4::tree::ParseTree* tree = parser.prog();

    if (errorer.hasErrors()) {
        for (auto &e: errorer.getErrors()) {
            std::cerr << e << std::endl;
        }
        std::cerr
            << red_color
            << "Parsing error"
            << normal_color 
        << std::endl;
        return 1;
    }
    
    // std::cout << tree->toStringTree() << std::endl;
    std::cout
        << std::endl 
        << blue_color 
        << "Constructing AST"
        << normal_color 
    << std::endl;

    Lua55Listener listener;
    // Lua55GrammarBaseListener listener;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
    auto ast = listener.ast;
    
    

    std::cout
        << std::endl 
        << green_color 
        << "AST contructed"
        << normal_color 
    << std::endl;

    ast->print(std::cout, 0);
    std::cout << std::endl;
    
    std::cout
        << std::endl 
        << blue_color 
        << "Analyzing..."
        << normal_color 
    << std::endl;

    Compiler compiler(ast);

    std::cout
        << std::endl 
        << blue_color 
        << "Static analyzation passed"
        << normal_color 
    << std::endl;

    std::cout
        << std::endl 
        << blue_color 
        << "Compiling..."
        << normal_color 
    << std::endl;

    auto bytecode = compiler.compile(ast);

    std::cout 
        << std::endl 
        << green_color
        << "Compilation passed"
        << normal_color 
    << std::endl;

    for (size_t i=0; i<bytecode.size(); i++) {
        std::cout << std::setw(4) << i << ": " << bytecode[i] << std::endl;
    }

    std::cout 
        << std::endl 
        << blue_color
        << "Setting up interpreter"
        << normal_color 
    << std::endl;

    LuaInterpreter::Interpreter interp(bytecode);

    std::cout 
        << std::endl 
        << blue_color
        << "Interpreting..."
        << normal_color 
    << std::endl;

    auto result = interp.run();

    if (!result) {
        std::cerr 
            << std::endl 
            << red_color
            << "Interpretation ended with an error"
            << normal_color 
        << std::endl;
        return 1;
    }

    std::cout 
        << std::endl 
        << green_color
        << "Interpretation passed"
        << normal_color 
    << std::endl;

    std::cout 
        << std::endl 
        << green_color
        << "Success"
        << normal_color 
    << std::endl;

    return 0;
}