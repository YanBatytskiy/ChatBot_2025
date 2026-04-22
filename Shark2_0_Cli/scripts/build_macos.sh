#!/bin/bash
set -e
echo "🔧 Сборка под macOS..."

cmake -B build_macos -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build_macos --parallel

strip build_macos/client
strip build_macos/server

echo "✅ macOS сборка завершена: build_macos/client и server"
