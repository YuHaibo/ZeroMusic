#!/bin/sh
# Build the aarch64 Debian package in an arm64 Ubuntu container, for hosts
# without a Linux cross toolchain (macOS included).
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
IMAGE=${ZERO_MUSIC_CROSS_IMAGE:-zero-music-cross}

docker build -f "$ROOT/packaging/cross/Dockerfile" -t "$IMAGE" "$ROOT/packaging/cross"
exec docker run --rm --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -v "$ROOT:$ROOT" -w "$ROOT" \
    "$IMAGE" cmake --workflow --preset cp0-cross-package
