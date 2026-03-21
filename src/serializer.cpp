#include "serializer.h"
#include <sstream>

std::string Serializer::serialize(const std::string& regex, const NFA& nfa, const DFA& dfa) {
    std::ostringstream os;
    os << "{\n";
    os << "  \"regex\": " << jsonStr(regex) << ",\n";
    
    os << "  \"alphabet\": [";
    for (size_t i = 0; i < dfa.alphabet.size(); ++i) {
        os << jsonStr(std::string(1, dfa.alphabet[i])) << (i + 1 < dfa.alphabet.size() ? ", " : "");
    }
    os << "],\n";
    
    // NFA
    os << "  \"nfa\": {\n";
    os << "    \"startState\": " << nfa.startState << ",\n";
    os << "    \"states\": [\n";
    for (size_t i = 0; i < nfa.states.size(); ++i) {
        os << "      " << serializeNFAState(nfa.states[i]) << (i + 1 < nfa.states.size() ? ",\n" : "\n");
    }
    os << "    ]\n";
    os << "  },\n";
    
    // DFA
    os << "  \"dfa\": {\n";
    os << "    \"startState\": " << dfa.startState << ",\n";
    os << "    \"states\": [\n";
    for (size_t i = 0; i < dfa.states.size(); ++i) {
        os << "      " << serializeDFAState(dfa.states[i]) << (i + 1 < dfa.states.size() ? ",\n" : "\n");
    }
    os << "    ]\n";
    os << "  },\n";
    
    // NFA steps
    os << "  \"nfaSteps\": [\n";
    for (size_t i = 0; i < nfa.steps.size(); ++i) {
        os << serializeNFAStep(nfa.steps[i]) << (i + 1 < nfa.steps.size() ? ",\n" : "\n");
    }
    os << "  ],\n";
    
    // DFA steps
    os << "  \"dfaSteps\": [\n";
    for (size_t i = 0; i < dfa.steps.size(); ++i) {
        os << serializeDFAStep(dfa.steps[i]) << (i + 1 < dfa.steps.size() ? ",\n" : "\n");
    }
    os << "  ]\n";
    
    os << "}\n";
    return os.str();
}

std::string Serializer::jsonInt(int v) {
    return std::to_string(v);
}

std::string Serializer::jsonStr(const std::string& s) {
    std::ostringstream os;
    os << "\"";
    for (char c : s) {
        if (c == '"') os << "\\\"";
        else if (c == '\\') os << "\\\\";
        else if (c == '\n') os << "\\n";
        else if (c == '\r') os << "\\r";
        else if (c == '\t') os << "\\t";
        else os << c;
    }
    os << "\"";
    return os.str();
}

std::string Serializer::jsonIntArray(const std::vector<int>& arr) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        os << arr[i] << (i + 1 < arr.size() ? ", " : "");
    }
    os << "]";
    return os.str();
}

std::string Serializer::serializeNFAState(const NFAState& state) {
    std::ostringstream os;
    os << "{ \"id\": " << state.id << ", \"accept\": " << (state.accept ? "true" : "false") << ", \"transitions\": { ";
    
    bool first = true;
    for (auto it = state.transitions.begin(); it != state.transitions.end(); ++it) {
        if (!first) os << ", ";
        first = false;
        std::string key = (it->first == '\0') ? "ε" : std::string(1, it->first);
        os << jsonStr(key) << ": " << jsonIntArray(it->second);
    }
    
    os << " } }";
    return os.str();
}

std::string Serializer::serializeDFAState(const DFAState& state) {
    std::ostringstream os;
    os << "{ \"id\": " << state.id << ", \"nfaIds\": " << jsonIntArray(state.nfaIds) << ", \"accept\": " << (state.accept ? "true" : "false") << ", \"transitions\": { ";
    
    bool first = true;
    for (auto it = state.transitions.begin(); it != state.transitions.end(); ++it) {
        if (!first) os << ", ";
        first = false;
        os << jsonStr(std::string(1, it->first)) << ": " << it->second;
    }
    
    os << " } }";
    return os.str();
}

std::string Serializer::serializeNFAStep(const NFAStep& step) {
    std::ostringstream os;
    os << "    {\n";
    os << "      \"description\": " << jsonStr(step.description) << ",\n";
    os << "      \"highlightStates\": " << jsonIntArray(step.highlightStates) << ",\n";
    
    os << "      \"highlightEdges\": [";
    for (size_t i = 0; i < step.highlightEdges.size(); ++i) {
        auto e = step.highlightEdges[i];
        std::string symbol = (std::get<2>(e) == '\0') ? "ε" : std::string(1, std::get<2>(e));
        os << "[" << std::get<0>(e) << ", " << std::get<1>(e) << ", " << jsonStr(symbol) << "]" 
           << (i + 1 < step.highlightEdges.size() ? ", " : "");
    }
    os << "],\n";
    
    os << "      \"newStates\": " << jsonIntArray(step.newStates) << ",\n";
    os << "      \"snapshot\": [\n";
    for (size_t i = 0; i < step.snapshot.size(); ++i) {
        os << "        " << serializeNFAState(step.snapshot[i]) << (i + 1 < step.snapshot.size() ? ",\n" : "\n");
    }
    os << "      ]\n";
    os << "    }";
    return os.str();
}

std::string Serializer::serializeDFAStep(const DFAStep& step) {
    std::ostringstream os;
    os << "    {\n";
    os << "      \"description\": " << jsonStr(step.description) << ",\n";
    os << "      \"currentDFAState\": " << step.currentDFAState << ",\n";
    os << "      \"symbol\": " << jsonStr(std::string(1, step.symbol)) << ",\n";
    os << "      \"moveResult\": " << jsonIntArray(step.moveResult) << ",\n";
    os << "      \"closureResult\": " << jsonIntArray(step.closureResult) << ",\n";
    os << "      \"resultDFAState\": " << step.resultDFAState << ",\n";
    os << "      \"isNewState\": " << (step.isNewState ? "true" : "false") << ",\n";
    os << "      \"snapshot\": [\n";
    for (size_t i = 0; i < step.snapshot.size(); ++i) {
        os << "        " << serializeDFAState(step.snapshot[i]) << (i + 1 < step.snapshot.size() ? ",\n" : "\n");
    }
    os << "      ]\n";
    os << "    }";
    return os.str();
}
