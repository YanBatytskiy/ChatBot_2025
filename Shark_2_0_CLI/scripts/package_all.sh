#!/bin/bash
set -e
echo "📦 Packaging binaries..."

mkdir -p dist/linux
cp build_linux_static/client build_linux_static/server dist/linux/

echo "✅ Packaging complete: dist/linux/"
