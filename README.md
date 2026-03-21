# Regex Automata Converter

A web application that converts a regular expression into an NFA (via Thompson's Construction) and then into a DFA (via Subset Construction), featuring an interactive step-by-step visualization in the browser. The core runs in C++ compiled to WebAssembly.

## Prerequisites
- Emscripten SDK (latest)
- CMake 3.20+
- A C++17-capable compiler (for native tests)
- Python 3 (for local server)

## Setup
Install Emscripten by following the official instructions:
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..
```

## Build
### Native build (for CLI & testing)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
./automata "(a|b)*abb"
```

### WASM build (for Frontend)
Ensure Emscripten is sourced, then:
```bash
bash build.sh
```

## Run
Start the local server and open the app:
```bash
cd web && python3 -m http.server 8080
```
Then navigate to `http://localhost:8080` in your web browser.

## Supported Regex Syntax

| Syntax | Meaning |
|---|---|
| `a` | Literal character |
| `ab` | Concatenation (a then b) |
| `a|b` | Union (a or b) |
| `a*` | Kleene star (zero or more) |
| `a+` | One or more |
| `a?` | Zero or one |
| `(ab)` | Grouping |

## Project Structure
```text
regex-automata/
├── src/         # C++ core
├── web/         # HTML/JS/CSS frontend
├── tests/       # C++ unit tests
├── CMakeLists.txt
├── build.sh
└── README.md
```

## How It Works
**Thompson's Construction:**
The construction converts an abstract syntax tree of a regular expression into a Non-deterministic Finite Automaton (NFA). It recursively builds small NFA fragments for base characters and operators (like Union, Concat, Star). Fragments are then stitched together using epsilon (ε) transitions to form the final NFA without simulating string processing.

**Subset Construction:**
This algorithm converts the resulting NFA into a Deterministic Finite Automaton (DFA) which is more optimal for execution. It creates DFA states that each correspond to a set of NFA states, grouped by their epsilon closures. The transition function then evaluates where the entire set would move upon reading an input token, computing a deterministic leap.
