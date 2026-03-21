cmake_minimum_required(VERSION 3.20)
project(regex-automata CXX)
set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/nfa.cpp
    src/dfa.cpp
    src/serializer.cpp
)

if(EMSCRIPTEN)
    add_executable(automata-wasm ${SOURCES} src/binding.cpp)
    set_target_properties(automata-wasm PROPERTIES OUTPUT_NAME "automata")
    target_link_options(automata-wasm PRIVATE
        --bind
        -sMODULARIZE=1
        -sEXPORT_NAME=AutomataModule
        -sALLOW_MEMORY_GROWTH=1
        -sENVIRONMENT=web
    )
else()
    add_executable(automata ${SOURCES} src/main.cpp)
    target_compile_options(automata PRIVATE -Wall -Wextra)
endif()
