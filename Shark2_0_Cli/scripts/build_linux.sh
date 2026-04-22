#!/bin/bash
set -e
echo "🔧 Статическая сборка client и server вручную через g++..."

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
-I$SRC_DIR/client/menu"

echo "🛠 Сборка client..."
g++ -std=c++20 -static -O2 -Wall \
  $INCLUDE_FLAGS \
  $(find "$SRC_DIR/core" -name "*.cpp") \
  $(find "$SRC_DIR/dto" -name "*.cpp") \
  "$SRC_DIR/server/server_session.cpp" \
  "$SRC_DIR/server/0_init_system.cpp" \
  "$SRC_DIR/client/client.cpp" \
  "$SRC_DIR/client/client_session.cpp" \
  $(find "$SRC_DIR/client/menu" -name "*.cpp") \
  -o client

echo "🛠 Сборка server..."
g++ -std=c++20 -static -O2 -Wall \
  $INCLUDE_FLAGS \
  $(find "$SRC_DIR/core" -name "*.cpp") \
  $(find "$SRC_DIR/dto" -name "*.cpp") \
  "$SRC_DIR/server/server.cpp" \
  "$SRC_DIR/server/server_session.cpp" \
  "$SRC_DIR/server/0_init_system.cpp" \
  -o server

strip client
strip server

echo "✅ Готово: $BUILD_DIR/client и server — статичные бинарники"
