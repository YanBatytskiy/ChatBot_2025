#!/bin/bash
set -e

echo "📦 Упаковка бинарников..."

DIST_DIR="dist/linux"
mkdir -p "$DIST_DIR"

cp -f build_linux_static/client build_linux_static/server "$DIST_DIR/"

echo "✅ Упаковка завершена: $DIST_DIR/client и $DIST_DIR/server"
