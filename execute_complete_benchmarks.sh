
#!/bin/bash

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs"
EVIDENCE_DIR="evidence"

mkdir -p "$LOG_DIR" "$EVIDENCE_DIR"

echo "=== EXÉCUTION COMPLÈTE SYSTÈME LUM/VORAX - $TIMESTAMP ===" | tee "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"

# 1. Compilation avec logs détaillés
echo "001. Compilation du système..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
make clean 2>&1 | tee -a "${LOG_DIR}/compilation_${TIMESTAMP}.log"
make all 2>&1 | tee -a "${LOG_DIR}/compilation_${TIMESTAMP}.log"
COMPILE_STATUS=$?

if [ $COMPILE_STATUS -ne 0 ]; then
    echo "ERREUR: Échec de compilation" | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
    exit 1
fi

# 2. Exécution du programme principal avec métriques
echo "002. Exécution programme principal..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
/usr/bin/time -v ./bin/lum_vorax 2>&1 | tee "${LOG_DIR}/main_execution_${TIMESTAMP}.log"

# 3. Tests Pareto avec métriques détaillées
echo "003. Tests optimisation Pareto..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
/usr/bin/time -v ./src/tests/test_pareto_optimization 2>&1 | tee "${LOG_DIR}/pareto_tests_${TIMESTAMP}.log"

# 4. Tests complets de fonctionnalité
echo "004. Tests complets..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
if [ -f "./test_complete" ]; then
    /usr/bin/time -v ./test_complete 2>&1 | tee "${LOG_DIR}/complete_tests_${TIMESTAMP}.log"
fi

# 5. Benchmark performance avec outils système
echo "005. Benchmark performance..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
if [ -f "./performance_test" ]; then
    /usr/bin/time -v ./performance_test 2>&1 | tee "${LOG_DIR}/performance_${TIMESTAMP}.log"
fi

# 6. Analyse mémoire avec valgrind si disponible
echo "006. Analyse mémoire..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
if command -v valgrind &> /dev/null; then
    valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./bin/lum_vorax 2>&1 | tee "${LOG_DIR}/memory_analysis_${TIMESTAMP}.log"
fi

# 7. Métriques système avant/pendant/après
echo "007. Collecte métriques système..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
echo "CPU Info:" > "${LOG_DIR}/system_metrics_${TIMESTAMP}.log"
cat /proc/cpuinfo | grep "model name" | head -1 >> "${LOG_DIR}/system_metrics_${TIMESTAMP}.log"
echo "Memory Info:" >> "${LOG_DIR}/system_metrics_${TIMESTAMP}.log"
cat /proc/meminfo | head -5 >> "${LOG_DIR}/system_metrics_${TIMESTAMP}.log"
echo "Load Average:" >> "${LOG_DIR}/system_metrics_${TIMESTAMP}.log"
cat /proc/loadavg >> "${LOG_DIR}/system_metrics_${TIMESTAMP}.log"

# 8. Génération checksums pour intégrité
echo "008. Génération checksums..." | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
find . -name "*.c" -o -name "*.h" -o -name "*.log" | xargs sha256sum > "${EVIDENCE_DIR}/checksums_${TIMESTAMP}.txt"

echo "=== EXÉCUTION TERMINÉE - $TIMESTAMP ===" | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
echo "Logs générés dans: ${LOG_DIR}/" | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
echo "Preuves dans: ${EVIDENCE_DIR}/" | tee -a "${LOG_DIR}/execution_complete_${TIMESTAMP}.log"
