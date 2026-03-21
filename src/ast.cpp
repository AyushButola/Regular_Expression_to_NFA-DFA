#include "ast.h"

std::unique_ptr<ASTNode> ASTNode::makeChar(char c) {
    auto node = std::make_unique<ASTNode>();
    node->type = NodeType::CHAR;
    node->value = c;
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeBinary(NodeType t, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r) {
    auto node = std::make_unique<ASTNode>();
    node->type = t;
    node->value = 0;
    node->left = std::move(l);
    node->right = std::move(r);
    return node;
}

std::unique_ptr<ASTNode> ASTNode::makeUnary(NodeType t, std::unique_ptr<ASTNode> child) {
    auto node = std::make_unique<ASTNode>();
    node->type = t;
    node->value = 0;
    node->left = std::move(child);
    return node;
}
