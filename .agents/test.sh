#!/usr/bin/env bash
# Build MC.C and check that a headless run draws the title screen and the world.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${MCC_TEST_OUT:-/tmp/mcc-test}"
mkdir -p "$OUT_DIR"

bash "$ROOT/.agents/build.sh"

DISPLAY_NUM=99
while [ -e "/tmp/.X11-unix/X${DISPLAY_NUM}" ]; do
  DISPLAY_NUM=$((DISPLAY_NUM + 1))
done

Xvfb ":${DISPLAY_NUM}" -screen 0 1280x720x24 -ac +extension GLX +render -noreset \
  >"$OUT_DIR/xvfb.log" 2>&1 &
XVFB_PID=$!

GAME_PID=""
cleanup() {
  if [ -n "$GAME_PID" ]; then
    kill "$GAME_PID" 2>/dev/null || true
    wait "$GAME_PID" 2>/dev/null || true
  fi
  kill "$XVFB_PID" 2>/dev/null || true
  wait "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT

sleep 0.4
if ! kill -0 "$XVFB_PID" 2>/dev/null; then
  echo "Xvfb failed to start" >&2
  cat "$OUT_DIR/xvfb.log" >&2
  exit 1
fi

export DISPLAY=":${DISPLAY_NUM}"
export LIBGL_ALWAYS_SOFTWARE=1

cd "$ROOT/src"
# Line-buffer the log so "Block textures loaded" shows up before the stdio buffer fills.
stdbuf -oL -eL "$ROOT/build/mcc/mcc" >"$OUT_DIR/game.log" 2>&1 &
GAME_PID=$!

WID=""
for _ in $(seq 1 40); do
  WID="$(xdotool search --name "MC.C" 2>/dev/null | head -1 || true)"
  if [ -n "$WID" ]; then
    break
  fi
  if ! kill -0 "$GAME_PID" 2>/dev/null; then
    echo "game exited before opening a window" >&2
    cat "$OUT_DIR/game.log" >&2
    exit 1
  fi
  sleep 0.25
done

if [ -z "$WID" ]; then
  echo "MC.C window did not appear" >&2
  cat "$OUT_DIR/game.log" >&2
  exit 1
fi

# Logo animation plus the fade into the title screen is a little over 8 seconds.
sleep 12
import -window root "$OUT_DIR/title.png"
TITLE_MEAN="$(convert "$OUT_DIR/title.png" -format '%[fx:mean]' info:)"
awk -v m="$TITLE_MEAN" 'BEGIN { if (m+0 < 0.05) exit 1 }' \
  || { echo "title screenshot looks blank (mean=$TITLE_MEAN)" >&2; exit 1; }
echo "title mean=$TITLE_MEAN"

# XTEST key events (no --window) are what GLFW treats as real input.
# Repeat only while the title screen is still up; an extra Enter in-game leaves the world.
loaded=0
for _ in $(seq 1 6); do
  xdotool windowfocus --sync "$WID"
  xdotool key --clearmodifiers Return
  sleep 1.5
  if grep -q "Block textures loaded" "$OUT_DIR/game.log"; then
    loaded=1
    break
  fi
  import -window root "$OUT_DIR/title-probe.png"
  probe="$(convert "$OUT_DIR/title-probe.png" -format '%[fx:mean]' info:)"
  # Settled title screen mean is about 0.52. Leaving it means the key landed.
  if ! awk -v m="$probe" 'BEGIN { exit !(m+0 > 0.45 && m+0 < 0.58) }'; then
    break
  fi
done

if [ "$loaded" -ne 1 ]; then
  for _ in $(seq 1 90); do
    if grep -q "Block textures loaded" "$OUT_DIR/game.log"; then
      loaded=1
      break
    fi
    if ! kill -0 "$GAME_PID" 2>/dev/null; then
      echo "game exited while loading the world" >&2
      cat "$OUT_DIR/game.log" >&2
      exit 1
    fi
    sleep 1
  done
fi

if [ "$loaded" -ne 1 ]; then
  echo "world did not finish loading textures" >&2
  cat "$OUT_DIR/game.log" >&2
  exit 1
fi

# The first gameplay frame uploads every visible chunk mesh before it swaps.
# The window stays on the black transition until that burst stops.
prev=-1
stable=0
for _ in $(seq 1 60); do
  count="$(grep -c "Mesh uploaded successfully" "$OUT_DIR/game.log" || true)"
  if [ "$count" -gt 0 ] && [ "$count" = "$prev" ]; then
    stable=$((stable + 1))
    if [ "$stable" -ge 2 ]; then
      break
    fi
  else
    stable=0
  fi
  prev="$count"
  if ! kill -0 "$GAME_PID" 2>/dev/null; then
    echo "game exited while meshing the world" >&2
    cat "$OUT_DIR/game.log" >&2
    exit 1
  fi
  sleep 1
done

if [ "$stable" -lt 2 ]; then
  echo "world mesh upload did not finish" >&2
  cat "$OUT_DIR/game.log" >&2
  exit 1
fi

import -window root "$OUT_DIR/gameplay.png"
GAME_MEAN="$(convert "$OUT_DIR/gameplay.png" -format '%[fx:mean]' info:)"
awk -v m="$GAME_MEAN" 'BEGIN { if (m+0 < 0.05) exit 1 }' \
  || { echo "gameplay screenshot looks blank (mean=$GAME_MEAN)" >&2; exit 1; }
echo "gameplay mean=$GAME_MEAN"

if ! kill -0 "$GAME_PID" 2>/dev/null; then
  echo "game exited after drawing the world" >&2
  cat "$OUT_DIR/game.log" >&2
  exit 1
fi

echo "smoke test passed"
echo "artifacts: $OUT_DIR"
