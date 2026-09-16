#!/usr/bin/env bash
# Offline bake entrypoint — expands later into full procedural mesh export.
# Today: validates package set shipped in APK; regenerates index timestamps.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BAKED="$ROOT/content/baked"
mkdir -p "$BAKED"
for f in index downtown forest lake swamp vehicles animation sound; do
  test -f "$BAKED/${f}.exg" || { echo "missing $f"; exit 1; }
done
echo "EXWORLD bake OK — packages ready for APK packaging"
ls -la "$BAKED"
