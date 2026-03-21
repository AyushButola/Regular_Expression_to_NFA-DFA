#include "dfa.h"
#include <set>
#include <algorithm>
#include <queue>

std::vector<int> DFA::epsilonClosure(const std::vector<int>& stateIds, const NFA& nfa) {
    std::set<int> closure(stateIds.begin(), stateIds.end());
    std::queue<int> q;
    for (int id : stateIds) q.push(id);
    
    while (!q.empty()) {
        int curr = q.front();
        q.pop();
        
        auto it = nfa.states[curr].transitions.find('\0');
        if (it != nfa.states[curr].transitions.end()) {
            for (int next : it->second) {
                if (closure.find(next) == closure.end()) {
                    closure.insert(next);
                    q.push(next);
                }
            }
        }
    }
    
    std::vector<int> result(closure.begin(), closure.end());
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<int> DFA::move(const std::vector<int>& stateIds, char sym, const NFA& nfa) {
    std::set<int> dests;
    for (int id : stateIds) {
        auto it = nfa.states[id].transitions.find(sym);
        if (it != nfa.states[id].transitions.end()) {
            for (int next : it->second) {
                dests.insert(next);
            }
        }
    }
    
    std::vector<int> result(dests.begin(), dests.end());
    std::sort(result.begin(), result.end());
    return result;
}

void DFA::build(const NFA& nfa) {
    states.clear();
    steps.clear();
    alphabet.clear();
    
    if (nfa.states.empty()) return;
    
    // 1. Collect alphabet
    std::set<char> alphaSet;
    for (const auto& state : nfa.states) {
        for (const auto& pair : state.transitions) {
            if (pair.first != '\0') {
                alphaSet.insert(pair.first);
            }
        }
    }
    alphabet.assign(alphaSet.begin(), alphaSet.end());
    std::sort(alphabet.begin(), alphabet.end());
    
    // 2. Start state
    std::vector<int> startSet = epsilonClosure({nfa.startState}, nfa);
    
    std::map<std::vector<int>, int> stateMap; // NFA id set -> DFA state id
    
    auto createDFAState = [&](const std::vector<int>& nfaIds) {
        int id = states.size();
        bool accept = false;
        for (int nid : nfaIds) {
            if (nfa.states[nid].accept) {
                accept = true;
                break;
            }
        }
        states.push_back({id, nfaIds, accept, {}});
        stateMap[nfaIds] = id;
        return id;
    };
    
    startState = createDFAState(startSet);
    
    // 4. Worklist
    size_t unmarked = 0;
    while (unmarked < states.size()) {
        int currId = unmarked++;
        std::vector<int> currNfaIds = states[currId].nfaIds;
        
        for (char sym : alphabet) {
            std::vector<int> mRes = move(currNfaIds, sym, nfa);
            if (mRes.empty()) continue;
            
            std::vector<int> closRes = epsilonClosure(mRes, nfa);
            
            int nextId = -1;
            bool isNew = false;
            auto it = stateMap.find(closRes);
            if (it != stateMap.end()) {
                nextId = it->second;
            } else {
                nextId = createDFAState(closRes);
                isNew = true;
            }
            
            states[currId].transitions[sym] = nextId;
            
            std::string desc = "From D" + std::to_string(currId) + " on '" + std::string(1, sym) + "': " +
                               "move={";
            for (size_t i = 0; i < mRes.size(); ++i) {
                desc += "q" + std::to_string(mRes[i]) + (i + 1 < mRes.size() ? "," : "");
            }
            desc += "} ε-closure={";
            for (size_t i = 0; i < closRes.size(); ++i) {
                desc += "q" + std::to_string(closRes[i]) + (i + 1 < closRes.size() ? "," : "");
            }
            desc += "} → D" + std::to_string(nextId) + (isNew ? " (new)" : "");

            steps.push_back({
                desc,
                currId,
                sym,
                mRes,
                closRes,
                nextId,
                isNew,
                states
            });
        }
    }
}
