#!/bin/bash
# LumVorax C255v8n Phase 4 — Activation TOUTES Optimisations
# Date: 2026-05-16
# Objectif: Activer batch dynamique, pause thermique, cache warmup, TESTNET3

set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  LumVorax C255v8n Phase 4 — Activation Complète"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Répertoire de travail
cd "$(dirname "$0")"
WORK_DIR="$(pwd)"

echo "[1/5] Nettoyage build précédent..."
make clean 2>/dev/null || true
rm -f bin/test_c255_multi_dispatch 2>/dev/null || true

echo "[2/5] Compilation avec optimisations activées..."
echo "  - Batch size dynamique: ACTIVÉ"
echo "  - Pause thermique NX48/NX49: ACTIVÉ"
echo "  - Cache warmup: ACTIVÉ"
echo "  - TESTNET3 soumission: ACTIVÉ"
echo ""

# Compiler avec Makefile.c255
make -f Makefile.c255 -j$(nproc) 2>&1 | tee logs/forensic/c255v8n_phase4_compilation.log

if [ ! -f "bin/test_c255_multi_dispatch" ]; then
    echo "❌ ERREUR: Compilation échouée"
    exit 1
fi

echo "✓ Compilation réussie"
echo ""

echo "[3/5] Vérification binaire..."
ls -lh bin/test_c255_multi_dispatch
file bin/test_c255_multi_dispatch
echo ""

echo "[4/5] Test validation 60s avec TOUTES optimisations..."
echo "  Batch size: Dynamique 1M→1000M"
echo "  Durée: 60 secondes"
echo "  Mode: TESTNET3"
echo ""

# Créer répertoires logs
mkdir -p logs/forensic/phase4
mkdir -p logs/forensic/modules
mkdir -p logs/forensic/metrics
mkdir -p logs/forensic/anomalies
mkdir -p logs/forensic/hw_samples
mkdir -p logs/forensic/nano
mkdir -p logs/forensic/sessions
mkdir -p logs/forensic/i915

# Lancer test avec timeout 90s (60s + 30s marge)
timeout 90s ./bin/test_c255_multi_dispatch 10 2>&1 | tee logs/forensic/phase4/c255v8n_phase4_execution.log

echo ""
echo "[5/5] Analyse résultats..."

# Compter fichiers logs générés
LOG_COUNT=$(find logs/forensic -type f -name "*.log" -o -name "*.csv" -o -name "*.txt" -o -name "*.jsonl" 2>/dev/null | wc -l)
echo "  Fichiers logs générés: $LOG_COUNT"

# Extraire hashrate final
HASHRATE=$(grep "C255_BATCH_COMPLETE" logs/forensic/test_c255_multi_dispatch.log 2>/dev/null | tail -1 | grep -oP 'hashrate=\K[0-9.]+' || echo "N/A")
echo "  Hashrate final: $HASHRATE GH/s"

# Extraire batch size final
BATCH_SIZE=$(grep "C255_ADAPTIVE_BATCH_UPDATE" logs/forensic/test_c255_multi_dispatch.log 2>/dev/null | tail -1 | grep -oP 'new_batch_size=\K[0-9]+' || echo "N/A")
echo "  Batch size final: $BATCH_SIZE nonces"

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  Phase 4 Terminée"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Logs disponibles:"
echo "  - Compilation: logs/forensic/c255v8n_phase4_compilation.log"
echo "  - Exécution: logs/forensic/phase4/c255v8n_phase4_execution.log"
echo "  - Forensic: logs/forensic/test_c255_multi_dispatch.log"
echo ""
echo "Prochaine étape: Analyser logs avec Bob"

# Made with Bob
