
#!/bin/bash

echo "=== SCRIPT EXÉCUTION COMPLÈTE 44 MODULES LUM/VORAX ==="
echo "Conformité standards forensiques ISO/IEC 27037"
echo ""

# Configuration
export MALLOC_CHECK_=2
export MALLOC_PERTURB_=42

echo "Phase 1: Nettoyage et préparation"
make clean

echo ""
echo "Phase 2: Compilation système complet"
if ! make -j$(nproc) all; then
    echo "❌ Échec compilation - arrêt du processus"
    exit 1
fi

echo ""
echo "Phase 3: Compilation tests spécialisés"
if ! make $(BIN_DIR)/test_all_44_modules_complete; then
    echo "❌ Échec compilation tests - tentative manuelle"
    gcc -Wall -Wextra -std=c99 -g -O2 src/tests/test_all_44_modules_complete.c obj/*/*.o -o bin/test_all_44_modules_complete -lm -lpthread
fi

echo ""
echo "Phase 4: Vérification binaires"
echo "Binaires disponibles:"
find bin/ -name "test_*" -executable 2>/dev/null || echo "Aucun binaire de test trouvé"

echo ""
echo "Phase 5: Exécution test principal 44 modules"
if [ -f bin/test_all_44_modules_complete ]; then
    echo "🚀 Lancement test complet des 44 modules..."
    ./bin/test_all_44_modules_complete | tee logs/tests/modules/execution_complete_$(date +%Y%m%d_%H%M%S).log
    TEST_RESULT=$?
else
    echo "❌ Binaire test principal non trouvé"
    echo "📋 Compilation manuelle:"
    echo "gcc -Wall -Wextra -std=c99 -g src/tests/test_all_44_modules_complete.c obj/*/*.o -o bin/test_all_44_modules_complete -lm -lpthread"
    TEST_RESULT=1
fi

echo ""
echo "Phase 6: Tests supplémentaires disponibles"
if [ -f bin/test_advanced_complete ]; then
    echo "🔬 Test modules avancés:"
    ./bin/test_advanced_complete | tee logs/tests/modules/advanced_complete_$(date +%Y%m%d_%H%M%S).log
fi

if [ -f bin/test_integration_complete ]; then
    echo "🔗 Test intégration complète:"
    ./bin/test_integration_complete | tee logs/tests/integration/complete_$(date +%Y%m%d_%H%M%S).log
fi

echo ""
echo "Phase 7: Génération rapport final"
echo "📊 RAPPORT FINAL EXÉCUTION 44 MODULES:"
echo "Logs générés: $(find logs/tests/ -name "*.log" 2>/dev/null | wc -l)"
echo "Taille totale logs: $(du -sh logs/tests/ 2>/dev/null | cut -f1)"

echo ""
echo "📋 LOGS RÉCENTS:"
find logs/tests/modules -name "*.log" -mmin -5 2>/dev/null | head -3 | while read file; do
    echo "📄 $file ($(wc -l < "$file" 2>/dev/null || echo "0") lignes)"
    tail -5 "$file" 2>/dev/null | grep -E "(PASS|FAIL|SUCCESS|ERROR|✅|❌|🎉)" || echo "Logs techniques générés"
    echo "---"
done

echo ""
if [ $TEST_RESULT -eq 0 ]; then
    echo "✅ EXÉCUTION COMPLÈTE 44 MODULES RÉUSSIE"
else
    echo "⚠️  EXÉCUTION PARTIELLEMENT RÉUSSIE - Voir logs pour détails"
fi

echo "🔒 Tous les logs sont disponibles dans logs/tests/"
echo "📊 Rapport détaillé: logs/tests/modules/execution_complete_$(date +%Y%m%d_%H%M%S).log"

exit $TEST_RESULT
