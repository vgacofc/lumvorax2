#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
CPU_TARGET="${1:-99}"
MEM_TARGET="${2:-99}"

STAMP="$(date +%Y%m%d_%H%M%S)"
BACKUP_DIR="$ROOT_DIR/backups/version_$STAMP"
mkdir -p "$BACKUP_DIR"
cp -a "$ROOT_DIR/include" "$BACKUP_DIR/"
cp -a "$ROOT_DIR/src" "$BACKUP_DIR/"
cp -a "$ROOT_DIR/Makefile" "$BACKUP_DIR/"

make -C "$ROOT_DIR" clean all
"$ROOT_DIR/hubbard_hts_runner" "$ROOT_DIR" "$CPU_TARGET" "$MEM_TARGET"

LATEST_RUN="$(ls -1 "$ROOT_DIR/results" | tail -n 1)"
echo "Run termine: $ROOT_DIR/results/$LATEST_RUN"
