#include <iostream>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "nfa.h"
#include "dfa.h"
#include "serializer.h"
#include <exception>

int main(int argc, char* argv[]) {
    if (argc < 2) { 
        std::cerr << "Usage: automata <regex>\n"; 
        return 1; 
    }
    
    std::string regex = argv[1];
    
    try {
        if (regex.empty()) {
            std::cerr << "{\"error\":\"Empty regex\"}\n";
            return 1;
        }
        
        Lexer lexer(regex);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parse();

        NFA nfa;
        nfa.build(ast.get());

        DFA dfa;
        dfa.build(nfa);

        Serializer s;
        std::cout << s.serialize(regex, nfa, dfa) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "{\"error\":\"" << e.what() << "\"}\n";
        return 1;
    }
    
    return 0;
}
