#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: build_linux.sh [options]

Options:
  --build-type <type>   CMAKE_BUILD_TYPE value (default: Release)
  --clean               Remove build directory before configuring
  --generator <name>    Explicit CMake generator (e.g. Ninja)
  --no-package          Skip packaging step
  -h, --help            Show this help and exit

Environment variables:
  BUILD_DIR             Override build directory (default: <repo>/build)
  DIST_DIR              Override dist directory (default: <repo>/dist/linux)
USAGE
}

log() {
  printf '%s\n' "$1"
}

err() {
  printf '❌ %s\n' "$1" >&2
}

die() {
  err "$1"
  exit 1
}

require_file() {
  local path="$1"
  local description="$2"
  [[ -e "$path" ]] || die "$description not found: $path"
}

should_skip_lib() {
  local base
  base=$(basename "$1")
  case "$base" in
    linux-vdso.so.*|ld-linux*.so.*)
      return 0 ;;
  esac
  return 1
}

copy_dependencies() {
  local file="$1"
  local destination="$2"
  local array_name="$3"
  local -n seen_ref="$array_name"

  while IFS= read -r dep; do
    [[ -z "$dep" ]] && continue
    if should_skip_lib "$dep"; then
      continue
    fi
    local base
    base=$(basename "$dep")
    if [[ -z "${seen_ref[$dep]:-}" ]]; then
      if [[ ! -f "$destination/$base" ]]; then
        cp -L "$dep" "$destination/$base"
      fi
      seen_ref["$dep"]=1
      copy_dependencies "$dep" "$destination" "$array_name"
    fi
  done < <(
    ldd "$file" | awk '
      $2 == "=>" {
        if ($3 == "not") {
          next
        } else if ($3 ~ /^\//) {
          print $3
        }
        next
      }
      $1 ~ /^\// {
        print $1
      }
    '
  )
}

check_missing_deps() {
  local file="$1"
  while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    if [[ "$line" == missing:* ]]; then
      local lib=${line#missing:}
      die "Library $lib not found for $file"
    fi
  done < <(
    ldd "$file" | awk '$2 == "=>" && $3 == "not" { printf "missing:%s\n", $1 }'
  )
}

prepare_wrapper() {
  local path="$1"
  local body="$2"
  cat > "$path" <<EOF_WRAPPER
#!/usr/bin/env bash
set -euo pipefail
DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
$body
EOF_WRAPPER
  chmod +x "$path"
}

copy_qt_plugins() {
  local destination="$1"
  local -a subdirs=(
    platforms
    iconengines
    imageformats
    styles
    xcbglintegrations
    platforminputcontexts
    platformthemes
    tls
    wayland-shell-integration
    wayland-decoration-client
  )
  for sub in "${subdirs[@]}"; do
    if [[ -d "$QT_PLUGIN_DIR/$sub" ]]; then
      mkdir -p "$destination/$sub"
      cp -a "$QT_PLUGIN_DIR/$sub"/. "$destination/$sub"/
    fi
  done
}

package_artifacts() {
  command -v ldd >/dev/null 2>&1 || die "ldd command not available"

  QT_PLUGIN_DIR=""
  if command -v qtpaths6 >/dev/null 2>&1; then
    QT_PLUGIN_DIR=$(qtpaths6 --query QT_INSTALL_PLUGINS)
  elif command -v qmake6 >/dev/null 2>&1; then
    QT_PLUGIN_DIR=$(qmake6 -query QT_INSTALL_PLUGINS)
  else
    die "Cannot determine Qt plugin path: neither qtpaths6 nor qmake6 is available"
  fi
  [[ -n "$QT_PLUGIN_DIR" && -d "$QT_PLUGIN_DIR" ]] || die "Qt plugin directory not found: $QT_PLUGIN_DIR"

  local server_bin="$BUILD_DIR/server"
  local client_bin="$BUILD_DIR/Shark_UI/Shark_ui"
  local core_lib="$BUILD_DIR/libcore.so"
  local config_dir="$ROOT_DIR/config"
  local config_file="$config_dir/connect_db.conf"

  require_file "$server_bin" "Server binary"
  require_file "$client_bin" "Qt client"
  require_file "$core_lib" "libcore.so"
  require_file "$config_file" "config/connect_db.conf configuration file"

  log "📦 Preparing directory $DIST_DIR"
  rm -rf "$DIST_DIR"
  mkdir -p "$DIST_DIR"

  local server_dir="$DIST_DIR/server"
  local server_lib_dir="$server_dir/lib"
  local package_config_dir="$DIST_DIR/config"
  mkdir -p "$server_lib_dir" "$package_config_dir"
  cp "$server_bin" "$server_dir/"
  cp "$config_file" "$package_config_dir/"
  cp "$config_dir/connect_db.local-docker.example" "$package_config_dir/"

  declare -A server_seen=()
  check_missing_deps "$server_bin"
  copy_dependencies "$server_bin" "$server_lib_dir" server_seen
  copy_dependencies "$core_lib" "$server_lib_dir" server_seen

  prepare_wrapper "$server_dir/run_server.sh" 'export LD_LIBRARY_PATH="$DIR/lib:${LD_LIBRARY_PATH:-}"
export SHARK_CONFIG_DIR="$DIR/../config"
exec "$DIR/server" "$@"'

  local client_dir="$DIST_DIR/client"
  local client_lib_dir="$client_dir/lib"
  local plugin_dir="$client_dir/plugins"
  mkdir -p "$client_lib_dir" "$plugin_dir"
  cp "$client_bin" "$client_dir/"

  declare -A client_seen=()
  check_missing_deps "$client_bin"
  copy_dependencies "$client_bin" "$client_lib_dir" client_seen
  copy_dependencies "$core_lib" "$client_lib_dir" client_seen

  copy_qt_plugins "$plugin_dir"

  find "$plugin_dir" -type f -name '*.so' -print0 | while IFS= read -r -d '' plugin; do
    copy_dependencies "$plugin" "$client_lib_dir" client_seen
  done

  cat > "$client_dir/qt.conf" <<'EOF_QTCONF'
[Paths]
Plugins = plugins
EOF_QTCONF

  prepare_wrapper "$client_dir/run_client.sh" 'export LD_LIBRARY_PATH="$DIR/lib:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="$DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$DIR/plugins/platforms"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
exec "$DIR/Shark_ui" "$@"'

  log "✅ Packaging complete. Client and server are in $DIST_DIR"
}

CLEAN=false
PACKAGE=true
BUILD_TYPE="${BUILD_TYPE:-Release}"
GENERATOR="${GENERATOR:-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-type)
      [[ $# -ge 2 ]] || { err "Missing value for --build-type"; exit 1; }
      BUILD_TYPE="$2"
      shift 2
      ;;
    --clean)
      CLEAN=true
      shift
      ;;
    --generator)
      [[ $# -ge 2 ]] || { err "Missing value for --generator"; exit 1; }
      GENERATOR="$2"
      shift 2
      ;;
    --no-package)
      PACKAGE=false
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      err "Unknown option: $1"
      usage >&2
      exit 1
      ;;
  esac
done

command -v cmake >/dev/null 2>&1 || die "cmake not found"

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
DIST_DIR="${DIST_DIR:-$ROOT_DIR/dist/linux}"

if [[ "$CLEAN" == true ]]; then
  log "🧹 Removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

if [[ -z "$GENERATOR" ]]; then
  if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
  fi
fi

CMAKE_ARGS=("-S" "$ROOT_DIR" "-B" "$BUILD_DIR" "-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
if [[ -n "$GENERATOR" ]]; then
  CMAKE_ARGS+=("-G" "$GENERATOR")
fi

log "🔧 Configuring project ($BUILD_TYPE)"
cmake "${CMAKE_ARGS[@]}"

log "🏗  Building server and Shark_ui"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --target server Shark_ui

log "✅ Artefacts ready"
log "  server   -> $BUILD_DIR/server"
log "  Shark_ui -> $BUILD_DIR/Shark_UI/Shark_ui"

if [[ "$PACKAGE" == true ]]; then
  package_artifacts
else
  log "⏭  Packaging skipped (--no-package)"
fi
