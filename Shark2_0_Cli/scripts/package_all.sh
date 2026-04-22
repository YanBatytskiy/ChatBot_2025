#!/bin/bash
set -e
echo "📦 Упаковка бинарников..."

mkdir -p dist/linux
cp build_linux_static/client build_linux_static/server dist/linux/

echo "✅ Упаковка завершена: dist/linux/"
