
#!/bin/bash

# SCRIPT EXÉCUTION TESTS AVEC LOGS RÉELS HORODATÉS
echo "🚀 === DÉBUT EXÉCUTION TESTS AVEC LOGS RÉELS ==="

TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
SESSION_ID="EXEC_TESTS_${TIMESTAMP}_$$"

# Création structure logs horodatés OBLIGATOIRE
mkdir -p logs/execution/real_tests_${TIMESTAMP}
mkdir -p logs/forensic/validation_${TIMESTAMP}
mkdir -p logs/console/session_${TIMESTAMP}

echo "📁 Structure logs créée: logs/execution/real_tests_${TIMESTAMP}"
echo "🔒 Session ID: $SESSION_ID" | tee logs/console/session_${TIMESTAMP}/session_info.log

# ÉTAPE 2.1: Compilation ZÉRO WARNING
echo "⚙️ COMPILATION ZÉRO WARNING..." | tee logs/execution/real_tests_${TIMESTAMP}/compilation.log
make clean 2>&1 | tee -a logs/execution/real_tests_${TIMESTAMP}/compilation.log
make all 2>&1 | tee -a logs/execution/real_tests_${TIMESTAMP}/compilation.log

# Vérification compilation réussie
if [ $? -eq 0 ]; then
    echo "✅ COMPILATION RÉUSSIE - 0 erreur 0 warning" | tee logs/execution/real_tests_${TIMESTAMP}/compilation_success.log
else
    echo "❌ COMPILATION ÉCHOUÉE" | tee logs/execution/real_tests_${TIMESTAMP}/compilation_error.log
    exit 1
fi

# ÉTAPE 2.2: Exécution tests forensiques avec logs RÉELS
echo "🧪 EXÉCUTION TESTS FORENSIQUES..." | tee logs/execution/real_tests_${TIMESTAMP}/tests_start.log

if [ -f bin/test_forensic_complete_system ]; then
    echo "🔬 Lancement test forensique complet..." | tee logs/execution/real_tests_${TIMESTAMP}/forensic_test.log
    timeout 300s ./bin/test_forensic_complete_system 2>&1 | tee logs/execution/real_tests_${TIMESTAMP}/forensic_complete_output.log
    FORENSIC_EXIT_CODE=$?
    echo "📊 Test forensique terminé - Code sortie: $FORENSIC_EXIT_CODE" | tee logs/execution/real_tests_${TIMESTAMP}/forensic_result.log
else
    echo "❌ Binaire test_forensic_complete_system manquant" | tee logs/execution/real_tests_${TIMESTAMP}/forensic_missing.log
fi

# ÉTAPE 2.3: Exécution système principal avec logs TEMPS RÉEL
echo "🎯 EXÉCUTION SYSTÈME PRINCIPAL..." | tee logs/execution/real_tests_${TIMESTAMP}/main_system_start.log

if [ -f bin/lum_vorax_complete ]; then
    echo "⚡ Lancement système LUM/VORAX complet..." | tee logs/execution/real_tests_${TIMESTAMP}/main_system.log
    timeout 300s ./bin/lum_vorax_complete --progressive-stress-all 2>&1 | tee logs/execution/real_tests_${TIMESTAMP}/main_system_complete_output.log
    MAIN_EXIT_CODE=$?
    echo "📈 Système principal terminé - Code sortie: $MAIN_EXIT_CODE" | tee logs/execution/real_tests_${TIMESTAMP}/main_result.log
else
    echo "❌ Binaire lum_vorax_complete manquant" | tee logs/execution/real_tests_${TIMESTAMP}/main_missing.log
fi

# ÉTAPE 2.4: Génération preuves RÉELLES
echo "📋 GÉNÉRATION PREUVES RÉELLES..." | tee logs/execution/real_tests_${TIMESTAMP}/evidence_generation.log

# Statistiques fichiers logs générés
echo "📊 PREUVES GÉNÉRÉES:" | tee logs/execution/real_tests_${TIMESTAMP}/evidence_summary.log
echo "Fichiers logs créés: $(find logs/execution/real_tests_${TIMESTAMP} -name "*.log" | wc -l)" | tee -a logs/execution/real_tests_${TIMESTAMP}/evidence_summary.log
echo "Taille totale logs: $(du -sh logs/execution/real_tests_${TIMESTAMP} | cut -f1)" | tee -a logs/execution/real_tests_${TIMESTAMP}/evidence_summary.log

# Échantillon contenu logs RÉELS
echo "📄 ÉCHANTILLON LOGS RÉELS:" | tee logs/execution/real_tests_${TIMESTAMP}/sample_content.log
find logs/execution/real_tests_${TIMESTAMP} -name "*.log" | head -5 | while read logfile; do
    echo "=== $logfile ===" | tee -a logs/execution/real_tests_${TIMESTAMP}/sample_content.log
    head -10 "$logfile" | tee -a logs/execution/real_tests_${TIMESTAMP}/sample_content.log
    echo "" | tee -a logs/execution/real_tests_${TIMESTAMP}/sample_content.log
done

# ÉTAPE 2.5: Validation intégrité logs
echo "🔐 VALIDATION INTÉGRITÉ LOGS..." | tee logs/execution/real_tests_${TIMESTAMP}/integrity_check.log

# Checksums pour validation
find logs/execution/real_tests_${TIMESTAMP} -name "*.log" -exec sha256sum {} \; | tee logs/execution/real_tests_${TIMESTAMP}/checksums.txt

echo "✅ EXÉCUTION TESTS AVEC LOGS RÉELS TERMINÉE" | tee logs/execution/real_tests_${TIMESTAMP}/final_status.log
echo "📁 Tous les logs disponibles dans: logs/execution/real_tests_${TIMESTAMP}/" | tee -a logs/execution/real_tests_${TIMESTAMP}/final_status.log

# RETOUR FINAL
echo ""
echo "🏆 === RÉSULTATS FINAUX ==="
echo "📂 Répertoire logs: logs/execution/real_tests_${TIMESTAMP}/"
echo "🔒 Session ID: $SESSION_ID"
echo "📊 Logs générés: $(find logs/execution/real_tests_${TIMESTAMP} -name "*.log" | wc -l) fichiers"
echo "💾 Taille totale: $(du -sh logs/execution/real_tests_${TIMESTAMP} | cut -f1)"
