#!/usr/bin/env bash
# Copy the static SPA into the LittleFS data directory.
# Keep this list explicit: no node_modules, no dev server, no dotfiles.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/web"
DST="$ROOT/firmware/data/www"

rm -rf "$DST"
mkdir -p "$DST"

for f in index.html app.js styles.css manifest.webmanifest icon.svg; do
  if [[ -f "$SRC/$f" ]]; then
    cp "$SRC/$f" "$DST/$f"
  else
    echo "warning: web/$f is missing" >&2
  fi
done

echo "packaged $(find "$DST" -type f | wc -l) files into firmware/data/www"
du -sh "$DST"
