#!/bin/bash
# C369: Script de compilation et exécution avec validation automatique
# Objectif: Valider la pile GPU Gen9 complète avant minage

set -e  # Arrêter en cas d'erreur

echo "═══════════════════════════════════════════════════════════"
echo "  C369 VALIDATION COMPLÈTE GPU GEN9"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Répertoire de travail
cd "$(dirname "$0")"

# Créer répertoires nécessaires
mkdir -p logs/forensic
mkdir -p kernels
mkdir -p build
mkdir -p bin

echo "[C369] Étape 1/4: Compilation kernel de test..."
echo "  Kernel: kernels/test_c369_gpu_write.cl"
echo "  Cible: Intel Gen9 (Kaby Lake)"

# Compiler kernel de test OpenCL → Gen9 ISA
if [ -f "kernels/test_c369_gpu_write.cl" ]; then
    # Utiliser ocloc directement (compile OpenCL → Gen9 ISA)
    ocloc compile \
        -file kernels/test_c369_gpu_write.cl \
        -device kbl \
        -output kernels/test_c369_gpu_write.bin_kbl.bin \
        2>&1 | grep -E "(Build succeeded|error|warning)" || true
    
    if [ -f "kernels/test_c369_gpu_write.bin_kbl.bin" ]; then
        SIZE=$(stat -f%z kernels/test_c369_gpu_write.bin_kbl.bin 2>/dev/null || stat -c%s kernels/test_c369_gpu_write.bin_kbl.bin)
        echo "  ✅ Kernel compilé: $SIZE bytes"
    else
        echo "  ❌ ERREUR: Compilation ocloc échouée"
        echo "  ⚠️  Utilisation kernel production à la place"
    fi
else
    echo "  ⚠️  Kernel de test non trouvé, utilisation kernel production"
fi

echo ""
echo "[C369] Étape 2/4: Compilation runner avec validation..."
echo "  Makefile: Makefile.gen9_native"
echo "  Cible: test_c369_validation"

make -f Makefile.gen9_native test_c369_validation 2>&1 | tail -30

if [ ! -f "bin/test_c369_forensic_validation" ]; then
    echo "  ❌ ERREUR: Compilation runner échouée"
    exit 1
fi

echo "  ✅ Runner compilé avec succès"
echo ""
echo "[C369] Étape 3/4: Exécution tests de validation..."
echo "  Programme: bin/test_c369_forensic_validation"
echo "  Logs: logs/forensic/"
echo ""

# Exécuter avec timeout de 60 secondes
timeout 60s bin/test_c369_forensic_validation 2>&1 | tee logs/forensic/test_c369_validation_$(date +%Y%m%d_%H%M%S).log

EXIT_CODE=$?

echo ""
echo "[C369] Étape 4/4: Analyse résultats..."

if [ $EXIT_CODE -eq 0 ]; then
    echo "  ✅ SUCCÈS: Tous les tests ont réussi"
    echo "  ✅ La pile GPU Gen9 est opérationnelle"
    echo "  ✅ Le minage peut démarrer"
else
    echo "  ❌ ÉCHEC: Un ou plusieurs tests ont échoué (code: $EXIT_CODE)"
    echo "  ❌ Consultez les logs pour identifier le problème"
    echo "  📁 Logs: logs/forensic/"
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  FIN C369 VALIDATION"
echo "═══════════════════════════════════════════════════════════"

exit $EXIT_CODE

# Made with Bob
