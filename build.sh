#!/bin/bash
set -e
source "$EMSDK/emsdk_env.sh"
mkdir -p build-wasm && cd build-wasm
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make
mkdir -p ../web
cp automata.js ../web/automata.js
cp automata.wasm ../web/automata.wasm
echo "WASM build complete. Files copied to web/"
