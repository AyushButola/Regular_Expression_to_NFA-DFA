#include <cassert>
#include <iostream>
#include <stdexcept>
#include "../src/lexer.h"
#include "../src/parser.h"

void test_parser() {
    auto parseStr = [](const std::string& s) {
        Lexer l(s);
        Parser p(l.tokenize());
        return p.parse();
    };

    // a|b -> UNION
    auto ast1 = parseStr("a|b");
    assert(ast1->type == NodeType::UNION);
    assert(ast1->left->type == NodeType::CHAR && ast1->left->value == 'a');
    assert(ast1->right->type == NodeType::CHAR && ast1->right->value == 'b');

    // ab -> CONCAT
    auto ast2 = parseStr("ab");
    assert(ast2->type == NodeType::CONCAT);
    assert(ast2->left->type == NodeType::CHAR && ast2->left->value == 'a');
    assert(ast2->right->type == NodeType::CHAR && ast2->right->value == 'b');

    // a* -> STAR
    auto ast3 = parseStr("a*");
    assert(ast3->type == NodeType::STAR);
    assert(ast3->left->type == NodeType::CHAR && ast3->left->value == 'a');

    // (a|b)*abb
    auto ast4 = parseStr("(a|b)*abb");
    assert(ast4->type == NodeType::CONCAT);

    // Unmatched (a -> throws
    try {
        parseStr("(a");
        assert(false);
    } catch (const std::runtime_error&) {}

    // Trailing operator a| -> throws
    try {
        parseStr("a|");
        assert(false);
    } catch (const std::runtime_error&) {}

    // *a -> throws
    try {
        parseStr("*a");
        assert(false);
    } catch (const std::runtime_error&) {}

    std::cout << "PASS: test_parser\n";
}

int main() {
    test_parser();
    std::cout << "All tests passed.\n";
    return 0;
}
