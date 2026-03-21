#include "lexer.h"
#include <stdexcept>

Lexer::Lexer(const std::string& input) : input_(input) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    // First pass: scan characters
    for (char c : input_) {
        if (c < 32 || c > 126) {
            throw std::runtime_error("Unrecognized character: " + std::string(1, c));
        }
        
        switch (c) {
            case '|': tokens.push_back({TokenType::UNION, 0}); break;
            case '*': tokens.push_back({TokenType::STAR, 0}); break;
            case '+': tokens.push_back({TokenType::PLUS, 0}); break;
            case '?': tokens.push_back({TokenType::QUESTION, 0}); break;
            case '(': tokens.push_back({TokenType::LPAREN, 0}); break;
            case ')': tokens.push_back({TokenType::RPAREN, 0}); break;
            default:  tokens.push_back({TokenType::CHAR, c}); break;
        }
    }
    
    if (tokens.empty()) return tokens;

    // Second pass: insert CONCAT tokens
    std::vector<Token> finalTokens;
    finalTokens.push_back(tokens[0]);
    
    for (size_t i = 1; i < tokens.size(); ++i) {
        TokenType left = tokens[i - 1].type;
        TokenType right = tokens[i].type;
        
        bool leftCanConcat = (left == TokenType::CHAR || left == TokenType::RPAREN ||
                              left == TokenType::STAR || left == TokenType::PLUS ||
                              left == TokenType::QUESTION);
        
        bool rightCanConcat = (right == TokenType::CHAR || right == TokenType::LPAREN);
        
        if (leftCanConcat && rightCanConcat) {
            finalTokens.push_back({TokenType::CONCAT, 0});
        }
        
        finalTokens.push_back(tokens[i]);
    }
    
    return finalTokens;
}
