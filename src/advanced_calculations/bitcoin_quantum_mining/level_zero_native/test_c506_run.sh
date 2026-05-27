#!/bin/bash
#
# LUMVORAX - Test C506: Extraction Batch Buffer OpenCL
#
# Objectif: Compiler wrapper interception et exécuter test OpenCL
#           pour capturer le batch buffer exact généré par Intel NEO
#
# Date: 2026-05-24
# Cycle: C506
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================================================"
echo "LUMVORAX C506 - EXTRACTION BATCH BUFFER OPENCL"
echo "================================================================================"
echo ""

# Création répertoire logs
mkdir -p logs/forensic

# Étape 1: Compilation wrapper interception
echo "[1/4] Compilation wrapper interception..."
gcc -shared -fPIC -o libintercept_c506.so \
    tests/test_c506_intercept_batch.c \
    -ldl -lrt \
    -Wall -Wextra

if [ $? -eq 0 ]; then
    echo "✓ Wrapper compilé: libintercept_c506.so"
    ls -lh libintercept_c506.so
else
    echo "✗ ERREUR: Compilation wrapper échouée"
    exit 1
fi

echo ""

# Étape 2: Vérification test OpenCL existe
echo "[2/4] Vérification test OpenCL..."
if [ ! -f "tests/test_c427_store_opencl" ]; then
    echo "Test OpenCL non trouvé, compilation..."
    gcc -o tests/test_c427_store_opencl \
        tests/test_c427_store_opencl.c \
        -lOpenCL -lm \
        -Wall -Wextra
    
    if [ $? -eq 0 ]; then
        echo "✓ Test OpenCL compilé"
    else
        echo "✗ ERREUR: Compilation test OpenCL échouée"
        exit 1
    fi
else
    echo "✓ Test OpenCL trouvé: tests/test_c427_store_opencl"
fi

echo ""

# Étape 3: Exécution test OpenCL avec interception
echo "[3/4] Exécution test OpenCL avec interception batch buffer..."
echo "LD_PRELOAD=./libintercept_c506.so ./tests/test_c427_store_opencl"
echo ""

LD_PRELOAD=./libintercept_c506.so ./tests/test_c427_store_opencl

echo ""

# Étape 4: Analyse logs générés
echo "[4/4] Analyse logs générés..."
BATCH_LOG=$(ls -t logs/forensic/batch_intercept_c506_*.log 2>/dev/null | head -1)

if [ -n "$BATCH_LOG" ]; then
    echo "✓ Log batch buffer capturé: $BATCH_LOG"
    echo ""
    
    # Statistiques
    TOTAL_LINES=$(wc -l < "$BATCH_LOG")
    EXECBUFFER_CALLS=$(grep -c "EXECBUFFER2 CALL" "$BATCH_LOG" || echo "0")
    BATCH_SIZE=$(grep "batch_len:" "$BATCH_LOG" | head -1 | awk '{print $2}')
    
    echo "Statistiques:"
    echo "  - Lignes log:        $TOTAL_LINES"
    echo "  - Appels EXECBUFFER2: $EXECBUFFER_CALLS"
    echo "  - Taille batch:      $BATCH_SIZE"
    echo ""
    
    # Afficher début du log
    echo "--- Début du log (50 premières lignes) ---"
    head -50 "$BATCH_LOG"
    echo ""
    echo "--- Fin du log (50 dernières lignes) ---"
    tail -50 "$BATCH_LOG"
    echo ""
    
    echo "✓ Log complet disponible: $BATCH_LOG"
else
    echo "✗ ATTENTION: Aucun log batch buffer trouvé"
    echo "Vérifier logs/forensic/"
    ls -lh logs/forensic/ | tail -10
fi

echo ""
echo "================================================================================"
echo "EXTRACTION BATCH BUFFER TERMINÉE"
echo "================================================================================"
echo ""
echo "Prochaines étapes:"
echo "  1. Analyser $BATCH_LOG"
echo "  2. Comparer avec batch i915 natif (test_c503_final_fix.c)"
echo "  3. Identifier différences exactes"
echo "  4. Corriger batch i915 natif (C507)"
echo ""

# Made with Bob
