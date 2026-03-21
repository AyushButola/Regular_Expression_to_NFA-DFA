#pragma once
#include "ast.h"
#include "token.h"
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ASTNode> parse();

private:
    std::unique_ptr<ASTNode> expr();
    std::unique_ptr<ASTNode> concat();
    std::unique_ptr<ASTNode> repeat();
    std::unique_ptr<ASTNode> atom();
    
    Token peek() const;
    Token consume();
    bool match(TokenType t);

    std::vector<Token> tokens_;
    size_t pos_;
};
