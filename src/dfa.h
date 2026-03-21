#pragma once
#include <vector>
#include <map>
#include <string>
#include "nfa.h"

struct DFAState {
    int id;
    std::vector<int> nfaIds;      // sorted
    bool accept;
    std::map<char, int> transitions;
};

struct DFAStep {
    std::string description;
    int currentDFAState;
    char symbol;
    std::vector<int> moveResult;
    std::vector<int> closureResult;
    int resultDFAState;                // -1 if dead/none
    bool isNewState;
    std::vector<DFAState> snapshot;
};

class DFA {
public:
    std::vector<DFAState> states;
    int startState;
    std::vector<char> alphabet;
    std::vector<DFAStep> steps;

    void build(const NFA& nfa);

private:
    std::vector<int> epsilonClosure(const std::vector<int>& stateIds, const NFA& nfa);
    std::vector<int> move(const std::vector<int>& stateIds, char sym, const NFA& nfa);
};
