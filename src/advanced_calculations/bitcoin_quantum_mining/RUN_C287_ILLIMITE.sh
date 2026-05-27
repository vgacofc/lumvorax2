#!/bin/bash

# C287 - MINAGE BITCOIN ILLIMITÉ - OBJECTIF 256 BITS LEADING ZEROS
# Aucune limitation de temps - Exécution jusqu'à découverte ou arrêt manuel (Ctrl+C)

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  C287 - MINAGE BITCOIN TESTNET3 ILLIMITÉ                     ║"
echo "║  OBJECTIF: 256 BITS LEADING ZEROS (HASH SHA-256 PARFAIT)     ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

cd /home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining

# Étape 1: Libérer mémoire
echo "[C287] Étape 1/4 — Libération mémoire système..."
echo "  Fermeture applications non essentielles..."
killall chrome firefox slack discord 2>/dev/null || true
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null 2>&1 || true

echo ""
echo "[C287] État mémoire:"
free -h | grep -E "Mem:|Swap:"
echo ""

# Étape 2: Compilation
echo "[C287] Étape 2/4 — Compilation binaire C287..."
cd level_zero_native
make clean > /dev/null 2>&1 || true
if make -j$(nproc) 2>&1 | tee /tmp/c287_compile.log | grep -q "error:"; then
    echo "❌ Erreur compilation"
    cat /tmp/c287_compile.log
    exit 1
fi
echo "✅ Compilation réussie"
ls -lh ../bin/btc_mining_runner | awk '{print "   Binaire: " $5 " " $9}'
cd ..

# Étape 3: Configuration
echo ""
echo "[C287] Étape 3/4 — Configuration minage illimité..."
echo "  - Objectif      : 256 bits leading zeros (hash parfait)"
echo "  - Durée         : ILLIMITÉE (jusqu'à découverte ou Ctrl+C)"
echo "  - Batch Size    : 10,000,000 nonces"
echo "  - GPU           : Intel UHD 620 Gen9 (100% i915 DRM natif)"
echo "  - Modules actifs: NX48, NX49, ASIC, PT-MC, Gen9 Native"
echo ""

# Étape 4: Lancement
echo "[C287] Étape 4/4 — Lancement minage ILLIMITÉ..."
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  MINAGE EN COURS - OBJECTIF 256 BITS                         ║"
echo "║  Arrêt: Ctrl+C                                                ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Lancer avec Doppler (secrets) ou sans
if command -v doppler &> /dev/null; then
    echo "[C287] Lancement avec Doppler (secrets chargés)..."
    doppler run --config dev_lumvorax -- bash tools/btc_run_ubuntu.sh
else
    echo "[C287] Lancement sans Doppler..."
    bash tools/btc_run_ubuntu.sh
fi

echo ""
echo "[C287] Minage terminé"

# Made with Bob
