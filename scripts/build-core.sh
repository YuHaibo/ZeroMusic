#!/bin/sh
# Build zero-music-core. Host by default; set GOOS/GOARCH for the device.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT=${OUT:-"$ROOT/bin/zero-music-core"}
CACHE=${ZERO_MUSIC_GO_CACHE:-"$ROOT/.cache"}

mkdir -p "$CACHE/go-build" "$CACHE/go-mod" "$CACHE/go-tmp" "$(dirname "$OUT")"
export GOCACHE="$CACHE/go-build"
export GOMODCACHE="$CACHE/go-mod"
export GOTMPDIR="$CACHE/go-tmp"
export CGO_ENABLED=0

cd "$ROOT/core"
exec go build -trimpath -ldflags "-s -w" -o "$OUT" ./cmd/zero-music-core
