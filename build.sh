#!/bin/bash
# Компилира и пуска прозореца (Linux / macOS). На Windows: отворете
# CMakeLists.txt в Qt Creator или Visual Studio.
set -e
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
if [ -x build/json_gui ]; then
    ./build/json_gui "$@"
else
    ./build/json_cli "$@"
fi
