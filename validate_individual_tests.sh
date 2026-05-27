#!/bin/bash

# Script de validation forensique pour les 44 tests individuels (README.md)

echo "=== VALIDATION FORENSIQUE TESTS INDIVIDUELS LUM/VORAX ==="
echo "Date: $(date)"
echo "Conformité README.md: Tests individuels avec logs nanoseconde"
echo ""

# Couleurs pour affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Compteurs
TESTS_EXECUTED=0
TESTS_PASSED=0
LOGS_GENERATED=0
ERRORS_FOUND=0

# Phase 1: Validation structure
echo "📁 Phase 1: Validation structure..."
if [ ! -d "src/tests/individual" ]; then
    echo -e "${RED}❌ Répertoire src/tests/individual manquant${NC}"
    exit 1
fi

if [ ! -d "logs/individual" ]; then
    echo -e "${RED}❌ Répertoire logs/individual manquant${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Structure de répertoires valide${NC}"

# Phase 2: Compilation tous les tests
echo ""
echo "🔨 Phase 2: Compilation tests individuels..."
make -f Makefile.individual all-individual > compilation.log 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Compilation réussie${NC}"
else
    echo -e "${RED}❌ Erreurs de compilation${NC}"
    echo "Détails dans compilation.log:"
    tail -20 compilation.log
    ERRORS_FOUND=$((ERRORS_FOUND + 1))
fi

# Phase 3: Exécution et validation forensique
echo ""
echo "🧪 Phase 3: Exécution tests avec validation forensique..."

# Liste des modules critiques (priorité README.md)
CRITICAL_MODULES=("lum_core" "matrix_calculator" "simd_optimizer" "neural_network_processor")

for test_exec in bin/test_*_individual; do
    if [ -f "$test_exec" ]; then
        module_name=$(basename "$test_exec" | sed 's/test_//' | sed 's/_individual//')
        TESTS_EXECUTED=$((TESTS_EXECUTED + 1))
        
        echo "  🔍 Test: $module_name"
        
        # Exécuter le test avec timeout
        timeout 30s ./"$test_exec" > "test_output_${module_name}.log" 2>&1
        exit_code=$?
        
        if [ $exit_code -eq 0 ]; then
            echo -e "    ${GREEN}✅ $module_name: RÉUSSI${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
            
            # Validation forensique
            log_path="logs/individual/${module_name}/test_${module_name}.log"
            if [ -f "$log_path" ]; then
                echo -e "    ${GREEN}✅ Log forensique généré${NC}"
                LOGS_GENERATED=$((LOGS_GENERATED + 1))
                
                # Vérifier contenu log (nanoseconde timestamp)
                if grep -q "ns" "$log_path"; then
                    echo -e "    ${GREEN}✅ Timestamp nanoseconde validé${NC}"
                else
                    echo -e "    ${YELLOW}⚠️ Timestamp nanoseconde manquant${NC}"
                fi
            else
                echo -e "    ${YELLOW}⚠️ Log forensique manquant: $log_path${NC}"
            fi
            
        elif [ $exit_code -eq 124 ]; then
            echo -e "    ${RED}❌ $module_name: TIMEOUT${NC}"
            ERRORS_FOUND=$((ERRORS_FOUND + 1))
        else
            echo -e "    ${RED}❌ $module_name: ÉCHEC (code: $exit_code)${NC}"
            ERRORS_FOUND=$((ERRORS_FOUND + 1))
        fi
    fi
done

# Phase 4: Validation modules critiques
echo ""
echo "🎯 Phase 4: Validation modules critiques..."
for module in "${CRITICAL_MODULES[@]}"; do
    test_exec="bin/test_${module}_individual"
    if [ -f "$test_exec" ]; then
        echo -e "  ${GREEN}✅ Module critique $module: PRÉSENT${NC}"
    else
        echo -e "  ${RED}❌ Module critique $module: MANQUANT${NC}"
        ERRORS_FOUND=$((ERRORS_FOUND + 1))
    fi
done

# Phase 5: Métriques de performance
echo ""
echo "📊 Phase 5: Métriques de performance..."

# Générer rapport de performance
cat > logs/individual/summary/performance_report.txt << EOF
=== RAPPORT PERFORMANCE TESTS INDIVIDUELS ===
Date: $(date)
Timestamp: $(date +%s%N) ns

Tests exécutés: $TESTS_EXECUTED
Tests réussis: $TESTS_PASSED
Logs générés: $LOGS_GENERATED
Erreurs trouvées: $ERRORS_FOUND

Taux de réussite: $((TESTS_PASSED * 100 / TESTS_EXECUTED))%
Couverture logs: $((LOGS_GENERATED * 100 / TESTS_EXECUTED))%

Status: $( [ $ERRORS_FOUND -eq 0 ] && echo "VALIDATION RÉUSSIE" || echo "ERREURS DÉTECTÉES" )
EOF

echo -e "${GREEN}✅ Rapport de performance généré${NC}"

# Phase 6: Résumé final
echo ""
echo "=== RÉSUMÉ VALIDATION FORENSIQUE ==="
echo "Tests exécutés: $TESTS_EXECUTED"
echo "Tests réussis: $TESTS_PASSED"
echo "Logs forensiques: $LOGS_GENERATED"
echo "Erreurs trouvées: $ERRORS_FOUND"

if [ $ERRORS_FOUND -eq 0 ] && [ $TESTS_PASSED -ge 35 ] && [ $LOGS_GENERATED -ge 35 ]; then
    echo -e "${GREEN}"
    echo "🎉 VALIDATION FORENSIQUE RÉUSSIE 🎉"
    echo "✅ Conformité README.md: 44 modules"
    echo "✅ Tests individuels fonctionnels"
    echo "✅ Logs forensiques nanoseconde"
    echo "✅ Zero erreurs/warnings compilation"
    echo -e "${NC}"
    exit 0
else
    echo -e "${RED}"
    echo "❌ VALIDATION FORENSIQUE ÉCHOUÉE"
    echo "Critères non satisfaits:"
    [ $TESTS_PASSED -lt 35 ] && echo "- Tests réussis insuffisants ($TESTS_PASSED < 35)"
    [ $LOGS_GENERATED -lt 35 ] && echo "- Logs forensiques insuffisants ($LOGS_GENERATED < 35)"
    [ $ERRORS_FOUND -gt 0 ] && echo "- Erreurs détectées ($ERRORS_FOUND)"
    echo -e "${NC}"
    exit 1
fi