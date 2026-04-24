#!/bin/bash
set -e
echo "🔧 Building for macOS..."

cmake -B build_macos -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build_macos --parallel

strip build_macos/client
strip build_macos/server

echo "✅ macOS build complete: build_macos/client and build_macos/server"
