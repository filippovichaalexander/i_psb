#!/usr/bin/env bash
# Upload the LittleFS image (web UI) with USB passthrough.
# Usage: tools/uploadfs.sh [/dev/ttyACM0]
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ ! -e "$PORT" ]]; then
  echo "error: $PORT not found. Connect the ESP32 data port and check 'ls /dev/ttyACM*'." >&2
  exit 1
fi

docker build -q -t ipsb-pio:local -f "$ROOT/docker/Dockerfile.pio" "$ROOT"

docker run --rm -it \
  --device="$PORT" \
  --group-add dialout \
  -v "$ROOT":/workspace \
  -w /workspace/firmware \
  -e PLATFORMIO_CORE_DIR=/workspace/.pio-core \
  ipsb-pio:local \
  pio run -t uploadfs --upload-port "$PORT"
