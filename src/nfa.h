#pragma once
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include "ast.h"

struct NFAState {
    int id;
    bool accept;
    std::map<char, std::vector<int>> transitions;
};

struct NFAFragment {
    int startId;
    int acceptId;
};

struct NFAStep {
    std::string description;
    std::vector<int> highlightStates;
    std::vector<std::tuple<int,int,char>> highlightEdges; // (from, to, symbol)
    std::vector<int> newStates;
    std::vector<NFAState> snapshot;
};

class NFA {
public:
    std::vector<NFAState> states;
    int startState;
    std::vector<NFAStep> steps;

    NFAFragment build(const ASTNode* node);

private:
    NFAFragment buildInternal(const ASTNode* node);
    int newState(bool accept = false);
    void addTransition(int from, char sym, int to);
    void recordStep(const std::string& desc,
                    const std::vector<int>& highlight,
                    const std::vector<std::tuple<int,int,char>>& highlightEdges,
                    const std::vector<int>& newSt);

    NFAFragment buildChar(char c);
    NFAFragment buildConcat(const ASTNode* node);
    NFAFragment buildUnion(const ASTNode* node);
    NFAFragment buildStar(const ASTNode* node);
    NFAFragment buildPlus(const ASTNode* node);
    NFAFragment buildQuestion(const ASTNode* node);
};
