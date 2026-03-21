#include "nfa.h"
#include <stdexcept>

int NFA::newState(bool accept) {
    int id = states.size();
    states.push_back({id, accept, {}});
    return id;
}

void NFA::addTransition(int from, char sym, int to) {
    states[from].transitions[sym].push_back(to);
}

void NFA::recordStep(const std::string& desc,
                     const std::vector<int>& highlight,
                     const std::vector<std::tuple<int,int,char>>& highlightEdges,
                     const std::vector<int>& newSt) {
    steps.push_back({desc, highlight, highlightEdges, newSt, states});
}

NFAFragment NFA::build(const ASTNode* node) {
    if (!node) {
        throw std::runtime_error("Cannot build NFA from null ASTNode");
    }
    
    // reset if first call
    states.clear();
    steps.clear();
    startState = -1;
    
    NFAFragment frag = buildInternal(node);
    
    startState = frag.startId;
    states[frag.acceptId].accept = true;
    
    // Spec required: snapshot must copy states with accurate values. 
    // Update the last recorded step to include the exact `accept` field.
    if (!steps.empty()) {
        steps.back().snapshot = states;
    }
    
    return frag;
}

NFAFragment NFA::buildInternal(const ASTNode* node) {
    NFAFragment frag;
    switch (node->type) {
        case NodeType::CHAR: frag = buildChar(node->value); break;
        case NodeType::CONCAT: frag = buildConcat(node); break;
        case NodeType::UNION: frag = buildUnion(node); break;
        case NodeType::STAR: frag = buildStar(node); break;
        case NodeType::PLUS: frag = buildPlus(node); break;
        case NodeType::QUESTION: frag = buildQuestion(node); break;
        default: throw std::runtime_error("Unknown AST node type");
    }
    return frag;
}

NFAFragment NFA::buildChar(char c) {
    int s = newState();
    int e = newState();
    addTransition(s, c, e);
    
    recordStep("CHAR '" + std::string(1, c) + "': create 2 states and transition",
               {s, e}, {{s, e, c}}, {s, e});
               
    return {s, e};
}

NFAFragment NFA::buildConcat(const ASTNode* node) {
    NFAFragment fragA = buildInternal(node->left.get());
    NFAFragment fragB = buildInternal(node->right.get());
    
    addTransition(fragA.acceptId, '\0', fragB.startId);
    
    recordStep("CONCAT: connect A.accept to B.start with ε",
               {fragA.acceptId, fragB.startId},
               {{fragA.acceptId, fragB.startId, '\0'}},
               {});
               
    return {fragA.startId, fragB.acceptId};
}

NFAFragment NFA::buildUnion(const ASTNode* node) {
    NFAFragment fragA = buildInternal(node->left.get());
    NFAFragment fragB = buildInternal(node->right.get());
    
    int s = newState();
    int e = newState();
    
    addTransition(s, '\0', fragA.startId);
    addTransition(s, '\0', fragB.startId);
    addTransition(fragA.acceptId, '\0', e);
    addTransition(fragB.acceptId, '\0', e);
    
    recordStep("UNION: new start and accept states, ε-transitions to A and B",
               {s, e, fragA.startId, fragB.startId, fragA.acceptId, fragB.acceptId},
               {{s, fragA.startId, '\0'}, {s, fragB.startId, '\0'},
                {fragA.acceptId, e, '\0'}, {fragB.acceptId, e, '\0'}},
               {s, e});
               
    return {s, e};
}

NFAFragment NFA::buildStar(const ASTNode* node) {
    NFAFragment fragA = buildInternal(node->left.get());
    
    int s = newState();
    int e = newState();
    
    addTransition(s, '\0', fragA.startId);
    addTransition(s, '\0', e);
    addTransition(fragA.acceptId, '\0', fragA.startId);
    addTransition(fragA.acceptId, '\0', e);
    
    recordStep("STAR: new start and accept states, 4 ε-transitions",
               {s, e, fragA.startId, fragA.acceptId},
               {{s, fragA.startId, '\0'}, {s, e, '\0'},
                {fragA.acceptId, fragA.startId, '\0'}, {fragA.acceptId, e, '\0'}},
               {s, e});
               
    return {s, e};
}

NFAFragment NFA::buildPlus(const ASTNode* node) {
    NFAFragment fragA = buildInternal(node->left.get());
    
    int s = newState();
    int e = newState();
    
    addTransition(s, '\0', fragA.startId);
    addTransition(fragA.acceptId, '\0', fragA.startId);
    addTransition(fragA.acceptId, '\0', e);
    
    recordStep("PLUS: new start and accept states, 3 ε-transitions",
               {s, e, fragA.startId, fragA.acceptId},
               {{s, fragA.startId, '\0'},
                {fragA.acceptId, fragA.startId, '\0'}, {fragA.acceptId, e, '\0'}},
               {s, e});
               
    return {s, e};
}

NFAFragment NFA::buildQuestion(const ASTNode* node) {
    NFAFragment fragA = buildInternal(node->left.get());
    
    int s = newState();
    int e = newState();
    
    addTransition(s, '\0', fragA.startId);
    addTransition(s, '\0', e);
    addTransition(fragA.acceptId, '\0', e);
    
    recordStep("QUESTION: new start and accept states, 3 ε-transitions",
               {s, e, fragA.startId, fragA.acceptId},
               {{s, fragA.startId, '\0'}, {s, e, '\0'},
                {fragA.acceptId, e, '\0'}},
               {s, e});
               
    return {s, e};
}
