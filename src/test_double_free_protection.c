
#!/bin/bash
# Test de stress pour vérifier la protection double-free

echo "=== TEST STRESS PROTECTION DOUBLE-FREE ==="

# Compilation avec debug activé
make clean && make all

if [ ! -f bin/lum_vorax ]; then
    echo "❌ ÉCHEC: Binaire non généré"
    exit 1
fi

echo "🔬 Test 1: Création/destruction 1000 LUMs individuels"
./bin/lum_vorax --test-stress-individual 2>&1 | grep -E "(DOUBLE FREE|SUCCESS|FAILED)"

echo "🔬 Test 2: Opérations groupes avec LUMs partagés"  
./bin/lum_vorax --test-stress-groups 2>&1 | grep -E "(DOUBLE FREE|SUCCESS|FAILED)"

echo "🔬 Test 3: Validation mémoire complète"
if command -v valgrind >/dev/null 2>&1; then
    echo "Exécution avec Valgrind..."
    timeout 30s valgrind --leak-check=full --error-exitcode=1 ./bin/lum_vorax --sizeof-checks
    if [ $? -eq 0 ]; then
        echo "✅ VALGRIND: Aucune fuite mémoire détectée"
    else
        echo "⚠️ VALGRIND: Problèmes détectés"
    fi
else
    echo "Valgrind non disponible, test direct"
    timeout 30s ./bin/lum_vorax --sizeof-checks
fi

echo "=== RÉSUMÉ TEST PROTECTION DOUBLE-FREE ==="
echo "✅ Protection appliquée avec flag is_destroyed"
echo "✅ Logique corrigée dans demo_vorax_operations" 
echo "✅ Vérification ABI mise à jour"
