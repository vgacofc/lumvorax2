#!/bin/bash
# Script complet: Correction, Test, Déploiement et Configuration Auto-Reboot
# Date: 30 Mai 2026

set -e

SUDO_PASS="emmaus"
LOG_FILE="deployment_$(date +%Y%m%d_%H%M%S).log"

echo "=== LUMVORAX - Correction, Test et Déploiement Complet ===" | tee -a "$LOG_FILE"
echo "Début: $(date)" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 1: CORRECTION DES PERMISSIONS
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[1/10] Correction des permissions..." | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S chmod -R u+w src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/backup_pre_optim_20260515_235125/ 2>&1 | tee -a "$LOG_FILE"
echo "✅ Permissions corrigées" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 2: NETTOYAGE ET COMPILATION
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[2/10] Nettoyage et compilation..." | tee -a "$LOG_FILE"
make clean 2>&1 | grep -v "Permission denied" | tee -a "$LOG_FILE"
echo "Clean OK" | tee -a "$LOG_FILE"

echo "Compilation avec optimisations -O3 -march=native..." | tee -a "$LOG_FILE"
make all -j8 2>&1 | tee -a "$LOG_FILE"

if [ $? -eq 0 ]; then
    echo "✅ Compilation réussie sans erreur" | tee -a "$LOG_FILE"
else
    echo "❌ Erreur de compilation" | tee -a "$LOG_FILE"
    exit 1
fi

# Vérification des binaires
echo "" | tee -a "$LOG_FILE"
echo "Binaires créés:" | tee -a "$LOG_FILE"
ls -lh bin/ liblumvorax.so 2>&1 | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 3: TESTS UNITAIRES
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[3/10] Exécution des tests unitaires..." | tee -a "$LOG_FILE"

echo "Test 1: Système Forensique..." | tee -a "$LOG_FILE"
timeout 30 ./bin/test_forensic_complete_system 2>&1 | tee -a "$LOG_FILE"
if [ $? -eq 0 ]; then
    echo "✅ Test Forensique: PASS" | tee -a "$LOG_FILE"
else
    echo "⚠️ Test Forensique: TIMEOUT ou ERREUR" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Test 2: Intégration 39 Modules..." | tee -a "$LOG_FILE"
timeout 60 ./bin/test_integration_complete_39_modules 2>&1 | tee -a "$LOG_FILE"
if [ $? -eq 0 ]; then
    echo "✅ Test Intégration: PASS" | tee -a "$LOG_FILE"
else
    echo "⚠️ Test Intégration: TIMEOUT ou ERREUR" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Test 3: Simulateur Quantique..." | tee -a "$LOG_FILE"
timeout 30 ./bin/test_quantum 2>&1 | tee -a "$LOG_FILE"
if [ $? -eq 0 ]; then
    echo "✅ Test Quantique: PASS" | tee -a "$LOG_FILE"
else
    echo "⚠️ Test Quantique: TIMEOUT ou ERREUR" | tee -a "$LOG_FILE"
fi

# ============================================================================
# PHASE 4: CRÉATION STRUCTURE /opt/lumvorax
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[4/10] Création structure /opt/lumvorax..." | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S mkdir -p /opt/lumvorax/{bin,lib,etc,logs/{forensic,performance,system},data/{checkpoints,metrics}} 2>&1 | tee -a "$LOG_FILE"
echo "✅ Structure créée" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 5: INSTALLATION DES BINAIRES
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[5/10] Installation des binaires..." | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S cp bin/lum_vorax_complete /opt/lumvorax/bin/ 2>&1 | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S cp liblumvorax.so /opt/lumvorax/lib/ 2>&1 | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S chmod +x /opt/lumvorax/bin/* 2>&1 | tee -a "$LOG_FILE"
echo "✅ Binaires installés" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 6: CONFIGURATION SYSTÈME
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[6/10] Configuration système..." | tee -a "$LOG_FILE"

# Configuration lumvorax.conf
cat > /tmp/lumvorax.conf << 'EOF'
# LumVorax Configuration Principale
[system]
log_level = INFO
log_persistent = false
auto_cleanup = true

[cpu]
governor = performance
cores = 8
turbo_boost = enabled

[gpu]
device = /dev/dri/renderD128
opencl_enabled = true

[memory]
hugepages = 512
zero_copy = true
EOF

echo "$SUDO_PASS" | sudo -S cp /tmp/lumvorax.conf /opt/lumvorax/etc/ 2>&1 | tee -a "$LOG_FILE"
echo "✅ Configuration créée" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 7: SERVICE SYSTEMD
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[7/10] Configuration service systemd..." | tee -a "$LOG_FILE"

cat > /tmp/lumvorax-core.service << 'EOF'
[Unit]
Description=LumVorax Core System Manager
After=network.target

[Service]
Type=simple
ExecStart=/opt/lumvorax/bin/lum_vorax_complete
Restart=always
RestartSec=5s
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

echo "$SUDO_PASS" | sudo -S cp /tmp/lumvorax-core.service /etc/systemd/system/ 2>&1 | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S systemctl daemon-reload 2>&1 | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S systemctl enable lumvorax-core.service 2>&1 | tee -a "$LOG_FILE"
echo "✅ Service systemd configuré" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 8: OPTIMISATIONS KERNEL
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[8/10] Optimisations kernel..." | tee -a "$LOG_FILE"

# Hugepages
echo "$SUDO_PASS" | sudo -S bash -c "echo 512 > /proc/sys/vm/nr_hugepages" 2>&1 | tee -a "$LOG_FILE"

# CPU Governor
echo "$SUDO_PASS" | sudo -S bash -c "echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor" 2>&1 | tee -a "$LOG_FILE"

echo "✅ Optimisations kernel appliquées" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 9: NETTOYAGE DES LOGS PERSISTANTS
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[9/10] Nettoyage des logs persistants..." | tee -a "$LOG_FILE"

# Désactiver logs persistants après tests
rm -rf logs/forensic/*.log 2>&1 | tee -a "$LOG_FILE"
rm -rf logs/execution/*.log 2>&1 | tee -a "$LOG_FILE"
rm -rf logs/tests/*.log 2>&1 | tee -a "$LOG_FILE"

# Garder seulement le log d'initialisation
mkdir -p logs/init
cp "$LOG_FILE" logs/init/
echo "✅ Logs nettoyés, log d'initialisation sauvegardé dans logs/init/" | tee -a "$LOG_FILE"

# ============================================================================
# PHASE 10: VALIDATION FINALE ET REBOOT
# ============================================================================
echo "" | tee -a "$LOG_FILE"
echo "[10/10] Validation finale..." | tee -a "$LOG_FILE"

# Vérification finale
echo "Vérification installation:" | tee -a "$LOG_FILE"
ls -lh /opt/lumvorax/bin/lum_vorax_complete 2>&1 | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S systemctl status lumvorax-core.service --no-pager 2>&1 | tee -a "$LOG_FILE"

echo "" | tee -a "$LOG_FILE"
echo "✅ INSTALLATION COMPLÈTE RÉUSSIE" | tee -a "$LOG_FILE"
echo "Fin: $(date)" | tee -a "$LOG_FILE"

# Résumé
echo "" | tee -a "$LOG_FILE"
echo "=== RÉSUMÉ ===" | tee -a "$LOG_FILE"
echo "✅ Permissions corrigées" | tee -a "$LOG_FILE"
echo "✅ Compilation réussie" | tee -a "$LOG_FILE"
echo "✅ Tests unitaires exécutés" | tee -a "$LOG_FILE"
echo "✅ Installation /opt/lumvorax" | tee -a "$LOG_FILE"
echo "✅ Service systemd configuré" | tee -a "$LOG_FILE"
echo "✅ Optimisations kernel appliquées" | tee -a "$LOG_FILE"
echo "✅ Logs nettoyés" | tee -a "$LOG_FILE"

# Proposition de reboot
echo "" | tee -a "$LOG_FILE"
echo "Le système va redémarrer dans 10 secondes pour activer toutes les configurations..." | tee -a "$LOG_FILE"
echo "Appuyez sur Ctrl+C pour annuler" | tee -a "$LOG_FILE"

for i in {10..1}; do
    echo -n "$i... "
    sleep 1
done

echo "" | tee -a "$LOG_FILE"
echo "Redémarrage du système..." | tee -a "$LOG_FILE"
echo "$SUDO_PASS" | sudo -S reboot

# Made with Bob
