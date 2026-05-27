#!/usr/bin/env bash
# LumVorax C255v8n Phase 2 — Script copie dépendances complètes
# Copie TOUS les fichiers nécessaires pour compilation autonome

set -euo pipefail

PARENT_DIR=".."
DEST_SRC="src"
DEST_INCLUDE="include"

echo "[C255v8n] Copie dépendances complètes vers level_zero_native..."

# Headers depuis src/
cp -v "$PARENT_DIR/src/sha256_lumvorax.h" "$DEST_INCLUDE/"

# Headers depuis include/
cp -v "$PARENT_DIR/include/btc_mining_forensic.h" "$DEST_INCLUDE/"
cp -v "$PARENT_DIR/include/btc_wallet.h" "$DEST_INCLUDE/"
cp -v "$PARENT_DIR/include/btc_block_validator.h" "$DEST_INCLUDE/"

# Sources GPU async logger
if [ -f "$PARENT_DIR/src/btc_gpu_async_logger.c" ]; then
    cp -v "$PARENT_DIR/src/btc_gpu_async_logger.c" "$DEST_SRC/"
    cp -v "$PARENT_DIR/src/btc_gpu_async_logger.h" "$DEST_INCLUDE/"
fi

echo "[C255v8n] ✅ Dépendances copiées"
echo "[C255v8n] Fichiers copiés :"
ls -lh "$DEST_INCLUDE"/*.h | tail -10

# Made with Bob
