#!/bin/bash

# LumVorax — Bitcoin Mining Gen9 Native i915 DRM
# test_c370_production.sh — Test corrections cache invalidation production
#
# CYCLE C370 — FIX CRITIQUE: Cache invalidation x86_64 + target_bits=1
# OBJECTIF: Valider near-miss GPU ≥1 bit après corrections mémoire
#
# CORRECTIONS IMPLÉMENTÉES:
# 1. ✅ GEM_WAIT après EXECBUFFER2 (déjà présent ligne 1835)
# 2. ✅ SET_DOMAIN avant lecture output (déjà présent ligne 1848)
# 3. ✅ Cache invalidation x86_64 CLFLUSH+MFENCE (NOUVEAU ligne 1860+)
# 4. ✅ Batch size = 10M nonces (déjà configuré adapter.c ligne 49)
# 5. ✅ Target bits = 1 (modifié asic_btc_optimizer.c ligne 170)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "════════════════════════════════════════════════════════════════"
echo "  C370 TEST PRODUCTION — Cache Invalidation x86_64"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "CORRECTIONS APPLIQUÉES:"
echo "  1. GEM_WAIT synchronisation GPU (ligne 1835)"
echo "  2. SET_DOMAIN CPU domain switch (ligne 1848)"
echo "  3. CLFLUSH + MFENCE cache invalidation (ligne 1860+) ← NOUVEAU"
echo "  4. Batch size = 10M nonces (optimal C283)"
echo "  5. Target bits = 1 (test maximal probabilité)"
echo ""

# Timestamp pour logs
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs"
mkdir -p "$LOG_DIR"

LOG_FILE="$LOG_DIR/execution_c370_production_${TIMESTAMP}.log"
FORENSIC_LOG="$LOG_DIR/forensic/c370_production_${TIMESTAMP}.log"

mkdir -p "$LOG_DIR/forensic"

echo "═══ ÉTAPE 1/5: Nettoyage ═══"
make -f Makefile.gen9_native clean 2>&1 | tee -a "$LOG_FILE"

echo ""
echo "═══ ÉTAPE 2/5: Recompilation avec corrections C370 ═══"
echo "  - Makefile: Makefile.gen9_native"
echo "  - Cache invalidation x86_64 activée"
echo "  - Target bits = 1 (test)"
echo "  - Batch size = 10M (optimal)"
echo ""

if ! make -f Makefile.gen9_native test_btc 2>&1 | tee -a "$LOG_FILE"; then
    echo "❌ ERREUR: Compilation échouée"
    echo "Voir logs: $LOG_FILE"
    exit 1
fi

echo ""
echo "✅ Compilation réussie"
echo ""

echo "═══ ÉTAPE 3/5: Vérification binaire ═══"
if [ ! -f "bin/test_btc_mining_native" ]; then
    echo "❌ ERREUR: Binaire bin/test_btc_mining_native non trouvé"
    exit 1
fi

echo "✅ Binaire présent: bin/test_btc_mining_native"
ls -lh bin/test_btc_mining_native

echo ""
echo "═══ ÉTAPE 4/5: Exécution test 30 secondes ═══"
echo "  OBJECTIF: Valider near-miss GPU ≥1 bit"
echo "  DURÉE: 30 secondes"
echo "  BATCH SIZE: 10M nonces"
echo "  TARGET BITS: 1 (probabilité maximale)"
echo ""

# Exécuter avec timeout 30s
timeout 30s ./bin/test_btc_mining_native 2>&1 | tee -a "$LOG_FILE" || true

echo ""
echo "═══ ÉTAPE 5/5: Analyse résultats ═══"
echo ""

# Extraire métriques clés
NEAR_MISS_GPU=$(grep -oP 'near_miss_gpu=\K\d+' "$LOG_FILE" | tail -1 || echo "0")
TOTAL_HASHES=$(grep -oP 'total_hashes=\K\d+' "$LOG_FILE" | tail -1 || echo "0")
GPU_EXECUTED=$(grep -oP 'gpu_executed=\K\w+' "$LOG_FILE" | tail -1 || echo "NO")
CACHE_FLUSH=$(grep -c "C370_CACHE_FLUSH" "$LOG_FILE" || echo "0")

echo "RÉSULTATS C370:"
echo "  • Near-miss GPU: $NEAR_MISS_GPU bits"
echo "  • Total hashes: $TOTAL_HASHES"
echo "  • GPU executed: $GPU_EXECUTED"
echo "  • Cache flush count: $CACHE_FLUSH"
echo ""

# Vérifier succès
if [ "$NEAR_MISS_GPU" -ge 1 ]; then
    echo "════════════════════════════════════════════════════════════════"
    echo "  ✅ SUCCÈS C370: Near-miss GPU ≥1 bit détecté!"
    echo "════════════════════════════════════════════════════════════════"
    echo ""
    echo "VALIDATION:"
    echo "  ✅ Cache invalidation x86_64 fonctionne"
    echo "  ✅ GPU écrit correctement dans output buffer"
    echo "  ✅ CPU lit les valeurs GPU synchronisées"
    echo ""
    echo "PROCHAINE ÉTAPE:"
    echo "  1. Restaurer target_bits = 20 (production)"
    echo "  2. Lancer minage illimité"
    echo "  3. Monitorer hashrate et near-miss"
    echo ""
    exit 0
else
    echo "════════════════════════════════════════════════════════════════"
    echo "  ⚠️  ÉCHEC C370: Near-miss GPU = 0 bits"
    echo "════════════════════════════════════════════════════════════════"
    echo ""
    echo "DIAGNOSTIC:"
    if [ "$GPU_EXECUTED" = "NO" ]; then
        echo "  ❌ GPU n'a pas exécuté (sentinel inchangé)"
        echo "  → Problème: Kernel GPU ou dispatch"
    elif [ "$CACHE_FLUSH" -eq 0 ]; then
        echo "  ❌ Cache flush non exécuté"
        echo "  → Problème: Code C370 non compilé"
    else
        echo "  ⚠️  GPU exécuté ET cache flush OK"
        echo "  → Problème: Kernel ne trouve pas de near-miss"
        echo "  → Vérifier: ISA kernel, target_bits, probabilité"
    fi
    echo ""
    echo "LOGS DÉTAILLÉS:"
    echo "  Main: $LOG_FILE"
    echo "  Forensic: $FORENSIC_LOG"
    echo ""
    exit 1
fi

# Made with Bob
