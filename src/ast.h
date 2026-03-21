#pragma once
#include <memory>

enum class NodeType { CHAR, CONCAT, UNION, STAR, PLUS, QUESTION };

struct ASTNode {
    NodeType type;
    char value;                           // only for CHAR nodes
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;       // only for CONCAT and UNION

    // Constructors
    static std::unique_ptr<ASTNode> makeChar(char c);
    static std::unique_ptr<ASTNode> makeBinary(NodeType t,
        std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r);
    static std::unique_ptr<ASTNode> makeUnary(NodeType t,
        std::unique_ptr<ASTNode> child);
};
