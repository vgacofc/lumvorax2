# PLAN D'INSTALLATION LUMVORAX - USAGE PERSONNEL UBUNTU

**Objectif**: Installer LumVorax sur votre système Ubuntu actuel pour optimiser les performances de calcul, GPU et gestion système personnelle.

**Contexte**: Installation locale, usage personnel uniquement, pas de production.

---

## ÉTAPE 1: PRÉPARATION DU SYSTÈME

### 1.1 Vérifier Votre Configuration Actuelle

```bash
# Vérifier version Ubuntu
cat /etc/os-release

# Vérifier kernel
uname -r

# Vérifier GPU Intel
lspci | grep -i vga

# Vérifier mémoire disponible
free -h

# Vérifier espace disque
df -h
```

### 1.2 Installer les Dépendances Requises

```bash
# Mise à jour système
sudo apt-get update
sudo apt-get upgrade -y

# Outils de compilation
sudo apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    pkg-config \
    git

# Bibliothèques système
sudo apt-get install -y \
    libpthread-stubs0-dev \
    libssl-dev \
    libcrypto++-dev \
    libdrm-dev \
    libdrm-intel1

# Support GPU Intel
sudo apt-get install -y \
    intel-gpu-tools \
    vainfo \
    mesa-utils

# Outils monitoring (optionnel)
sudo apt-get install -y \
    htop \
    iotop \
    nethogs \
    sysstat
```

---

## ÉTAPE 2: COMPILATION DE LUMVORAX

### 2.1 Naviguer vers le Répertoire

```bash
cd ~/LVX/lumvorax2
```

### 2.2 Vérifier le Makefile

```bash
# Afficher le Makefile
cat Makefile

# Vérifier les sources listées
grep "SOURCES =" Makefile
```

### 2.3 Compilation Complète

```bash
# Nettoyer compilations précédentes
make clean

# Compiler avec tous les cœurs CPU
make all -j$(nproc)

# Vérifier la compilation
ls -lh bin/
```

**Résultat attendu**:
```
bin/lum_vorax_complete
bin/test_forensic_complete_system
bin/test_integration_complete_39_modules
bin/test_quantum
liblumvorax.so
```

---

## ÉTAPE 3: INSTALLATION LOCALE

### 3.1 Créer Structure de Répertoires

```bash
# Créer répertoires dans votre HOME
mkdir -p ~/.lumvorax/{bin,lib,logs,config,data}
mkdir -p ~/.lumvorax/logs/{forensic,execution,tests,gpu}

# Copier binaires
cp bin/* ~/.lumvorax/bin/
cp liblumvorax.so ~/.lumvorax/lib/

# Rendre exécutables
chmod +x ~/.lumvorax/bin/*
```

### 3.2 Configuration Environnement

```bash
# Ajouter à votre ~/.bashrc
cat >> ~/.bashrc << 'EOF'

# LumVorax Environment
export LUMVORAX_HOME="$HOME/.lumvorax"
export PATH="$LUMVORAX_HOME/bin:$PATH"
export LD_LIBRARY_PATH="$LUMVORAX_HOME/lib:$LD_LIBRARY_PATH"

# Alias utiles
alias lum='$LUMVORAX_HOME/bin/lum_vorax_complete'
alias lum-stats='$LUMVORAX_HOME/bin/lum_vorax_complete --stats'
alias lum-logs='tail -f $LUMVORAX_HOME/logs/forensic/*.log'
EOF

# Recharger configuration
source ~/.bashrc
```

### 3.3 Créer Fichier de Configuration

```bash
cat > ~/.lumvorax/config/lumvorax.conf << 'EOF'
# Configuration LumVorax - Usage Personnel

[core]
max_lums = 10000000          # 10M LUMs max (ajuster selon RAM)
pool_size = 1048576          # 1M pool size
enable_forensic_logging = true
log_level = INFO

[gpu]
enable_gpu = true
device_id = 0                # Premier GPU Intel
max_batch_size = 256000      # 256K nonces par batch
watchdog_timeout_sec = 5
auto_throttle = true
max_temperature_celsius = 80

[memory]
enable_zero_copy = true
enable_pool_allocator = true
enable_simd = true           # AVX2/AVX-512 si disponible
defrag_interval_sec = 300    # Défragmentation toutes les 5 min

[optimization]
cpu_affinity = auto          # Affinité CPU automatique
numa_aware = false           # Désactiver si mono-socket
hugepages = false            # Désactiver par défaut

[monitoring]
enable_stats = true
stats_interval_sec = 10
enable_temperature_monitor = true
enable_memory_monitor = true

[debug]
log_rotation_mb = 100        # Rotation à 100MB
max_log_files = 5
enable_crash_dumps = true
EOF
```

---

## ÉTAPE 4: CONFIGURATION GPU INTEL

### 4.1 Vérifier Permissions GPU

```bash
# Vérifier devices DRM
ls -la /dev/dri/

# Ajouter votre utilisateur aux groupes GPU
sudo usermod -aG video $USER
sudo usermod -aG render $USER

# Vérifier appartenance groupes
groups $USER
```

**⚠️ IMPORTANT**: Vous devez vous **déconnecter et reconnecter** pour que les permissions prennent effet.

### 4.2 Tester Accès GPU

```bash
# Après reconnexion, tester
vainfo

# Vérifier capacités GPU
intel_gpu_top -l

# Tester rendering
glxinfo | grep "OpenGL renderer"
```

---

## ÉTAPE 5: TESTS DE VALIDATION

### 5.1 Test Basique

```bash
# Test version
lum --version

# Test configuration
lum --test-config

# Test basique (rapide)
lum --test-basic
```

### 5.2 Test Mémoire (1M LUMs)

```bash
# Test allocation mémoire
lum --test-memory --count 1000000

# Vérifier logs
tail -n 50 ~/.lumvorax/logs/forensic/lumvorax.log
```

### 5.3 Test GPU (Si Intel GPU disponible)

```bash
# Test GPU court (10 secondes)
lum --test-gpu --duration 10

# Monitorer température pendant test
watch -n 1 'sensors | grep -i temp'
```

### 5.4 Test Complet (Optionnel)

```bash
# Test progressif 1M → 10M LUMs
lum --progressive-stress-all --target 10000000

# Durée estimée: 5-10 minutes
```

---

## ÉTAPE 6: INTÉGRATION SYSTÈME PERSONNEL

### 6.1 Créer Scripts Utilitaires

#### Script de Monitoring

```bash
cat > ~/.lumvorax/bin/lum-monitor.sh << 'EOF'
#!/bin/bash
# Monitoring LumVorax en temps réel

echo "=== MONITORING LUMVORAX ==="
echo ""

while true; do
    clear
    echo "=== $(date) ==="
    echo ""
    
    # Stats LumVorax
    echo "--- LumVorax Stats ---"
    $LUMVORAX_HOME/bin/lum_vorax_complete --stats 2>/dev/null || echo "LumVorax non actif"
    echo ""
    
    # CPU
    echo "--- CPU ---"
    top -b -n 1 | head -n 5
    echo ""
    
    # Mémoire
    echo "--- Mémoire ---"
    free -h
    echo ""
    
    # GPU (si Intel)
    echo "--- GPU Intel ---"
    intel_gpu_top -s 1 -o - 2>/dev/null | head -n 10 || echo "GPU non disponible"
    echo ""
    
    # Température
    echo "--- Température ---"
    sensors 2>/dev/null | grep -E "Core|temp" || echo "Sensors non disponible"
    
    sleep 5
done
EOF

chmod +x ~/.lumvorax/bin/lum-monitor.sh
```

#### Script de Nettoyage Logs

```bash
cat > ~/.lumvorax/bin/lum-clean-logs.sh << 'EOF'
#!/bin/bash
# Nettoyage logs LumVorax

echo "Nettoyage logs LumVorax..."

# Logs > 7 jours
find ~/.lumvorax/logs -type f -mtime +7 -delete

# Logs > 100MB
find ~/.lumvorax/logs -type f -size +100M -delete

echo "Nettoyage terminé"
du -sh ~/.lumvorax/logs
EOF

chmod +x ~/.lumvorax/bin/lum-clean-logs.sh
```

#### Script de Benchmark Personnel

```bash
cat > ~/.lumvorax/bin/lum-benchmark.sh << 'EOF'
#!/bin/bash
# Benchmark personnel LumVorax

echo "=== BENCHMARK LUMVORAX PERSONNEL ==="
echo ""

# Test 1: Mémoire (1M LUMs)
echo "[1/4] Test Mémoire (1M LUMs)..."
time lum --test-memory --count 1000000
echo ""

# Test 2: GPU (30 secondes)
echo "[2/4] Test GPU (30s)..."
time lum --test-gpu --duration 30
echo ""

# Test 3: SIMD Optimizer
echo "[3/4] Test SIMD..."
time lum --test-simd --iterations 1000000
echo ""

# Test 4: Stress complet (5M LUMs)
echo "[4/4] Test Stress (5M LUMs)..."
time lum --progressive-stress-all --target 5000000
echo ""

echo "=== BENCHMARK TERMINÉ ==="
echo "Logs: ~/.lumvorax/logs/forensic/"
EOF

chmod +x ~/.lumvorax/bin/lum-benchmark.sh
```

### 6.2 Tâche Cron pour Maintenance (Optionnel)

```bash
# Ajouter nettoyage automatique hebdomadaire
(crontab -l 2>/dev/null; echo "0 3 * * 0 $HOME/.lumvorax/bin/lum-clean-logs.sh") | crontab -

# Vérifier crontab
crontab -l
```

---

## ÉTAPE 7: UTILISATION QUOTIDIENNE

### 7.1 Commandes de Base

```bash
# Lancer calcul simple
lum --create-lums 1000000

# Opérations VORAX
lum --vorax-fuse --count 500000

# Monitoring en temps réel
lum-monitor.sh

# Voir logs
lum-logs

# Stats rapides
lum-stats
```

### 7.2 Cas d'Usage Personnels

#### A. Calculs Intensifs

```bash
# Calcul matrice grande taille
lum --matrix-multiply --size 1000x1000

# Calcul neural network
lum --neural-compute --layers 10 --neurons 1000
```

#### B. Mining Bitcoin (Test/Apprentissage)

```bash
# Mining test (NE PAS utiliser pour vrai mining)
lum --btc-mining-test --duration 60 --testnet

# Voir hashrate
lum --btc-stats
```

#### C. Optimisation GPU

```bash
# Benchmark GPU
lum --gpu-benchmark --duration 120

# Test stabilité GPU
lum --gpu-stress --duration 600  # 10 minutes
```

### 7.3 Debugging et Forensic

```bash
# Activer mode debug
lum --debug-mode --verbose

# Analyser crash
lum --analyze-crash ~/.lumvorax/logs/crash_dump_*.log

# Vérifier fuites mémoire
lum --memory-leak-check --duration 300
```

---

## ÉTAPE 8: OPTIMISATIONS SYSTÈME PERSONNEL

### 8.1 Optimiser Kernel (Optionnel)

```bash
# Créer fichier sysctl personnel
sudo tee /etc/sysctl.d/99-lumvorax-personal.conf > /dev/null << 'EOF'
# Optimisations LumVorax Personnel

# Mémoire
vm.swappiness = 10                    # Réduire swap
vm.vfs_cache_pressure = 50            # Garder cache
vm.dirty_ratio = 15                   # Écriture disque
vm.dirty_background_ratio = 5

# Réseau (si utilisation réseau)
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216

# Scheduler
kernel.sched_migration_cost_ns = 5000000
kernel.sched_autogroup_enabled = 0
EOF

# Appliquer
sudo sysctl -p /etc/sysctl.d/99-lumvorax-personal.conf
```

### 8.2 Optimiser CPU Governor

```bash
# Passer en mode performance (pour calculs intensifs)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Revenir en mode powersave (économie énergie)
echo powersave | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### 8.3 Désactiver Services Inutiles (Optionnel)

```bash
# Lister services actifs
systemctl list-units --type=service --state=running

# Désactiver services non essentiels (ATTENTION: à adapter selon vos besoins)
# Exemples (NE PAS copier aveuglément):
# sudo systemctl disable bluetooth.service
# sudo systemctl disable cups.service
```

---

## ÉTAPE 9: RÉSOLUTION PROBLÈMES COURANTS

### 9.1 Problème: Compilation Échoue

```bash
# Vérifier dépendances manquantes
sudo apt-get install -f

# Réinstaller build-essential
sudo apt-get install --reinstall build-essential

# Vérifier version GCC
gcc --version  # Doit être >= 9.0
```

### 9.2 Problème: GPU Non Détecté

```bash
# Vérifier driver i915
lsmod | grep i915

# Recharger driver
sudo modprobe -r i915
sudo modprobe i915

# Vérifier firmware
ls -la /lib/firmware/i915/
```

### 9.3 Problème: Permissions GPU

```bash
# Vérifier groupes
groups

# Si video/render manquants, ajouter:
sudo usermod -aG video,render $USER

# IMPORTANT: Se déconnecter/reconnecter
```

### 9.4 Problème: Crash GPU

```bash
# Vérifier dmesg pour erreurs GPU
dmesg | grep -i "i915\|drm\|gpu"

# Réduire batch size dans config
nano ~/.lumvorax/config/lumvorax.conf
# Changer: max_batch_size = 128000  (au lieu de 256000)

# Activer watchdog
# Changer: watchdog_timeout_sec = 3  (au lieu de 5)
```

### 9.5 Problème: Fuites Mémoire

```bash
# Activer tracking mémoire détaillé
lum --memory-tracker --verbose

# Forcer garbage collection
lum --force-gc

# Redémarrer si nécessaire
```

---

## ÉTAPE 10: MAINTENANCE RÉGULIÈRE

### 10.1 Checklist Hebdomadaire

```bash
# 1. Nettoyer logs
~/.lumvorax/bin/lum-clean-logs.sh

# 2. Vérifier santé système
lum --health-check

# 3. Mettre à jour si nécessaire
cd ~/LVX/lumvorax2
git pull
make clean && make all -j$(nproc)
cp bin/* ~/.lumvorax/bin/
```

### 10.2 Backup Configuration

```bash
# Sauvegarder config
cp ~/.lumvorax/config/lumvorax.conf ~/.lumvorax/config/lumvorax.conf.backup

# Sauvegarder logs importants
tar -czf ~/lumvorax-logs-$(date +%Y%m%d).tar.gz ~/.lumvorax/logs/
```

---

## RÉSUMÉ: COMMENT LUMVORAX AMÉLIORE VOTRE SYSTÈME

### Ce que LumVorax Fait pour Vous

1. **Optimisation Calculs**
   - Utilise SIMD (AVX2/AVX-512) pour calculs 4-8× plus rapides
   - Pool allocator réduit fragmentation mémoire
   - Zero-copy évite copies mémoire inutiles

2. **Gestion GPU Intelligente**
   - Accès direct DRM (pas de overhead OpenCL/CUDA)
   - Pipeline asynchrone pour latence minimale
   - Watchdog automatique évite GPU hang

3. **Monitoring Avancé**
   - Logging forensique nanoseconde
   - Tracking mémoire complet
   - Métriques performance temps réel

4. **Debugging Puissant**
   - Détection fuites mémoire automatique
   - Analyse crash détaillée
   - Profiling performance

### Ce que LumVorax NE Fait PAS

- ❌ Ne remplace PAS le kernel Linux
- ❌ Ne modifie PAS les fichiers système
- ❌ Ne nécessite PAS de reboot
- ❌ Ne bloque PAS d'autres applications
- ❌ N'est PAS un virus/malware

### Architecture d'Intégration

```
Votre Système Ubuntu
├── Kernel Linux (inchangé)
├── Drivers système (inchangés)
├── Applications normales (inchangées)
└── LumVorax (couche optimisation)
    ├── Calculs intensifs → LumVorax
    ├── GPU tasks → LumVorax GPU Manager
    └── Monitoring → LumVorax Forensic
```

---

## COMMANDES RAPIDES - AIDE-MÉMOIRE

```bash
# Installation initiale
cd ~/LVX/lumvorax2 && make clean && make all -j$(nproc)
cp bin/* ~/.lumvorax/bin/

# Tests rapides
lum --test-basic                    # Test basique
lum --test-memory --count 1000000   # Test mémoire
lum --test-gpu --duration 30        # Test GPU

# Utilisation quotidienne
lum --stats                         # Stats système
lum-monitor.sh                      # Monitoring temps réel
lum-logs                            # Voir logs

# Maintenance
lum-clean-logs.sh                   # Nettoyer logs
lum --health-check                  # Vérifier santé

# Debugging
lum --debug-mode --verbose          # Mode debug
lum --memory-leak-check             # Vérifier fuites
```

---

## PROCHAINES ÉTAPES

1. **Maintenant**: Suivre ÉTAPE 1-5 pour installation de base
2. **Après tests**: Configurer selon vos besoins (ÉTAPE 6-8)
3. **Usage régulier**: Utiliser commandes quotidiennes (ÉTAPE 7)
4. **Optimisation**: Ajuster configuration selon résultats

---

**Questions Fréquentes**

**Q: Est-ce dangereux pour mon système ?**
R: Non, LumVorax s'installe dans votre HOME (~/.lumvorax) et ne modifie pas les fichiers système.

**Q: Puis-je désinstaller facilement ?**
R: Oui, simplement: `rm -rf ~/.lumvorax` et retirer les lignes dans ~/.bashrc

**Q: Ça consomme beaucoup de ressources ?**
R: Seulement quand vous l'utilisez. Au repos, 0% CPU/GPU.

**Q: Compatible avec mes autres logiciels ?**
R: Oui, LumVorax ne bloque aucune application.

**Q: Besoin de redémarrer ?**
R: Non, sauf pour permissions GPU (déconnexion/reconnexion suffit).

---

**FIN DU PLAN D'INSTALLATION**

Prêt à installer ? Commencez par l'ÉTAPE 1 !