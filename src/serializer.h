#pragma once
#include <string>
#include "nfa.h"
#include "dfa.h"

class Serializer {
public:
    std::string serialize(const std::string& regex, const NFA& nfa, const DFA& dfa);

private:
    std::string jsonInt(int v);
    std::string jsonStr(const std::string& s);
    std::string jsonIntArray(const std::vector<int>& arr);
    std::string serializeNFAState(const NFAState& state);
    std::string serializeDFAState(const DFAState& state);
    std::string serializeNFAStep(const NFAStep& step);
    std::string serializeDFAStep(const DFAStep& step);
};
