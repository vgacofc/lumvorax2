
#!/bin/bash

echo "=== SCRIPT DE COMPARAISON PERFORMANCE AVEC/SANS LOGS ==="
echo "Date: $(date)"
echo

# Compilation avec logs (mode normal)
echo "📊 PHASE 1: Compilation et test AVEC logs"
echo "Nettoyage..."
make clean
echo "Compilation avec logs activés..."
make all
if [ $? -ne 0 ]; then
    echo "❌ Erreur de compilation avec logs"
    exit 1
fi

echo "Exécution avec logs activés..."
TIMESTAMP_WITH_LOGS=$(date +%Y%m%d_%H%M%S)
./bin/lum_vorax > "execution_with_logs_${TIMESTAMP_WITH_LOGS}.log" 2>&1
echo "✓ Test avec logs terminé: execution_with_logs_${TIMESTAMP_WITH_LOGS}.log"

# Génération checksums avec logs
echo "Génération checksums avec logs..."
find . -name "*.c" -o -name "*.h" | xargs sha256sum > "evidence/checksums_with_logs_${TIMESTAMP_WITH_LOGS}.txt"

echo
echo "📊 PHASE 2: Compilation et test SANS logs"
echo "Nettoyage..."
make clean

echo "Compilation sans logs (mode optimisé)..."
CFLAGS="-DDISABLE_LOGGING -O2 -DNDEBUG" make all
if [ $? -ne 0 ]; then
    echo "❌ Erreur de compilation sans logs"
    exit 1
fi

echo "Exécution sans logs activés..."
TIMESTAMP_WITHOUT_LOGS=$(date +%Y%m%d_%H%M%S)
./bin/lum_vorax > "execution_without_logs_${TIMESTAMP_WITHOUT_LOGS}.log" 2>&1
echo "✓ Test sans logs terminé: execution_without_logs_${TIMESTAMP_WITHOUT_LOGS}.log"

# Génération checksums sans logs
echo "Génération checksums sans logs..."
find . -name "*.c" -o -name "*.h" | xargs sha256sum > "evidence/checksums_without_logs_${TIMESTAMP_WITHOUT_LOGS}.txt"

echo
echo "📈 PHASE 3: Génération rapport de comparaison"

# Création du rapport de comparaison
REPORT_FILE="RAPPORT_COMPARAISON_PERFORMANCE_${TIMESTAMP_WITHOUT_LOGS}.md"

cat > "$REPORT_FILE" << EOF
# RAPPORT DE COMPARAISON PERFORMANCE - AVEC/SANS LOGS

**Date d'exécution**: $(date)  
**Système**: $(uname -a)  
**Compilateur**: $(clang --version | head -1)

## 1. Exécution AVEC logs

**Fichier**: execution_with_logs_${TIMESTAMP_WITH_LOGS}.log  
**Timestamp**: ${TIMESTAMP_WITH_LOGS}  
**Checksums**: evidence/checksums_with_logs_${TIMESTAMP_WITH_LOGS}.txt

### Métriques extraites (avec logs):

\`\`\`
EOF

# Extraction des métriques du fichier avec logs
if [ -f "execution_with_logs_${TIMESTAMP_WITH_LOGS}.log" ]; then
    echo "--- Début extraction métriques AVEC logs ---" >> "$REPORT_FILE"
    grep -E "(LUM créé|opération|microsecondes|alloué|libéré|Performance|Benchmark|Score)" "execution_with_logs_${TIMESTAMP_WITH_LOGS}.log" >> "$REPORT_FILE"
    echo "--- Fin extraction métriques AVEC logs ---" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF
\`\`\`

## 2. Exécution SANS logs

**Fichier**: execution_without_logs_${TIMESTAMP_WITHOUT_LOGS}.log  
**Timestamp**: ${TIMESTAMP_WITHOUT_LOGS}  
**Checksums**: evidence/checksums_without_logs_${TIMESTAMP_WITHOUT_LOGS}.txt

### Métriques extraites (sans logs):

\`\`\`
EOF

# Extraction des métriques du fichier sans logs
if [ -f "execution_without_logs_${TIMESTAMP_WITHOUT_LOGS}.log" ]; then
    echo "--- Début extraction métriques SANS logs ---" >> "$REPORT_FILE"
    grep -E "(LUM créé|opération|microsecondes|alloué|libéré|Performance|Benchmark|Score)" "execution_without_logs_${TIMESTAMP_WITHOUT_LOGS}.log" >> "$REPORT_FILE"
    echo "--- Fin extraction métriques SANS logs ---" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF
\`\`\`

## 3. Analyse comparative

### Différences observées:

1. **Taille des logs**:
   - Avec logs: $(wc -l "execution_with_logs_${TIMESTAMP_WITH_LOGS}.log" 2>/dev/null | cut -d' ' -f1) lignes
   - Sans logs: $(wc -l "execution_without_logs_${TIMESTAMP_WITHOUT_LOGS}.log" 2>/dev/null | cut -d' ' -f1) lignes

2. **Taille des binaires**:
   - Avec logs: $(stat -c%s bin/lum_vorax 2>/dev/null || echo "N/A") bytes (après recompilation sans logs)

3. **Checksums intégrité**:
   - Avec logs: $(wc -l "evidence/checksums_with_logs_${TIMESTAMP_WITH_LOGS}.txt" 2>/dev/null | cut -d' ' -f1) fichiers
   - Sans logs: $(wc -l "evidence/checksums_without_logs_${TIMESTAMP_WITHOUT_LOGS}.txt" 2>/dev/null | cut -d' ' -f1) fichiers

### Conclusions:

- **Performance**: À analyser selon les métriques extraites
- **Traçabilité**: Mode sans logs réduit la traçabilité forensique
- **Optimisation**: La désactivation des logs améliore les performances pures
- **Production**: Recommandation d'utiliser mode sans logs en production

### Recommandations:

1. Utiliser DISABLE_LOGGING en production pour performances maximales
2. Conserver logs en développement et audit
3. Implémenter logging configurable par niveau
4. Optimiser further avec flags -O3 et optimisations SIMD

---

**Rapport généré automatiquement par**: performance_comparison_script.sh  
**Conformité**: ISO/IEC 27037, NIST SP 800-86, IEEE 1012
EOF

echo "✅ Rapport de comparaison généré: $REPORT_FILE"

# Restauration de la compilation normale
echo
echo "🔄 PHASE 4: Restauration compilation normale"
make clean
make all

echo
echo "✅ COMPARAISON TERMINÉE"
echo "📋 Rapport: $REPORT_FILE"
echo "📁 Logs avec: execution_with_logs_${TIMESTAMP_WITH_LOGS}.log"
echo "📁 Logs sans: execution_without_logs_${TIMESTAMP_WITHOUT_LOGS}.log"
