#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_DIR"

echo "📦 Packaging binaries..."

DIST_DIR="dist/linux"
CONFIG_DIR="config"
PACKAGE_CONFIG_DIR="$DIST_DIR/config"
mkdir -p "$DIST_DIR" "$PACKAGE_CONFIG_DIR"

cp -f build_linux_static/client build_linux_static/server "$DIST_DIR/"
if [ -f "$CONFIG_DIR/connect_db.conf" ]; then
  cp -f "$CONFIG_DIR/connect_db.conf" "$PACKAGE_CONFIG_DIR/"
fi
cp -f "$CONFIG_DIR/connect_db.local-docker.example" "$PACKAGE_CONFIG_DIR/"
cp -f "$CONFIG_DIR/schema_db.conf" "$PACKAGE_CONFIG_DIR/"

cat > "$DIST_DIR/run_server.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export SHARK_CONFIG_DIR="$DIR/config"
exec "$DIR/server" "$@"
EOF
chmod +x "$DIST_DIR/run_server.sh"

cat > "$DIST_DIR/run_client.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$DIR/client" "$@"
EOF
chmod +x "$DIST_DIR/run_client.sh"

echo "✅ Packaging complete: $DIST_DIR/client and $DIST_DIR/server"
echo "ℹ️  Runtime config now lives in $PACKAGE_CONFIG_DIR/"
