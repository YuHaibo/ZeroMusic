#!/bin/sh
# Run the core alone, for a terminal or the desktop UI to talk to.
# With --dev, everything stays in .run/ in the repository.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN="$ROOT/bin/zero-music-core"

if [ ! -x "$BIN" ]; then
    "$ROOT/scripts/build-core.sh"
fi

if [ "${1:-}" = "--dev" ]; then
    shift
    RUN="$ROOT/.run"
    mkdir -p "$RUN/config" "$RUN/data" "$RUN/cache" "$RUN/state"
    SOCKET="$RUN/zero-music.sock"
    set -- --config-dir "$RUN/config" --data-dir "$RUN/data" \
        --cache-dir "$RUN/cache" --state-dir "$RUN/state" "$@"
else
    RUNTIME=${XDG_RUNTIME_DIR:-/tmp}
    SOCKET="$RUNTIME/zero-music.sock"
fi

exec "$BIN" --socket "$SOCKET" "$@"
