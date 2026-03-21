#include <emscripten/bind.h>
#include "lexer.h"
#include "parser.h"
#include "nfa.h"
#include "dfa.h"
#include "serializer.h"
#include <exception>

std::string processRegex(const std::string& regex) {
    try {
        if (regex.empty()) {
            return "{\"error\":\"Empty regex\"}";
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
        return s.serialize(regex, nfa, dfa);
    } catch (const std::exception& e) {
        return std::string("{\"error\":\"") + e.what() + "\"}";
    }
}

EMSCRIPTEN_BINDINGS(automata_module) {
    emscripten::function("processRegex", &processRegex);
}
