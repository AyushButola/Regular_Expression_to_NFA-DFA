#include <cassert>
#include <iostream>
#include <stdexcept>
#include "../src/lexer.h"

void test_lexer() {
    // Sequence for (a|b)*abb
    Lexer l1("(a|b)*abb");
    auto t1 = l1.tokenize();
    // LPAREN CHAR(a) UNION CHAR(b) RPAREN STAR CONCAT CHAR(a) CONCAT CHAR(b) CONCAT CHAR(b)
    assert(t1.size() == 12);
    assert(t1[0].type == TokenType::LPAREN);
    assert(t1[1].type == TokenType::CHAR && t1[1].value == 'a');
    assert(t1[2].type == TokenType::UNION);
    assert(t1[3].type == TokenType::CHAR && t1[3].value == 'b');
    assert(t1[4].type == TokenType::RPAREN);
    assert(t1[5].type == TokenType::STAR);
    assert(t1[6].type == TokenType::CONCAT);
    assert(t1[7].type == TokenType::CHAR && t1[7].value == 'a');
    assert(t1[8].type == TokenType::CONCAT);
    assert(t1[9].type == TokenType::CHAR && t1[9].value == 'b');
    assert(t1[10].type == TokenType::CONCAT);
    assert(t1[11].type == TokenType::CHAR && t1[11].value == 'b');
    
    // Concatenation insertion: ab
    Lexer l2("ab");
    auto t2 = l2.tokenize();
    assert(t2.size() == 3);
    assert(t2[1].type == TokenType::CONCAT);
    
    // )*a
    Lexer l3(")*a");
    auto t3 = l3.tokenize();
    assert(t3.size() == 4);
    assert(t3[2].type == TokenType::CONCAT);
    
    // Empty string
    Lexer l4("");
    auto t4 = l4.tokenize();
    assert(t4.empty());
    
    // Single character a
    Lexer l5("a");
    auto t5 = l5.tokenize();
    assert(t5.size() == 1);
    assert(t5[0].type == TokenType::CHAR && t5[0].value == 'a');
    
    // | alone
    Lexer l6("|");
    auto t6 = l6.tokenize();
    assert(t6.size() == 1);
    assert(t6[0].type == TokenType::UNION);
    
    // Unrecognized character
    try {
        Lexer("\x01").tokenize();
        assert(false);
    } catch (const std::runtime_error&) {}
    
    std::cout << "PASS: test_lexer\n";
}

int main() {
    test_lexer();
    std::cout << "All tests passed.\n";
    return 0;
}
