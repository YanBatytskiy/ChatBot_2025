#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

echo "🔧 Building for macOS..."

PG_ROOT="${PostgreSQL_ROOT:-/opt/homebrew/opt/libpq}"

cmake -B build_macos -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DPostgreSQL_ROOT="$PG_ROOT" \
  -DPostgreSQL_INCLUDE_DIR="$PG_ROOT/include" \
  -DPostgreSQL_LIBRARY="$PG_ROOT/lib/libpq.dylib"
cmake --build build_macos --parallel

strip build_macos/client
strip build_macos/server

echo "✅ macOS build complete: build_macos/client and build_macos/server"
echo "ℹ️  Server reads connect_db.conf from the project-local config/ folder."
