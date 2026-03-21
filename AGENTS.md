# Agent Orchestration Plan — Regex → NFA → DFA Converter

> Reference spec: `regex-automata-spec.md`
> All agents must read the spec in full before starting work.
> All C++ must compile with `-std=c++17 -Wall -Wextra` and zero warnings.
> All agents must verify their deliverables against the checklist in the spec before marking a task done.

---

## Agent Roster

| Agent | Codename | Responsibility |
|---|---|---|
| A1 | `agent-lexer-parser` | Token types, lexer, AST, parser |
| A2 | `agent-nfa` | Thompson's NFA construction + step recording |
| A3 | `agent-dfa` | Subset construction DFA + step recording |
| A4 | `agent-serializer` | JSON serialization of NFA, DFA, and all steps |
| A5 | `agent-binding` | Emscripten WASM binding + CMakeLists + build.sh |
| A6 | `agent-frontend` | HTML, app.js, renderer.js, style.css |
| A7 | `agent-test` | All unit tests across all modules |
| A8 | `agent-integrator` | Final wiring, integration testing, README |

---

## Dependency Graph

```
A1 (lexer + parser)
    └── A2 (NFA)
            └── A3 (DFA)
                    └── A4 (serializer)
                                └── A5 (binding + build)
                                            └── A8 (integrator)
A6 (frontend) ─────────────────────────────────┘
A7 (tests) ── depends on A1, A2, A3 (can run in parallel once each is done)
```

**Parallel work is possible in these groups:**
- Group 1 (sequential): A1 → A2 → A3 → A4 → A5
- Group 2 (parallel with Group 1): A6 can start immediately using the mock JSON schema from the spec
- Group 3 (parallel per module): A7 writes tests as soon as each upstream agent finishes

---

## Agent A1 — `agent-lexer-parser`

### Role
Build the tokenization and parsing layer. This is the entry point of the entire pipeline. All downstream agents depend on the types defined here.

### Reads
- `regex-automata-spec.md` → sections: Lexer, AST, Parser

### Produces
```
src/token.h
src/lexer.h
src/lexer.cpp
src/ast.h
src/ast.cpp
src/parser.h
src/parser.cpp
```

### Detailed Instructions

**token.h**
- Define `TokenType` enum class with values: `CHAR`, `UNION`, `CONCAT`, `STAR`, `PLUS`, `QUESTION`, `LPAREN`, `RPAREN`
- Define `Token` struct with fields `TokenType type` and `char value`
- No implementation in this file — declarations only

**lexer.h / lexer.cpp**
- Class `Lexer` takes a `const std::string&` in its constructor
- Public method: `std::vector<Token> tokenize()`
- Scan character by character; map `|*+?()` to their token types; all other printable ASCII → `CHAR`
- After initial scan, do a second pass to insert `CONCAT` tokens using the rule: left ∈ {CHAR, RPAREN, STAR, PLUS, QUESTION} AND right ∈ {CHAR, LPAREN}
- Throw `std::runtime_error("Unrecognized character: X")` for any character outside printable ASCII or the supported operators

**ast.h / ast.cpp**
- Define `NodeType` enum class: `CHAR`, `CONCAT`, `UNION`, `STAR`, `PLUS`, `QUESTION`
- Define `ASTNode` struct with: `NodeType type`, `char value`, `std::unique_ptr<ASTNode> left`, `std::unique_ptr<ASTNode> right`
- Implement three static factory methods: `makeChar(char)`, `makeBinary(NodeType, left, right)`, `makeUnary(NodeType, child)` — store the single child of unary nodes in `left`

**parser.h / parser.cpp**
- Class `Parser` takes `const std::vector<Token>&`
- Public method: `std::unique_ptr<ASTNode> parse()` — calls `expr()` then asserts no tokens remain
- Implement private methods `expr()`, `concat()`, `repeat()`, `atom()` matching the grammar in the spec exactly
- `peek()` returns the current token without consuming; returns a sentinel `Token{TokenType::RPAREN, 0}` at end of input
- `consume()` advances and returns the current token
- Throw `std::runtime_error` with position info on any syntax error

### Acceptance Criteria
- `Lexer("(a|b)*abb").tokenize()` produces exactly the token sequence shown in the spec
- `Lexer("").tokenize()` returns an empty vector
- `Lexer("a").tokenize()` returns `[CHAR(a)]`
- Parser produces a correct AST for `a|b`, `ab`, `a*`, `(a|b)*abb`
- Parser throws on `(a`, `a|`, `*a`
- All files compile with zero warnings under `-std=c++17 -Wall -Wextra`

### Must Not
- Define any NFA or DFA types — those belong to A2 and A3
- Use `new` or `delete` directly
- Include any Emscripten headers

---

## Agent A2 — `agent-nfa`

### Role
Implement Thompson's Construction. Walk the AST produced by A1 and build an NFA, recording every construction step for the frontend step player.

### Reads
- `regex-automata-spec.md` → section: NFA + Thompson's Construction
- `src/ast.h` (from A1)

### Produces
```
src/nfa.h
src/nfa.cpp
```

### Detailed Instructions

**nfa.h**
- Define `NFAState` struct: `int id`, `bool accept`, `std::map<char, std::vector<int>> transitions`
  - Use `'\0'` as the key for ε-transitions
- Define `NFAFragment` struct: `int startId`, `int acceptId`
- Define `NFAStep` struct: `std::string description`, `std::vector<int> highlightStates`, `std::vector<std::tuple<int,int,char>> highlightEdges`, `std::vector<int> newStates`, `std::vector<NFAState> snapshot`
- Define class `NFA`: public fields `std::vector<NFAState> states`, `int startState`, `std::vector<NFAStep> steps`; public method `NFAFragment build(const ASTNode* node)`

**nfa.cpp**
- `newState(bool accept)` — appends a new state to `states` with a monotonically incrementing id, returns the id
- `addTransition(int from, char sym, int to)` — appends `to` to `states[from].transitions[sym]`
- `build(node)` — dispatches on `node->type` to `buildChar`, `buildConcat`, `buildUnion`, `buildStar`, `buildPlus`, `buildQuestion`
- Each `buildX` method must:
  1. Create the required states and transitions as per the spec's Thompson rules table
  2. Call `recordStep()` with a human-readable description, the new states, and highlighted edges
  3. Return an `NFAFragment` with the correct start and accept ids
- `recordStep()` must snapshot `states` at that moment (copy the full vector)
- The final accept state of the root fragment must have `accept = true`; all others must have `accept = false`

**Thompson's rules (exact):**

| Node | States created | Transitions added |
|---|---|---|
| `CHAR(c)` | s, e | s→e on c |
| `CONCAT(A,B)` | none | A.accept→B.start on ε |
| `UNION(A,B)` | s, e | s→A.start on ε, s→B.start on ε, A.accept→e on ε, B.accept→e on ε |
| `STAR(A)` | s, e | s→A.start on ε, s→e on ε, A.accept→A.start on ε, A.accept→e on ε |
| `PLUS(A)` | s, e | s→A.start on ε, A.accept→A.start on ε, A.accept→e on ε |
| `QUESTION(A)` | s, e | s→A.start on ε, s→e on ε, A.accept→e on ε |

### Acceptance Criteria
- `NFA` for single char has exactly 2 states
- `NFA` for `a|b` has exactly 6 states
- `NFA` for `a*` has exactly 4 states
- `steps` vector is non-empty after any `build()` call
- Each step's `snapshot` contains a complete copy of all states at that point
- The final state's `accept` field is `true`; no other state has `accept = true`
- Compiles cleanly against A1's headers

### Must Not
- Depend on `dfa.h` or `serializer.h`
- Modify `ASTNode` in any way
- Use `new`/`delete`

---

## Agent A3 — `agent-dfa`

### Role
Implement Subset Construction. Convert the NFA from A2 into a DFA, recording each worklist iteration as a step.

### Reads
- `regex-automata-spec.md` → section: DFA + Subset Construction
- `src/nfa.h` (from A2)

### Produces
```
src/dfa.h
src/dfa.cpp
```

### Detailed Instructions

**dfa.h**
- Define `DFAState` struct: `int id`, `std::vector<int> nfaIds` (sorted), `bool accept`, `std::map<char, int> transitions`
- Define `DFAStep` struct: `std::string description`, `int currentDFAState`, `char symbol`, `std::vector<int> moveResult`, `std::vector<int> closureResult`, `int resultDFAState` (-1 if none/dead), `bool isNewState`, `std::vector<DFAState> snapshot`
- Define class `DFA`: public fields `std::vector<DFAState> states`, `int startState`, `std::vector<char> alphabet`, `std::vector<DFAStep> steps`; public method `void build(const NFA& nfa)`

**dfa.cpp**

Implement `epsilonClosure(stateIds, nfa)`:
- BFS/DFS over ε-transitions (key `'\0'`) from each state in `stateIds`
- Return sorted, deduplicated list of all reachable state ids

Implement `move(stateIds, sym, nfa)`:
- For each state id in `stateIds`, collect all states reachable via `sym` (not ε)
- Return sorted, deduplicated list

Implement `build(nfa)`:
1. Collect alphabet: all transition keys in all NFA states that are not `'\0'`, sort them, store in `alphabet`
2. Start: compute `epsilonClosure({nfa.startState})`, create DFA state D0 with this NFA set
3. Maintain a `std::map<std::vector<int>, int>` from NFA-id-set to DFA-state-id for lookup
4. Worklist: for each unmarked DFA state, for each symbol in alphabet:
   - Compute `move(currentNFAIds, sym, nfa)` → `moveResult`
   - If `moveResult` is empty, skip (no transition on this symbol from this DFA state)
   - Compute `epsilonClosure(moveResult)` → `closureResult`
   - Look up or create DFA state for `closureResult`
   - Add transition from current DFA state to result on `sym`
   - Record a `DFAStep` with all fields populated
5. Mark a DFA state as accepting if any of its NFA ids is the NFA's accept state

### Acceptance Criteria
- DFA for `a` has 2 states (accepting on "a", dead otherwise)
- DFA for `a*` accepts empty string (start state is accepting)
- DFA for `(a|b)*abb` correctly accepts `"abb"`, `"aabb"`, `"babb"` and rejects `"ab"`, `"abba"`, `""`
- `steps` vector is non-empty after `build()`
- Each `DFAStep` snapshot is a complete copy of DFA states at that point
- Compiles cleanly against A1 and A2 headers

### Must Not
- Modify NFA states during DFA construction
- Depend on `serializer.h`
- Use `new`/`delete`

---

## Agent A4 — `agent-serializer`

### Role
Serialize the complete pipeline output — NFA states, DFA states, all NFA steps, all DFA steps — into a single JSON string. No external JSON libraries.

### Reads
- `regex-automata-spec.md` → section: Serializer, JSON output schema
- `src/nfa.h` (from A2)
- `src/dfa.h` (from A3)

### Produces
```
src/serializer.h
src/serializer.cpp
```

### Detailed Instructions

**serializer.h**
- Class `Serializer` with one public method:
  `std::string serialize(const std::string& regex, const NFA& nfa, const DFA& dfa)`

**serializer.cpp**
- Hand-write all JSON output using `std::ostringstream`
- Escape strings properly: replace `"` with `\"`, `\` with `\\`, newlines with `\n`
- Serialize ε (the `'\0'` char key) as the UTF-8 string `"ε"` in JSON
- Output structure must exactly match the schema in the spec:
  - Top-level keys: `regex`, `alphabet`, `nfa`, `dfa`, `nfaSteps`, `dfaSteps`
  - `nfa.states[i].transitions` is an object: symbol string → array of ints
  - `dfa.states[i].transitions` is an object: symbol string → int
  - `nfaSteps[i].highlightEdges` is an array of `[from, to, "symbol"]` triples
  - `dfaSteps[i].snapshot` contains the full DFA state list at that step

Write private helper methods to avoid repetition:
- `jsonInt(int)`, `jsonStr(const std::string&)`, `jsonIntArray(const std::vector<int>&)`
- `serializeNFAState(const NFAState&)`, `serializeDFAState(const DFAState&)`
- `serializeNFAStep(const NFAStep&)`, `serializeDFAStep(const DFAStep&)`

### Acceptance Criteria
- Output is valid JSON (parseable by `JSON.parse()` in a browser)
- `ε` appears correctly in NFA transition keys
- All step snapshots are present and non-empty
- Round-trip: the NFA state count in JSON matches the NFA object's `states.size()`
- No use of `sprintf`, `printf`, or C-style formatting — use `std::ostringstream` only
- Compiles cleanly with zero warnings

### Must Not
- Include `nlohmann/json`, `rapidjson`, or any third-party JSON library
- Modify NFA or DFA objects

---

## Agent A5 — `agent-binding`

### Role
Wire everything together. Write the Emscripten JS binding, the CMakeLists.txt for both native and WASM builds, and the build.sh script.

### Reads
- `regex-automata-spec.md` → sections: Emscripten Binding, Build Instructions, CLI Entry Point
- All headers from A1, A2, A3, A4

### Produces
```
src/binding.cpp
src/main.cpp
CMakeLists.txt
build.sh
```

### Detailed Instructions

**binding.cpp**
- Include Emscripten bind header and all module headers
- Implement `std::string processRegex(const std::string& regex)` exactly as shown in the spec
- Wrap the entire pipeline in `try/catch(const std::exception& e)` — return `{"error":"<message>"}` on any failure
- Register with `EMSCRIPTEN_BINDINGS(automata_module)`

**main.cpp**
- Accept one command-line argument: the regex string
- Run the full pipeline (Lexer → Parser → NFA → DFA → Serializer)
- Print the resulting JSON to stdout
- Print error message to stderr and return exit code 1 on failure
- Useful for debugging; wrap all pipeline calls in try/catch

**CMakeLists.txt**
- `cmake_minimum_required(VERSION 3.20)`
- `project(regex-automata CXX)`
- `set(CMAKE_CXX_STANDARD 17)`
- Define a `SOURCES` variable listing all `.cpp` files in `src/` except `binding.cpp`
- Native target: `add_executable(automata ${SOURCES} src/main.cpp)` with `-Wall -Wextra`
- WASM target: guarded by `if(EMSCRIPTEN)` — `add_executable(automata-wasm ${SOURCES} src/binding.cpp)` with Emscripten flags from the spec

**build.sh**
```bash
#!/bin/bash
set -e
source "$EMSDK/emsdk_env.sh"
mkdir -p build-wasm && cd build-wasm
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make
cp automata-wasm.js ../web/automata.js
cp automata-wasm.wasm ../web/automata.wasm
echo "WASM build complete. Files copied to web/"
```
- Must use `set -e` so any failure aborts the script
- Output `.js` and `.wasm` files go into `web/`

### Acceptance Criteria
- `cmake .. && make` (native) produces a working `./automata` binary
- `./automata "(a|b)*abb"` prints valid JSON to stdout
- `./automata ""` prints `{"error":"..."}` to stdout (empty regex is an error)
- WASM build produces `web/automata.js` and `web/automata.wasm`
- `AutomataModule().then(m => m.processRegex("a"))` works in a browser console

### Must Not
- Hard-code the path to Emscripten — use `$EMSDK` env variable
- Include frontend files in the CMake build

---

## Agent A6 — `agent-frontend`

### Role
Build the complete browser frontend. Loads the WASM module, drives the step player, and renders NFA/DFA graphs on canvas. Can start immediately using the mock JSON schema from the spec.

### Reads
- `regex-automata-spec.md` → section: Frontend
- The JSON output schema from the Serializer section (use this to build a mock for development)

### Produces
```
web/index.html
web/app.js
web/renderer.js
web/style.css
```

### Detailed Instructions

**index.html**
- Load `automata.js` via `<script src="automata.js"></script>`
- Load `app.js` and `renderer.js` as modules: `<script type="module" src="app.js"></script>`
- Layout:
  - Header with project title
  - Input row: text field (`id="regex-input"`, default value `(a|b)*abb`) + "Convert" button
  - Error display: `<div id="error-msg">`
  - Tab bar: buttons for "NFA", "DFA" (each tab shows the graph for the final automaton)
  - Step player: Prev button, step counter (`3 / 14`), Next button, Play/Pause button, speed slider
  - Step description: `<div id="step-desc">` showing `step.description`
  - Canvas: `<canvas id="graph-canvas">` wrapped in a scrollable div
- No frameworks, no external CSS libraries

**style.css**
- CSS variables for colors, consistent with the system `prefers-color-scheme` media query
- Dark mode: dark background, light text, slightly lighter surfaces for panels
- Light mode: white background, dark text
- Monospace font for the regex input and state labels
- Buttons: flat style, subtle border, hover background change
- Tab bar: underline active tab, no box around tabs
- Canvas wrapper: `overflow: auto`, border

**app.js**
- On load: call `AutomataModule()` and store the resolved module in a variable
- `convert()` function: read `regex-input`, call `module.processRegex(regex)`, parse JSON, cache in `window._automata`, call `initPlayer('nfa')`
- `initPlayer(phase)`: reset step counter to 0, call `renderStep(phase, 0)`
- `renderStep(phase, i)`: get the correct steps array, call `Renderer.draw(step.snapshot, step.highlightStates, step.highlightEdges, isStart, isAccept)`, update description and counter
- Step player controls:
  - Prev: decrement step index, clamp at 0, call `renderStep`
  - Next: increment step index, clamp at max, call `renderStep`
  - Play/Pause: use `setInterval` at the speed set by the slider (default 800ms per step), clear interval on Pause
  - Speed slider: range 200ms–2000ms, update interval if playing
- Tab switching: clicking "NFA" or "DFA" calls `initPlayer('nfa')` or `initPlayer('dfa')`
- Error handling: if JSON contains `error` key, show it in `#error-msg` and stop

**renderer.js**
Export a `Renderer` object with a `draw(snapshot, highlightStates, highlightEdges, startId, acceptIds)` method.

Layout algorithm (BFS rank):
1. BFS from `startId` through the snapshot's transitions (use all non-ε edges for DFA, include ε edges for NFA)
2. Assign each state a `col` equal to its BFS discovery depth
3. Within each column, distribute states evenly vertically with at least 90px between centres
4. Compute total canvas width = `(maxCol + 1) * 130 + 80`; height = `maxStatesInAnyCol * 90 + 80`
5. Set canvas dimensions accordingly

Drawing:
- Clear canvas before each draw
- Draw edges first, then states on top
- State circle radius: 22px
  - Normal: gray fill, dark border
  - Start: blue fill
  - Accept: green fill, double ring (inner circle at radius 16px)
  - Highlighted (in `highlightStates`): amber fill
- Start arrow: small arrow pointing into the start state from the left
- Self-loop: quadratic bezier curving above the state circle
- Forward edge: straight line with arrowhead
- Back edge (reverse of an existing edge): curved quadratic bezier offset perpendicular
- Highlighted edge (in `highlightEdges`): purple stroke, 2px width
- Normal edge: gray stroke, 1px width
- Arrow labels: 11px monospace, positioned at midpoint offset 10px perpendicular to edge direction
- Dark mode: detect `window.matchMedia('(prefers-color-scheme: dark)')` and switch fill/stroke colors

### Acceptance Criteria
- Page loads without errors in Chrome and Firefox
- Typing `(a|b)*abb` and clicking Convert shows the NFA step player and DFA step player
- Prev/Next navigate through steps and graph updates correctly
- Play button auto-advances steps at the selected speed and stops at the end
- Dark mode works without any hardcoded colors bleeding through
- Canvas is scrollable if the automaton is wide
- Error message displays for invalid regex input (e.g. `(a|`)

### Must Not
- Use any JS framework (React, Vue, etc.)
- Use any CSS framework (Bootstrap, Tailwind, etc.)
- Call `module.processRegex` more than once per Convert button press
- Use `localStorage` or any persistent storage

---

## Agent A7 — `agent-test`

### Role
Write all unit tests. Can work module-by-module in parallel as each upstream agent completes. Tests use only `<cassert>` and `<iostream>` — no test framework.

### Reads
- `regex-automata-spec.md` → section: Testing Requirements
- Headers from A1, A2, A3 as they become available

### Produces
```
tests/test_lexer.cpp
tests/test_parser.cpp
tests/test_nfa.cpp
tests/test_dfa.cpp
```

### Detailed Instructions

Each test file must follow this pattern:
```cpp
#include <cassert>
#include <iostream>
#include "../src/lexer.h"   // adjust path as needed

void test_name() {
    // setup
    // action
    // assert
    std::cout << "PASS: test_name\n";
}

int main() {
    test_name();
    // ...
    std::cout << "All tests passed.\n";
    return 0;
}
```

**test_lexer.cpp** — must cover:
- Token sequence for `(a|b)*abb` matches the spec exactly (check type and value of every token)
- Concatenation insertion: `ab` produces `[CHAR(a), CONCAT, CHAR(b)]`
- `)*a` inserts CONCAT between RPAREN/STAR and CHAR
- Empty string produces empty vector
- Single character `a` produces `[CHAR(a)]`
- `|` alone produces `[UNION]`
- Unrecognized character (e.g. `@`) throws `std::runtime_error`

**test_parser.cpp** — must cover:
- `a|b` → UNION node, left = CHAR(a), right = CHAR(b)
- `ab` (after lexing) → CONCAT node
- `a*` → STAR node, child = CHAR(a)
- `(a|b)*abb` → root is CONCAT, descend to verify structure at least 2 levels deep
- Unmatched `(a` → throws
- Trailing operator `a|` → throws
- `*a` (operator with no left operand) → throws

**test_nfa.cpp** — must cover:
- State count for single char = 2
- State count for `a|b` = 6
- State count for `a*` = 4
- ε-transitions (`'\0'` key) exist for UNION node result
- ε-transitions exist for STAR node result (back-edge)
- `steps.size() > 0` after any build
- Final state has `accept == true`
- No state other than the final accept has `accept == true`

**test_dfa.cpp** — must cover for each regex, test by simulating the DFA (follow transitions):
- `a`: accepts `"a"`, rejects `"b"`, rejects `""`
- `a*`: accepts `""`, accepts `"a"`, accepts `"aaa"`, rejects `"b"`, rejects `"ba"`
- `a|b`: accepts `"a"`, accepts `"b"`, rejects `"ab"`, rejects `""`
- `(a|b)*abb`: accepts `"abb"`, `"aabb"`, `"babb"`, `"aababb"`; rejects `"ab"`, `"abba"`, `""`
- `steps.size() > 0` after build
- Start state exists in `states`
- All states referenced in transitions exist in `states`

Write a helper `bool simulate(const DFA& dfa, const std::string& input)` at the top of `test_dfa.cpp` and reuse it across all test cases.

### Acceptance Criteria
- All test executables compile and run with zero failures
- Tests print `PASS: <name>` for each passing test and `All tests passed.` at the end
- No test has side effects that affect other tests

### Must Not
- Use any testing framework (gtest, catch2, doctest)
- Mock any module — test against the real implementations

---

## Agent A8 — `agent-integrator`

### Role
Final integration. Wire all pieces together, run end-to-end tests, write the README. This agent runs last.

### Reads
- All source files from A1–A7
- `regex-automata-spec.md` in full

### Produces
```
README.md
(any bug fixes needed for integration)
```

### Detailed Instructions

**Integration checklist:**
1. Run native build: `cmake .. && make` — must succeed with zero warnings
2. Run all unit tests: `./test_lexer && ./test_parser && ./test_nfa && ./test_dfa` — all must pass
3. Run CLI: `./automata "(a|b)*abb"` — pipe output to `python3 -m json.tool` to validate JSON
4. Run WASM build: `bash build.sh` — must produce `web/automata.js` and `web/automata.wasm`
5. Serve: `cd web && python3 -m http.server 8080`
6. Open browser: test `(a|b)*abb`, `a*`, `a|b`, `a+b?` — step player must work for all
7. Test error case: enter `(a|` — error message must display

**README.md must include:**
- Project description (1 paragraph)
- Prerequisites (Emscripten, CMake, C++17 compiler, Python for local server)
- Setup: how to install Emscripten
- Build: native build commands, WASM build command
- Run: how to start the local server and open the app
- Usage: supported regex syntax table (copy from spec)
- Project structure: the directory tree from the spec
- How it works: 3–4 sentences each on Thompson's Construction and Subset Construction

**If any integration issue is found:**
- Fix it directly if it is a minor wiring issue (wrong include path, missing CMake target, etc.)
- Document it clearly with the fix applied if it required a logic change, so the owning agent is aware

### Acceptance Criteria
- Full build succeeds end-to-end from a clean checkout
- All 4 unit test binaries pass
- WASM app works in Chrome and Firefox
- README is complete and accurate
- The final `web/` directory contains exactly: `index.html`, `app.js`, `renderer.js`, `style.css`, `automata.js`, `automata.wasm`

---

## Handoff Protocol

Each agent must signal completion by producing a short handoff note in a comment block at the top of their primary output file:

```cpp
// HANDOFF: agent-lexer-parser → agent-nfa
// Status: complete
// Files produced: token.h, lexer.h, lexer.cpp, ast.h, ast.cpp, parser.h, parser.cpp
// Notes: CONCAT insertion uses two-pass approach. '\0' is not used here — epsilon is NFA's concern.
```

For JS/HTML files, use a `<!-- HANDOFF: ... -->` comment at the top.

---

## Global Rules (apply to all agents)

- Read `regex-automata-spec.md` in full before writing any code
- Never use `new` or `delete` — use `std::unique_ptr`, `std::make_unique`, `std::vector`
- Every public function that can fail must throw `std::runtime_error` with a descriptive message
- No external C++ libraries beyond the standard library and Emscripten headers
- No external JS libraries or CSS frameworks in the frontend
- All C++ compiles with `-std=c++17 -Wall -Wextra` and zero warnings
- Use `'\0'` for epsilon internally in C++; serialize as `"ε"` in JSON
- Do not modify types or interfaces defined by another agent — open a note in your handoff comment if you need a change, and coordinate with the owning agent
- The step player must never re-invoke the WASM module on step navigation — JSON is parsed once and cached