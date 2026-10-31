#!/bin/bash
# Build Splice Groove locally on a Mac.
# Requirements: Xcode Command Line Tools (xcode-select --install) and CMake (brew install cmake)
set -e
cd "$(dirname "$0")"

command -v cmake >/dev/null || { echo "CMake missing: brew install cmake"; exit 1; }
xcode-select -p >/dev/null 2>&1 || { echo "Xcode tools missing: xcode-select --install"; exit 1; }

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SpliceGroove_AU SpliceGroove_VST3 -j "$(sysctl -n hw.ncpu)"

cp -R "build/SpliceGroove_artefacts/Release/AU/Splice Groove.component" .
cp -R "build/SpliceGroove_artefacts/Release/VST3/Splice Groove.vst3" .
./install_mac.command
