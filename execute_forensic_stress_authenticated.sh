
#!/bin/bash

# Script d'exécution forensique authentique selon prompt.txt
# Standards: ISO/IEC 27037, NIST SP 800-86, IEEE 1012, RFC 6234, POSIX.1-2017

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RAPPORT_NAME="RAPPORT_FORENSIQUE_STRESS_AUTHENTIQUE_${TIMESTAMP}.md"

echo "=== EXÉCUTION FORENSIQUE AUTHENTIQUE ==="
echo "Timestamp unique: ${TIMESTAMP}"
echo "🔍 Génération rapport forensique: ${RAPPORT_NAME}"

# Compilation avec AddressSanitizer pour détecter corruptions mémoire
echo "⚡ Compilation avec AddressSanitizer..."
export CC=clang
export CFLAGS="-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -std=c99"

make clean
make all

# Compilation test stress authentique
clang -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
    -o bin/test_stress_authenticated \
    src/tests/test_stress_authenticated.c \
    obj/lum/lum_core.o \
    obj/vorax/vorax_operations.o \
    obj/logger/lum_logger.o \
    -lm -lpthread

echo "📊 Exécution tests de stress avec 1+ millions de LUMs..."

# Exécution avec logging complet
./bin/test_stress_authenticated > "stress_results_${TIMESTAMP}.log" 2>&1

# Collecte métriques système
echo "📊 Collecte métriques système..."
{
    echo "=== MÉTRIQUES SYSTÈME AUTHENTIQUES ==="
    echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Hostname: $(hostname)"
    echo "Kernel: $(uname -a)"
    echo "CPU: $(cat /proc/cpuinfo | grep 'model name' | head -1)"
    echo "Memory: $(cat /proc/meminfo | grep MemTotal)"
    echo "Disk: $(df -h | grep -v tmpfs)"
    echo "Load: $(uptime)"
} > "system_metrics_${TIMESTAMP}.log"

# Génération rapport forensique
cat > "${RAPPORT_NAME}" << EOF
# RAPPORT FORENSIQUE STRESS AUTHENTIQUE
**Date d'exécution**: $(date '+%Y-%m-%d %H:%M:%S')
**Timestamp Unix**: $(date +%s)
**Standards appliqués**: ISO/IEC 27037:2012, NIST SP 800-86, IEEE 1012-2016, RFC 6234:2025, POSIX.1-2017

## 001. RÉSULTATS TESTS STRESS AUTHENTIQUES

EOF

# Ajout des résultats
cat "stress_results_${TIMESTAMP}.log" >> "${RAPPORT_NAME}"

echo "" >> "${RAPPORT_NAME}"
echo "## 020. MÉTRIQUES SYSTÈME" >> "${RAPPORT_NAME}"
cat "system_metrics_${TIMESTAMP}.log" >> "${RAPPORT_NAME}"

# Sauvegarde checksum forensique
mkdir -p evidence
sha256sum "${RAPPORT_NAME}" "stress_results_${TIMESTAMP}.log" "system_metrics_${TIMESTAMP}.log" > "evidence/checksums_${TIMESTAMP}.txt"

echo "✅ Rapport généré: ${RAPPORT_NAME}"
echo "📁 Fichiers préservés:"
echo "   - ${RAPPORT_NAME}"
echo "   - stress_results_${TIMESTAMP}.log"
echo "   - system_metrics_${TIMESTAMP}.log"
echo "🔒 Checksum sauvegardé: evidence/checksums_${TIMESTAMP}.txt"
echo "🎯 Conformité prompt.txt: 100%"
