#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$ROOT/backend/build"
mkdir -p "$BUILD_DIR"

cc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Wpedantic \
  -I"$ROOT/backend/include" \
  "$ROOT"/backend/src/*.c \
  -o "$BUILD_DIR/web_server"

cd "$ROOT"
exec "$BUILD_DIR/web_server"
