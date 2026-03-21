# Regex → NFA → DFA Converter (C++ + WebAssembly)

## Project Overview

Build a web application that converts a regular expression into an NFA (via Thompson's Construction) and then into a DFA (via Subset Construction), with interactive step-by-step visualization in the browser. The C++ core compiles to WebAssembly; the frontend is plain HTML/JS/CSS with no frameworks.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Core logic | C++17 |
| Browser runtime | WebAssembly (via Emscripten) |
| JS/WASM glue | Emscripten `--bind` + `MODULARIZE` |
| Frontend rendering | HTML5 Canvas or SVG |
| Build system | CMake + Emscripten (`em++`) |
| Local dev server | Python `http.server` or `emrun` |

---

## Project Structure

```
regex-automata/
├── src/
│   ├── token.h
│   ├── token.cpp
│   ├── lexer.h
│   ├── lexer.cpp
│   ├── ast.h
│   ├── ast.cpp
│   ├── parser.h
│   ├── parser.cpp
│   ├── nfa.h
│   ├── nfa.cpp
│   ├── dfa.h
│   ├── dfa.cpp
│   ├── serializer.h
│   ├── serializer.cpp
│   ├── binding.cpp
│   └── main.cpp
├── web/
│   ├── index.html
│   ├── app.js
│   ├── renderer.js
│   └── style.css
├── tests/
│   ├── test_lexer.cpp
│   ├── test_parser.cpp
│   ├── test_nfa.cpp
│   └── test_dfa.cpp
├── CMakeLists.txt
├── build.sh
└── README.md
```

---

## Pipeline

```
Regex string
    ↓  [Lexer]         tokenize + insert explicit concatenation operator
Token list
    ↓  [Parser]        recursive descent → Abstract Syntax Tree
AST
    ↓  [Thompson's]    walk AST recursively → NFA fragments composed with ε-transitions
NFA
    ↓  [Subset Const.] worklist algorithm → DFA states = sets of NFA states
DFA
    ↓  [Serializer]    serialize NFA, DFA, and all construction steps to JSON
JSON string
    ↓  [WASM binding]  returned to JavaScript via Emscripten
Frontend
    ↓  [Step player]   parse JSON, render graph, play through steps
```

---

## Module Specifications

### 1. Lexer (`token.h`, `lexer.h / lexer.cpp`)

**Responsibility:** Scan the raw regex string and emit a flat list of tokens. Insert explicit concatenation operators (`·`) so the parser handles sequencing uniformly.

**Token types:**
```cpp
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
```

**Concatenation insertion rule:** Insert a `CONCAT` token between position `i` and `i+1` if:
- Left token is one of: `CHAR`, `RPAREN`, `STAR`, `PLUS`, `QUESTION`
- Right token is one of: `CHAR`, `LPAREN`

**Example:**
```
Input:  "(a|b)*abb"
Output: LPAREN CHAR(a) UNION CHAR(b) RPAREN STAR CONCAT CHAR(a) CONCAT CHAR(b) CONCAT CHAR(b)
```

**Error handling:** Throw `std::runtime_error` on unrecognized characters.

---

### 2. AST (`ast.h / ast.cpp`)

**Responsibility:** Define the tree node structure used by the parser.

```cpp
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
```

---

### 3. Parser (`parser.h / parser.cpp`)

**Responsibility:** Consume the token list and build an AST using recursive descent.

**Grammar (lowest to highest precedence):**
```
expr    → concat ( UNION concat )*
concat  → repeat ( CONCAT repeat )*
repeat  → atom ( STAR | PLUS | QUESTION )?
atom    → CHAR | LPAREN expr RPAREN
```

**Interface:**
```cpp
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ASTNode> parse();  // throws on syntax error
private:
    std::unique_ptr<ASTNode> expr();
    std::unique_ptr<ASTNode> concat();
    std::unique_ptr<ASTNode> repeat();
    std::unique_ptr<ASTNode> atom();
    Token peek() const;
    Token consume();
    bool match(TokenType t);
    // ...
};
```

**Error handling:** Throw `std::runtime_error` with a message including the position of the unexpected token.

---

### 4. NFA + Thompson's Construction (`nfa.h / nfa.cpp`)

**Responsibility:** Walk the AST and produce an NFA. Record each construction step.

**Data structures:**
```cpp
struct NFAState {
    int id;
    bool accept;
    std::map<char, std::vector<int>> transitions;
    // Use '\0' as the epsilon symbol key
};

struct NFAFragment {
    int startId;
    int acceptId;
};

struct NFAStep {
    std::string description;        // human-readable explanation
    std::vector<int> highlightStates;
    std::vector<std::tuple<int,int,char>> highlightEdges; // (from, to, symbol)
    std::vector<int> newStates;
    // full NFA snapshot at this point (all states + transitions)
    std::vector<NFAState> snapshot;
};

class NFA {
public:
    std::vector<NFAState> states;
    int startState;
    std::vector<NFAStep> steps;      // populated during build

    NFAFragment build(const ASTNode* node);  // recursive Thompson's
private:
    int newState(bool accept = false);
    void addTransition(int from, char sym, int to);
    void recordStep(const std::string& desc,
                    const std::vector<int>& highlight,
                    const std::vector<int>& newSt);

    NFAFragment buildChar(char c);
    NFAFragment buildConcat(const ASTNode* node);
    NFAFragment buildUnion(const ASTNode* node);
    NFAFragment buildStar(const ASTNode* node);
    NFAFragment buildPlus(const ASTNode* node);
    NFAFragment buildQuestion(const ASTNode* node);
};
```

**Thompson's rules:**

| Node | Construction |
|---|---|
| `CHAR(c)` | Two states `s→e` connected by label `c`. |
| `CONCAT(A,B)` | Connect A's accept to B's start with ε. |
| `UNION(A,B)` | New start with ε→A.start and ε→B.start; A.accept and B.accept each ε→new accept. |
| `STAR(A)` | New start with ε→A.start and ε→new accept; A.accept with ε→A.start and ε→new accept. |
| `PLUS(A)` | New start with ε→A.start; A.accept with ε→A.start and ε→new accept. |
| `QUESTION(A)` | New start with ε→A.start and ε→new accept; A.accept with ε→new accept. |

**Step recording:** Call `recordStep()` after each fragment is assembled. The step must include a snapshot of all current states so the frontend can replay the graph at any point in time.

---

### 5. DFA + Subset Construction (`dfa.h / dfa.cpp`)

**Responsibility:** Convert the NFA to a DFA using the worklist algorithm. Record each step.

**Data structures:**
```cpp
struct DFAState {
    int id;
    std::vector<int> nfaIds;      // the NFA state set this DFA state represents
    bool accept;
    std::map<char, int> transitions;  // symbol → DFA state id
};

struct DFAStep {
    std::string description;
    int currentDFAState;
    char symbol;
    std::vector<int> moveResult;       // raw NFA states after move()
    std::vector<int> closureResult;    // after ε-closure
    int resultDFAState;                // -1 if dead/none
    bool isNewState;
    std::vector<DFAState> snapshot;    // full DFA at this point
};

class DFA {
public:
    std::vector<DFAState> states;
    int startState;
    std::vector<char> alphabet;
    std::vector<DFAStep> steps;

    void build(const NFA& nfa);
private:
    std::vector<int> epsilonClosure(const std::vector<int>& stateIds,
                                    const NFA& nfa);
    std::vector<int> move(const std::vector<int>& stateIds,
                          char sym, const NFA& nfa);
    int findOrCreate(const std::vector<int>& nfaSet);
};
```

**Algorithm:**
1. Compute the alphabet from all non-ε transitions in the NFA.
2. Start state = ε-closure({NFA.startState}).
3. Worklist loop: for each unmarked DFA state, for each symbol in alphabet, compute `move` then `ε-closure`. If the result is non-empty and not seen before, create a new DFA state.
4. Any DFA state whose NFA set contains the NFA's accept state is a DFA accept state.
5. Record a `DFAStep` for each iteration of the inner loop.

---

### 6. Serializer (`serializer.h / serializer.cpp`)

**Responsibility:** Convert the NFA, DFA, and all steps into a single JSON string. Use a lightweight hand-rolled JSON serializer (no external dependencies).

**Output schema:**
```json
{
  "regex": "(a|b)*abb",
  "alphabet": ["a", "b"],
  "nfa": {
    "startState": 0,
    "states": [
      { "id": 0, "accept": false, "transitions": { "a": [1], "\u0000": [2, 3] } }
    ]
  },
  "dfa": {
    "startState": 0,
    "states": [
      { "id": 0, "nfaIds": [0, 1], "accept": false, "transitions": { "a": 1, "b": 2 } }
    ]
  },
  "nfaSteps": [
    {
      "description": "CHAR 'a': created states q0 → q1",
      "highlightStates": [0, 1],
      "highlightEdges": [[0, 1, "a"]],
      "newStates": [0, 1],
      "snapshot": { ... }
    }
  ],
  "dfaSteps": [
    {
      "description": "From D0 on 'a': move={q1,q3} ε-closure={q1,q3,q5} → D1 (new)",
      "currentDFAState": 0,
      "symbol": "a",
      "moveResult": [1, 3],
      "closureResult": [1, 3, 5],
      "resultDFAState": 1,
      "isNewState": true,
      "snapshot": { ... }
    }
  ]
}
```

---

### 7. Emscripten Binding (`binding.cpp`)

**Responsibility:** Expose `processRegex` to JavaScript. All error handling must be caught here — never let exceptions propagate to JS.

```cpp
#include <emscripten/bind.h>
#include "lexer.h"
#include "parser.h"
#include "nfa.h"
#include "dfa.h"
#include "serializer.h"

std::string processRegex(const std::string& regex) {
    try {
        Lexer lexer(regex);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parse();

        NFA nfa;
        nfa.build(ast.get());

        DFA dfa;
        dfa.build(nfa);

        Serializer s;
        return s.serialize(regex, nfa, dfa);
    } catch (const std::exception& e) {
        return std::string("{\"error\":\"") + e.what() + "\"}";
    }
}

EMSCRIPTEN_BINDINGS(automata_module) {
    emscripten::function("processRegex", &processRegex);
}
```

**Build command:**
```bash
em++ -O2 -std=c++17 \
  src/lexer.cpp src/parser.cpp src/ast.cpp \
  src/nfa.cpp src/dfa.cpp src/serializer.cpp src/binding.cpp \
  -o web/automata.js \
  --bind \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=AutomataModule \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ENVIRONMENT=web
```

---

### 8. CLI Entry Point (`main.cpp`)

**Responsibility:** Allow testing the full pipeline from the command line without the browser. Useful for debugging before WASM integration.

```cpp
int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: automata <regex>\n"; return 1; }
    // run pipeline, print JSON to stdout
}
```

---

### 9. Frontend (`web/`)

#### `index.html`
- Input field for the regex string
- "Convert" button
- Tab switcher: NFA | DFA | Steps
- Canvas or SVG element for graph rendering
- Step player controls: Prev / Play / Next / slider
- Step description panel (shows the current step's `description` string)

#### `app.js`

```js
// Load WASM
const Module = await AutomataModule();

function convert() {
    const regex = document.getElementById('regex-input').value;
    const json = Module.processRegex(regex);
    const result = JSON.parse(json);
    if (result.error) { showError(result.error); return; }
    window.automataData = result;
    renderStep('nfa', 0);
}

// Step player state
let currentPhase = 'nfa';   // 'nfa' or 'dfa'
let currentStep = 0;
let playing = false;

function renderStep(phase, stepIndex) {
    const steps = phase === 'nfa'
        ? window.automataData.nfaSteps
        : window.automataData.dfaSteps;
    const step = steps[stepIndex];
    updateDescription(step.description);
    renderer.draw(step.snapshot, step.highlightStates, step.highlightEdges);
    updateStepCounter(stepIndex, steps.length);
}
```

#### `renderer.js`

**Graph layout:** Use BFS from the start state. Assign each state a column equal to its BFS depth, distribute states with equal depth vertically. This gives a left-to-right layout that works well for automata.

**Drawing:**
- States: circles of radius 22px. Blue fill = start, green fill = accept, double ring for accept, gray for normal.
- Self-loops: bezier curve looping above the state.
- Transitions between different states: straight lines; if a reverse edge exists, curve both.
- Arrow labels: placed at the midpoint of the edge, offset perpendicular to avoid overlap.
- Highlighted states (from current step): amber fill.
- Highlighted edges: purple stroke.
- New states (just created this step): animated fade-in.

**Canvas sizing:** Compute required width and height from the layout, then set canvas dimensions accordingly. Wrap in a scrollable div.

---

## Build Instructions

### Prerequisites
- Emscripten SDK (latest)
- CMake 3.20+
- A C++17-capable compiler (for native tests)

### Native build (for testing)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
./automata "(a|b)*abb"
```

### WASM build
```bash
source /path/to/emsdk/emsdk_env.sh
bash build.sh
```

### Run locally
```bash
cd web && python3 -m http.server 8080
# open http://localhost:8080
```

---

## Supported Regex Syntax

| Syntax | Meaning |
|---|---|
| `a` | Literal character |
| `ab` | Concatenation (a then b) |
| `a\|b` | Union (a or b) |
| `a*` | Kleene star (zero or more) |
| `a+` | One or more |
| `a?` | Zero or one |
| `(ab)` | Grouping |

Operator precedence (highest to lowest): `*` `+` `?` → concatenation → `|`

---

## Testing Requirements

Each module must have a corresponding test file in `tests/`. Tests use plain `assert()` — no external test framework required.

### Lexer tests (`test_lexer.cpp`)
- Correct token sequence for `(a|b)*abb`
- Concatenation insertion between `)*` and `a`
- Empty string input
- Single character input
- Error on unsupported character

### Parser tests (`test_parser.cpp`)
- AST shape for `a|b` (UNION node with two CHAR children)
- AST shape for `ab` (CONCAT node)
- AST shape for `a*` (STAR node)
- Nested groups `(a|b)*`
- Error on unmatched parenthesis
- Error on trailing operator `a|`

### NFA tests (`test_nfa.cpp`)
- State count for single char (2 states)
- State count for `a|b` (6 states)
- ε-transitions exist for UNION and STAR
- Accept state correctly placed
- Steps vector is non-empty after build

### DFA tests (`test_dfa.cpp`)
- DFA for `a` accepts "a", rejects "b", rejects ""
- DFA for `a*` accepts "", "a", "aaa", rejects "b"
- DFA for `(a|b)*abb` accepts "abb", "aabb", "babb", rejects "ab", "abba"
- Steps vector is non-empty after build

---

## Constraints and Notes for Agents

- Do not use any external C++ libraries beyond the standard library and Emscripten headers.
- Do not use `new`/`delete` directly — use `std::unique_ptr` and `std::vector`.
- Use `'\0'` as the epsilon symbol internally; serialize it as `"ε"` in JSON output.
- Every public function that can fail must throw `std::runtime_error` with a descriptive message.
- All JSON serialization must be hand-written (no nlohmann/json or similar).
- The frontend must load `automata.js` and `automata.wasm` from the same directory — no CDN dependencies for the WASM module itself.
- The step player must not re-run the C++ pipeline on every step — parse the JSON once and cache the result in `window.automataData`.
- Canvas rendering must work on both light and dark OS color schemes (`prefers-color-scheme` media query).
- All C++ code must compile cleanly with `-Wall -Wextra` and zero warnings.

---

## Deliverables Checklist

- [ ] `src/token.h` — token types and struct
- [ ] `src/lexer.h` + `src/lexer.cpp` — tokenizer with concat insertion
- [ ] `src/ast.h` + `src/ast.cpp` — AST node types and factory methods
- [ ] `src/parser.h` + `src/parser.cpp` — recursive descent parser
- [ ] `src/nfa.h` + `src/nfa.cpp` — Thompson's construction with step recording
- [ ] `src/dfa.h` + `src/dfa.cpp` — subset construction with step recording
- [ ] `src/serializer.h` + `src/serializer.cpp` — JSON output
- [ ] `src/binding.cpp` — Emscripten JS bindings
- [ ] `src/main.cpp` — CLI entry point
- [ ] `tests/test_lexer.cpp` — lexer unit tests
- [ ] `tests/test_parser.cpp` — parser unit tests
- [ ] `tests/test_nfa.cpp` — NFA unit tests
- [ ] `tests/test_dfa.cpp` — DFA unit tests
- [ ] `CMakeLists.txt` — CMake build for native + WASM targets
- [ ] `build.sh` — Emscripten build script
- [ ] `web/index.html` — UI layout
- [ ] `web/app.js` — WASM loading, step player logic
- [ ] `web/renderer.js` — graph layout and canvas drawing
- [ ] `web/style.css` — styling