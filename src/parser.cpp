#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<ASTNode> Parser::parse() {
    if (tokens_.empty()) {
        return nullptr;
    }
    auto node = expr();
    if (pos_ < tokens_.size()) {
        throw std::runtime_error("Unexpected token at end of input");
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::expr() {
    auto node = concat();
    while (match(TokenType::UNION)) {
        consume(); // consume UNION
        auto right = concat();
        node = ASTNode::makeBinary(NodeType::UNION, std::move(node), std::move(right));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::concat() {
    auto node = repeat();
    while (match(TokenType::CONCAT)) {
        consume(); // consume CONCAT
        auto right = repeat();
        node = ASTNode::makeBinary(NodeType::CONCAT, std::move(node), std::move(right));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::repeat() {
    auto node = atom();
    if (match(TokenType::STAR)) {
        consume();
        node = ASTNode::makeUnary(NodeType::STAR, std::move(node));
    } else if (match(TokenType::PLUS)) {
        consume();
        node = ASTNode::makeUnary(NodeType::PLUS, std::move(node));
    } else if (match(TokenType::QUESTION)) {
        consume();
        node = ASTNode::makeUnary(NodeType::QUESTION, std::move(node));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::atom() {
    Token t = peek();
    if (t.type == TokenType::CHAR) {
        consume();
        return ASTNode::makeChar(t.value);
    } else if (t.type == TokenType::LPAREN) {
        consume();
        auto node = expr();
        if (!match(TokenType::RPAREN)) {
            throw std::runtime_error("Expected closing parenthesis");
        }
        consume();
        return node;
    }
    throw std::runtime_error("Unexpected token in atom");
}

Token Parser::peek() const {
    if (pos_ >= tokens_.size()) {
        return Token{TokenType::RPAREN, 0}; // sentinel
    }
    return tokens_[pos_];
}

Token Parser::consume() {
    if (pos_ >= tokens_.size()) {
        throw std::runtime_error("Unexpected end of input");
    }
    return tokens_[pos_++];
}

bool Parser::match(TokenType t) {
    if (pos_ >= tokens_.size()) {
        return false;
    }
    return tokens_[pos_].type == t;
}
