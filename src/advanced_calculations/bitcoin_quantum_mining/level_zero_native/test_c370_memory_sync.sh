#!/bin/bash
#
# TEST C370 - Memory Synchronization Validation
# Compile et exécute tests de cohérence mémoire GPU→CPU
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  TEST C370 - Memory Synchronization Validation            ║"
echo "║  Compilation et Exécution                                  ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Vérifier dépendances
echo "🔍 Vérification dépendances..."
if ! pkg-config --exists libdrm; then
    echo "❌ ERREUR: libdrm non trouvé"
    echo "   Installer: sudo apt-get install libdrm-dev"
    exit 1
fi
echo "✓ libdrm OK"

# Compilation
echo ""
echo "🔨 Compilation test_c370_memory_sync.c..."

gcc -o test_c370_memory_sync \
    tests/test_c370_memory_sync.c \
    $(pkg-config --cflags --libs libdrm) \
    -I/usr/include/libdrm \
    -I/usr/include/drm \
    -Wall -Wextra \
    -O2 \
    -msse2 \
    -g

if [ $? -eq 0 ]; then
    echo "✅ Compilation réussie"
else
    echo "❌ Compilation échouée"
    exit 1
fi

# Vérifier permissions DRM
echo ""
echo "🔍 Vérification permissions /dev/dri/renderD128..."
if [ ! -r /dev/dri/renderD128 ] || [ ! -w /dev/dri/renderD128 ]; then
    echo "⚠️  Permissions insuffisantes"
    echo "   Ajouter utilisateur au groupe render:"
    echo "   sudo usermod -a -G render $USER"
    echo "   Puis se reconnecter"
fi

# Créer répertoire logs
mkdir -p logs/test_c370

# Exécution
echo ""
echo "🚀 Exécution tests..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="logs/test_c370/execution_${TIMESTAMP}.log"

./test_c370_memory_sync 2>&1 | tee "$LOG_FILE"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ TESTS RÉUSSIS"
    echo "   Log sauvegardé: $LOG_FILE"
    echo ""
    echo "📊 CONCLUSION:"
    echo "   → Synchronisation mémoire fonctionne"
    echo "   → Problème ailleurs (kernel ISA ou target bits)"
    echo ""
    echo "🎯 PROCHAINES ACTIONS:"
    echo "   1. Désassembler kernel ISA (vérifier stores)"
    echo "   2. Tester avec target_bits=1"
    echo "   3. Implémenter corrections dans runner production"
else
    echo "❌ TESTS ÉCHOUÉS"
    echo "   Log sauvegardé: $LOG_FILE"
    echo ""
    echo "📊 CONCLUSION:"
    echo "   → Problème synchronisation mémoire détecté"
    echo "   → Implémenter corrections identifiées"
    echo ""
    echo "🔧 CORRECTIONS REQUISES:"
    
    # Analyser quel test a échoué
    if grep -q "TEST 5 FAILED" "$LOG_FILE"; then
        echo "   • Ajouter GEM_WAIT dans runner production"
    fi
    
    if grep -q "TEST 6 FAILED" "$LOG_FILE"; then
        echo "   • Ajouter cache invalidation (CLFLUSH + MFENCE)"
    fi
    
    if grep -q "TEST 7 FAILED" "$LOG_FILE"; then
        echo "   • Ajouter SET_DOMAIN avant lecture output"
    fi
fi

echo ""
echo "📁 Fichiers générés:"
echo "   • Exécutable: ./test_c370_memory_sync"
echo "   • Log: $LOG_FILE"
echo ""

exit $EXIT_CODE

# Made with Bob
