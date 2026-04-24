#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT_CONFIG_DIR="$PROJECT_DIR/config"
cd "$PROJECT_DIR"

echo "🔧 Static build of client and server via g++..."

BUILD_DIR="build_linux_static"
SRC_DIR="../src"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

INCLUDE_FLAGS="-I$SRC_DIR \
-I$SRC_DIR/core \
-I$SRC_DIR/core/exception \
-I$SRC_DIR/core/message \
-I$SRC_DIR/core/chat \
-I$SRC_DIR/core/chat_system \
-I$SRC_DIR/core/system \
-I$SRC_DIR/core/user \
-I$SRC_DIR/dto \
-I$SRC_DIR/server \
-I$SRC_DIR/client \
-I$SRC_DIR/client/menu \
-I/usr/include/postgresql"

PQ_FLAGS="$(pkg-config --libs libpq 2>/dev/null || echo '-lpq')"
SQLITE_FLAGS="$(pkg-config --cflags --libs --static sqlite3 2>/dev/null || echo '-lsqlite3')"
THREAD_FLAGS="-pthread"
EXTRA_LIBS="-ldl -lrt"

CXXFLAGS="-std=c++20 -O2 -Wall -static-libstdc++ -static-libgcc"

echo "🛠 Building client..."
g++ $CXXFLAGS \
  $INCLUDE_FLAGS \
  $(find "$SRC_DIR/core" -name "*.cpp") \
  $(find "$SRC_DIR/dto" -name "*.cpp") \
  $(find "$SRC_DIR/client" -maxdepth 1 -name "*.cpp") \
  $(find "$SRC_DIR/client/menu" -name "*.cpp") \
  -o client \
  -Wl,-Bdynamic $PQ_FLAGS $THREAD_FLAGS $EXTRA_LIBS \
  -Wl,-Bstatic  $SQLITE_FLAGS \
  -Wl,-Bdynamic

echo "🛠 Building server..."
g++ $CXXFLAGS \
  $INCLUDE_FLAGS \
  $(find "$SRC_DIR/core" -name "*.cpp") \
  $(find "$SRC_DIR/dto" -name "*.cpp") \
  $(find "$SRC_DIR/server" -name "*.cpp") \
  -DCONFIG_DIR=\"$PROJECT_CONFIG_DIR\" \
  -o server \
  -Wl,-Bdynamic $PQ_FLAGS $THREAD_FLAGS $EXTRA_LIBS

strip client || true
strip server || true

echo "✅ Done: $BUILD_DIR/client and $BUILD_DIR/server"
echo "ℹ️  Server reads connect_db.conf from the project-local config/ folder."
