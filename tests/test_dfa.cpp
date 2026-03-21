#include <cassert>
#include <iostream>
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/nfa.h"
#include "../src/dfa.h"

bool simulate(const DFA& dfa, const std::string& input) {
    if (dfa.states.empty()) return false;
    int curr = dfa.startState;
    for (char c : input) {
        const auto& state = dfa.states[curr];
        auto it = state.transitions.find(c);
        if (it == state.transitions.end()) return false;
        curr = it->second;
    }
    return dfa.states[curr].accept;
}

void test_dfa() {
    auto buildDFA = [](const std::string& s) {
        Lexer l(s);
        Parser p(l.tokenize());
        auto ast = p.parse();
        NFA nfa;
        nfa.build(ast.get());
        DFA dfa;
        dfa.build(nfa);
        return dfa;
    };

    // a
    DFA dfa1 = buildDFA("a");
    assert(dfa1.steps.size() > 0);
    assert(simulate(dfa1, "a") == true);
    assert(simulate(dfa1, "b") == false);
    assert(simulate(dfa1, "") == false);
    
    // a*
    DFA dfa2 = buildDFA("a*");
    assert(simulate(dfa2, "") == true);
    assert(simulate(dfa2, "a") == true);
    assert(simulate(dfa2, "aaa") == true);
    assert(simulate(dfa2, "b") == false);
    assert(simulate(dfa2, "ba") == false);
    
    // a|b
    DFA dfa3 = buildDFA("a|b");
    assert(simulate(dfa3, "a") == true);
    assert(simulate(dfa3, "b") == true);
    assert(simulate(dfa3, "ab") == false);
    assert(simulate(dfa3, "") == false);
    
    // (a|b)*abb
    DFA dfa4 = buildDFA("(a|b)*abb");
    assert(simulate(dfa4, "abb") == true);
    assert(simulate(dfa4, "aabb") == true);
    assert(simulate(dfa4, "babb") == true);
    assert(simulate(dfa4, "aababb") == true);
    assert(simulate(dfa4, "ab") == false);
    assert(simulate(dfa4, "abba") == false);
    assert(simulate(dfa4, "") == false);

    // Check states exist corresponding to transitions
    for (const auto& state : dfa4.states) {
        for (const auto& pair : state.transitions) {
            int toNode = pair.second;
            assert(toNode >= 0 && toNode < (int)dfa4.states.size());
        }
    }

    std::cout << "PASS: test_dfa\n";
}

int main() {
    test_dfa();
    std::cout << "All tests passed.\n";
    return 0;
}
