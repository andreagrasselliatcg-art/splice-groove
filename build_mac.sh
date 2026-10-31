#!/bin/bash
# Build Groove-X locally on a Mac.
# Requirements: Xcode Command Line Tools (xcode-select --install) and CMake (brew install cmake)
set -e
cd "$(dirname "$0")"

command -v cmake >/dev/null || { echo "CMake missing: brew install cmake"; exit 1; }
xcode-select -p >/dev/null 2>&1 || { echo "Xcode tools missing: xcode-select --install"; exit 1; }

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target GrooveX_AU GrooveX_VST3 -j "$(sysctl -n hw.ncpu)"

cp -R "build/GrooveX_artefacts/Release/AU/Groove-X.component" .
cp -R "build/GrooveX_artefacts/Release/VST3/Groove-X.vst3" .
./install_mac.command
