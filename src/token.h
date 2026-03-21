#pragma once

enum class TokenType {
    CHAR,        // any literal character
    UNION,       // |
    CONCAT,      // · (inserted, not in source)
    STAR,        // *
    PLUS,        // +
    QUESTION,    // ?
    LPAREN,      // (
    RPAREN       // )
};

struct Token {
    TokenType type;
    char value; // only meaningful for CHAR
};
