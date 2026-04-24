#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

echo "🔧 Building for macOS..."

QT_PREFIX="$(qtpaths6 --query QT_INSTALL_PREFIX 2>/dev/null || true)"
if [ -z "$QT_PREFIX" ] && [ -d "$HOME/Qt/6.10.0/macos" ]; then
  QT_PREFIX="$HOME/Qt/6.10.0/macos"
fi

PG_ROOT="${PostgreSQL_ROOT:-/opt/homebrew/opt/libpq}"
cmd=(cmake -B build_macos -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPostgreSQL_ROOT="$PG_ROOT" -DPostgreSQL_INCLUDE_DIR="$PG_ROOT/include" -DPostgreSQL_LIBRARY="$PG_ROOT/lib/libpq.dylib")
if [ -n "$QT_PREFIX" ]; then
  cmd+=(-DCMAKE_PREFIX_PATH="$QT_PREFIX")
fi
"${cmd[@]}"
cmake --build build_macos --parallel

strip build_macos/Shark_UI/Shark_ui
strip build_macos/server

echo "✅ macOS build complete: build_macos/Shark_UI/Shark_ui and build_macos/server"
echo "ℹ️  Server reads connect_db.conf from the project-local config/ folder."
