#include <cassert>
#include <iostream>
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/nfa.h"

void test_nfa() {
    auto buildNFA = [](const std::string& s) {
        Lexer l(s);
        Parser p(l.tokenize());
        auto ast = p.parse();
        NFA nfa;
        nfa.build(ast.get());
        return nfa;
    };

    // Single char = 2
    NFA nfa1 = buildNFA("a");
    assert(nfa1.states.size() == 2);
    assert(nfa1.steps.size() > 0);

    // a|b = 6
    NFA nfa2 = buildNFA("a|b");
    assert(nfa2.states.size() == 6);
    // UNION node ε-transitions exist
    bool hasEpsilonUnion = false;
    for (const auto& st : nfa2.states) {
        if (st.transitions.count('\0') && st.transitions.at('\0').size() == 2) hasEpsilonUnion = true;
    }
    assert(hasEpsilonUnion);

    // a* = 4
    NFA nfa3 = buildNFA("a*");
    assert(nfa3.states.size() == 4);

    // Accept state logic
    int acceptCount = 0;
    int finalStateId = -1;
    for (const auto& st : nfa3.states) {
        if (st.accept) {
            acceptCount++;
            finalStateId = st.id;
        }
    }
    assert(acceptCount == 1);
    assert(nfa3.states[finalStateId].accept == true);

    std::cout << "PASS: test_nfa\n";
}

int main() {
    test_nfa();
    std::cout << "All tests passed.\n";
    return 0;
}
