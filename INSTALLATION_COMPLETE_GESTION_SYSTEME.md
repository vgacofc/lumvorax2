# INSTALLATION LUMVORAX - GESTIONNAIRE SYSTÈME COMPLET
## Gestion Totale: CPU + RAM + GPU + OpenCL

**Objectif**: Installer LumVorax comme gestionnaire système complet pour optimiser TOUTES les ressources de votre Ubuntu.

---

## 🎯 CE QUE LUMVORAX VA GÉRER

### ✅ Gestion CPU Complète
- **Scheduler optimisé** - Répartition intelligente des tâches
- **Affinité CPU** - Binding threads sur cœurs spécifiques
- **SIMD automatique** - AVX2/AVX-512 pour calculs vectoriels
- **Monitoring temps réel** - Utilisation par cœur

### ✅ Gestion RAM Complète
- **Pool allocator** - Allocation ultra-rapide sans fragmentation
- **Zero-copy** - Évite copies mémoire inutiles
- **Défragmentation auto** - Compactage mémoire périodique
- **Tracking fuites** - Détection automatique memory leaks
- **Hugepages** - Support pages 2MB/1GB pour performance

### ✅ Gestion GPU Complète
- **DRM natif** - Accès direct Intel GPU (i915)
- **OpenCL** - Support calculs parallèles portables
- **Level Zero** - API Intel haute performance
- **Watchdog GPU** - Détection et recovery GPU hang
- **Température** - Monitoring et throttling automatique

### ✅ Orchestration Globale
- **Load balancer** - Répartition charge CPU/GPU
- **Pipeline** - Traitement parallèle 4 étages
- **Forensic logging** - Monitoring nanoseconde
- **Auto-tuning** - Adaptation automatique selon charge

---

## 📦 ÉTAPE 1: INSTALLATION COMPLÈTE DES DÉPENDANCES

### 1.1 Dépendances Système de Base

```bash
# Mise à jour complète
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get dist-upgrade -y

# Outils compilation
sudo apt-get install -y \
    build-essential \
    gcc-12 \
    g++-12 \
    make \
    cmake \
    pkg-config \
    git \
    autoconf \
    automake \
    libtool

# Définir GCC 12 par défaut
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100
```

### 1.2 Bibliothèques Système Complètes

```bash
# Threading et temps réel
sudo apt-get install -y \
    libpthread-stubs0-dev \
    librt-dev \
    libnuma-dev

# Cryptographie
sudo apt-get install -y \
    libssl-dev \
    libcrypto++-dev \
    libgcrypt20-dev

# Mathématiques
sudo apt-get install -y \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    libblas-dev \
    liblapack-dev
```

### 1.3 Support GPU Complet (Intel + OpenCL)

```bash
# Driver Intel i915
sudo apt-get install -y \
    libdrm-dev \
    libdrm-intel1 \
    intel-gpu-tools \
    vainfo \
    mesa-utils \
    mesa-common-dev

# OpenCL (Intel + générique)
sudo apt-get install -y \
    intel-opencl-icd \
    ocl-icd-opencl-dev \
    ocl-icd-libopencl1 \
    opencl-headers \
    clinfo

# Level Zero (Intel haute performance)
sudo apt-get install -y \
    level-zero \
    level-zero-dev

# Vérifier OpenCL installé
clinfo
```

### 1.4 Outils Monitoring et Debug

```bash
# Monitoring système
sudo apt-get install -y \
    htop \
    iotop \
    nethogs \
    sysstat \
    lm-sensors \
    stress-ng

# Debug et profiling
sudo apt-get install -y \
    gdb \
    valgrind \
    strace \
    ltrace \
    perf-tools-unstable

# Configurer sensors
sudo sensors-detect --auto
```

---

## 🔧 ÉTAPE 2: COMPILATION AVEC SUPPORT COMPLET

### 2.1 Vérifier Configuration Système

```bash
cd ~/LVX/lumvorax2

# Vérifier CPU
lscpu | grep -E "Model name|CPU\(s\)|Thread|Core|Socket|Flags"

# Vérifier RAM
free -h
cat /proc/meminfo | grep -E "MemTotal|HugePages"

# Vérifier GPU
lspci | grep -i vga
lsmod | grep i915

# Vérifier OpenCL
clinfo | grep -E "Platform|Device|Version"
```

### 2.2 Configurer Makefile pour Support Complet

```bash
# Backup Makefile original
cp Makefile Makefile.backup

# Éditer Makefile pour ajouter flags OpenCL
cat >> Makefile << 'EOF'

# Support OpenCL complet
OPENCL_CFLAGS = -I/usr/include/CL
OPENCL_LDFLAGS = -lOpenCL

# Support Level Zero
LEVELZERO_CFLAGS = -I/usr/include/level_zero
LEVELZERO_LDFLAGS = -lze_loader

# Ajouter aux CFLAGS
CFLAGS += $(OPENCL_CFLAGS) $(LEVELZERO_CFLAGS) -DENABLE_OPENCL -DENABLE_LEVELZERO

# Ajouter aux LDFLAGS
LDFLAGS += $(OPENCL_LDFLAGS) $(LEVELZERO_LDFLAGS)
EOF
```

### 2.3 Compilation Optimisée Complète

```bash
# Nettoyer
make clean
rm -f bin/* *.o src/**/*.o

# Compiler avec optimisations maximales
make all -j$(nproc) \
    CFLAGS="-O3 -march=native -mtune=native -mavx2 -mfma -fopenmp" \
    LDFLAGS="-lm -lpthread -lrt -lOpenCL -lze_loader -lnuma"

# Vérifier binaires créés
ls -lh bin/
file bin/lum_vorax_complete

# Vérifier dépendances OpenCL
ldd bin/lum_vorax_complete | grep -i opencl
```

---

## 🚀 ÉTAPE 3: INSTALLATION SYSTÈME COMPLÈTE

### 3.1 Installation dans /opt (Système Global)

```bash
# Créer structure système
sudo mkdir -p /opt/lumvorax/{bin,lib,logs,config,data,cache}
sudo mkdir -p /opt/lumvorax/logs/{forensic,execution,gpu,cpu,memory}
sudo mkdir -p /var/log/lumvorax

# Copier binaires
sudo cp bin/* /opt/lumvorax/bin/
sudo cp liblumvorax.so /opt/lumvorax/lib/

# Permissions
sudo chown -R $USER:$USER /opt/lumvorax
sudo chmod -R 755 /opt/lumvorax/bin
sudo chmod -R 644 /opt/lumvorax/lib

# Liens symboliques système
sudo ln -sf /opt/lumvorax/bin/lum_vorax_complete /usr/local/bin/lumvorax
sudo ln -sf /opt/lumvorax/lib/liblumvorax.so /usr/local/lib/
sudo ldconfig
```

### 3.2 Configuration Environnement Global

```bash
# Créer fichier environnement système
sudo tee /etc/profile.d/lumvorax.sh > /dev/null << 'EOF'
# LumVorax System Environment

export LUMVORAX_HOME="/opt/lumvorax"
export PATH="$LUMVORAX_HOME/bin:$PATH"
export LD_LIBRARY_PATH="$LUMVORAX_HOME/lib:$LD_LIBRARY_PATH"

# OpenCL
export OCL_ICD_VENDORS="/etc/OpenCL/vendors"

# Optimisations
export OMP_NUM_THREADS=$(nproc)
export OMP_PROC_BIND=true
export OMP_PLACES=cores
EOF

# Recharger
source /etc/profile.d/lumvorax.sh
```

### 3.3 Configuration Système Complète

```bash
# Créer configuration maître
sudo tee /opt/lumvorax/config/system.conf > /dev/null << 'EOF'
# Configuration LumVorax - Gestionnaire Système Complet

[system]
mode = full_control              # Gestion complète système
auto_start = true                # Démarrage automatique
priority = high                  # Priorité haute

[cpu]
enable_management = true         # Gestion CPU active
num_threads = auto               # Détection automatique
affinity = auto                  # Affinité automatique
simd = avx2                      # AVX2 par défaut (avx512 si disponible)
scheduler = performance          # Governor performance
turbo_boost = true               # Activer turbo

[memory]
enable_management = true         # Gestion RAM active
pool_size = 2147483648          # 2GB pool (ajuster selon RAM)
enable_zero_copy = true
enable_hugepages = true          # Activer hugepages
hugepage_size = 2M               # 2MB pages
defrag_interval = 300            # Défrag toutes les 5 min
max_usage_percent = 80           # Limite 80% RAM

[gpu]
enable_management = true         # Gestion GPU active
backend = auto                   # auto, opencl, levelzero, drm
device_id = 0
max_batch_size = 256000
watchdog_timeout = 5
auto_throttle = true
max_temperature = 80
enable_opencl = true             # Activer OpenCL
opencl_platform = 0              # Premier platform
opencl_device = 0                # Premier device

[monitoring]
enable = true
interval = 1                     # Monitoring chaque seconde
log_level = INFO
enable_forensic = true
enable_temperature = true
enable_power = true

[optimization]
auto_tune = true                 # Auto-tuning actif
learning_rate = 0.01
adaptation_interval = 60         # Adapter toutes les 60s

[debug]
enable_crash_dumps = true
max_log_size_mb = 500
log_rotation = true
EOF
```

---

## ⚙️ ÉTAPE 4: CONFIGURATION KERNEL POUR GESTION COMPLÈTE

### 4.1 Optimisations Kernel

```bash
# Configuration kernel optimisée
sudo tee /etc/sysctl.d/99-lumvorax-system.conf > /dev/null << 'EOF'
# LumVorax - Optimisations Kernel Complètes

# === CPU Scheduler ===
kernel.sched_migration_cost_ns = 5000000
kernel.sched_autogroup_enabled = 0
kernel.sched_latency_ns = 10000000
kernel.sched_min_granularity_ns = 2000000
kernel.sched_wakeup_granularity_ns = 3000000

# === Mémoire ===
vm.swappiness = 1                      # Minimiser swap
vm.vfs_cache_pressure = 50
vm.dirty_ratio = 10
vm.dirty_background_ratio = 5
vm.min_free_kbytes = 65536
vm.zone_reclaim_mode = 0

# Hugepages (ajuster selon RAM)
vm.nr_hugepages = 1024                 # 2GB en pages 2MB
vm.hugetlb_shm_group = 0

# OOM killer
vm.overcommit_memory = 1
vm.overcommit_ratio = 50

# === Réseau ===
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.core.netdev_max_backlog = 5000
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728

# === Sécurité ===
kernel.dmesg_restrict = 0              # Permettre lecture dmesg
kernel.perf_event_paranoid = -1        # Permettre perf
EOF

# Appliquer
sudo sysctl -p /etc/sysctl.d/99-lumvorax-system.conf
```

### 4.2 Configuration Hugepages

```bash
# Activer hugepages au boot
sudo tee -a /etc/default/grub << 'EOF'
GRUB_CMDLINE_LINUX_DEFAULT="$GRUB_CMDLINE_LINUX_DEFAULT hugepagesz=2M hugepages=1024"
EOF

# Mettre à jour GRUB
sudo update-grub

# Vérifier hugepages (après reboot)
cat /proc/meminfo | grep -i huge
```

### 4.3 Configuration CPU Governor

```bash
# Installer cpufrequtils
sudo apt-get install -y cpufrequtils

# Configurer performance mode
sudo tee /etc/default/cpufrequtils > /dev/null << 'EOF'
GOVERNOR="performance"
EOF

# Appliquer immédiatement
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Vérifier
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

---

## 🎮 ÉTAPE 5: CONFIGURATION GPU COMPLÈTE

### 5.1 Permissions GPU Système

```bash
# Créer groupe lumvorax
sudo groupadd -f lumvorax

# Ajouter utilisateur aux groupes GPU
sudo usermod -aG video,render,lumvorax $USER

# Règles udev pour permissions GPU
sudo tee /etc/udev/rules.d/99-lumvorax-gpu.rules > /dev/null << 'EOF'
# LumVorax GPU Permissions
KERNEL=="card[0-9]*", GROUP="video", MODE="0666"
KERNEL=="renderD[0-9]*", GROUP="render", MODE="0666"
SUBSYSTEM=="drm", GROUP="video", MODE="0666"
EOF

# Recharger udev
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 5.2 Configuration OpenCL

```bash
# Vérifier platforms OpenCL
clinfo | grep -E "Platform Name|Device Name"

# Créer configuration OpenCL
mkdir -p ~/.local/share/OpenCL
cat > ~/.local/share/OpenCL/vendors.conf << 'EOF'
# LumVorax OpenCL Configuration
/usr/lib/x86_64-linux-gnu/libintelocl.so
EOF

# Tester OpenCL
cat > /tmp/test_opencl.c << 'EOF'
#include <CL/cl.h>
#include <stdio.h>

int main() {
    cl_uint num_platforms;
    clGetPlatformIDs(0, NULL, &num_platforms);
    printf("OpenCL Platforms: %u\n", num_platforms);
    
    cl_uint num_devices;
    clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
    printf("OpenCL Devices: %u\n", num_devices);
    
    return 0;
}
EOF

gcc /tmp/test_opencl.c -o /tmp/test_opencl -lOpenCL
/tmp/test_opencl
```

### 5.3 Optimisations GPU i915

```bash
# Configuration module i915
sudo tee /etc/modprobe.d/i915.conf > /dev/null << 'EOF'
# LumVorax i915 Optimizations
options i915 enable_guc=3
options i915 enable_fbc=1
options i915 fastboot=1
options i915 enable_psr=1
EOF

# Recharger module (ou reboot)
sudo modprobe -r i915
sudo modprobe i915
```

---

## 🔄 ÉTAPE 6: SERVICE SYSTEMD GESTIONNAIRE SYSTÈME

### 6.1 Créer Service Principal

```bash
sudo tee /etc/systemd/system/lumvorax-manager.service > /dev/null << 'EOF'
[Unit]
Description=LumVorax System Manager - CPU/RAM/GPU Controller
After=network.target multi-user.target
Wants=network.target

[Service]
Type=simple
User=root
Group=root
WorkingDirectory=/opt/lumvorax
ExecStartPre=/bin/sleep 5
ExecStart=/opt/lumvorax/bin/lum_vorax_complete --mode system-manager --daemon
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal
LimitNOFILE=65536
LimitMEMLOCK=infinity

# Priorité temps réel
Nice=-10
IOSchedulingClass=realtime
IOSchedulingPriority=0
CPUSchedulingPolicy=fifo
CPUSchedulingPriority=50

[Install]
WantedBy=multi-user.target
EOF
```

### 6.2 Créer Services Monitoring

```bash
# Service monitoring CPU
sudo tee /etc/systemd/system/lumvorax-cpu-monitor.service > /dev/null << 'EOF'
[Unit]
Description=LumVorax CPU Monitor
After=lumvorax-manager.service
Requires=lumvorax-manager.service

[Service]
Type=simple
ExecStart=/opt/lumvorax/bin/lum_vorax_complete --monitor cpu --interval 1
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# Service monitoring RAM
sudo tee /etc/systemd/system/lumvorax-memory-monitor.service > /dev/null << 'EOF'
[Unit]
Description=LumVorax Memory Monitor
After=lumvorax-manager.service
Requires=lumvorax-manager.service

[Service]
Type=simple
ExecStart=/opt/lumvorax/bin/lum_vorax_complete --monitor memory --interval 1
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# Service monitoring GPU
sudo tee /etc/systemd/system/lumvorax-gpu-monitor.service > /dev/null << 'EOF'
[Unit]
Description=LumVorax GPU Monitor
After=lumvorax-manager.service
Requires=lumvorax-manager.service

[Service]
Type=simple
ExecStart=/opt/lumvorax/bin/lum_vorax_complete --monitor gpu --interval 1
Restart=always

[Install]
WantedBy=multi-user.target
EOF
```

### 6.3 Activer Services

```bash
# Recharger systemd
sudo systemctl daemon-reload

# Activer services
sudo systemctl enable lumvorax-manager.service
sudo systemctl enable lumvorax-cpu-monitor.service
sudo systemctl enable lumvorax-memory-monitor.service
sudo systemctl enable lumvorax-gpu-monitor.service

# Démarrer services
sudo systemctl start lumvorax-manager.service
sudo systemctl start lumvorax-cpu-monitor.service
sudo systemctl start lumvorax-memory-monitor.service
sudo systemctl start lumvorax-gpu-monitor.service

# Vérifier status
sudo systemctl status lumvorax-manager.service
sudo systemctl status lumvorax-*-monitor.service
```

---

## 📊 ÉTAPE 7: DASHBOARD MONITORING COMPLET

### 7.1 Script Dashboard Temps Réel

```bash
sudo tee /opt/lumvorax/bin/lumvorax-dashboard.sh > /dev/null << 'EOF'
#!/bin/bash
# LumVorax System Dashboard - Monitoring Complet

while true; do
    clear
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║          LUMVORAX SYSTEM MANAGER - DASHBOARD                   ║"
    echo "║                  $(date '+%Y-%m-%d %H:%M:%S')                        ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
    
    # === CPU ===
    echo "┌─ CPU ──────────────────────────────────────────────────────────┐"
    echo "│ Utilisation:"
    top -b -n 1 | grep "Cpu(s)" | awk '{print "│   Total: " $2 " user, " $4 " system, " $8 " idle"}'
    echo "│ Fréquence:"
    cat /proc/cpuinfo | grep "cpu MHz" | head -n 1 | awk '{print "│   " $4 " MHz"}'
    echo "│ Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
    echo "│ Threads actifs: $(ps -eLf | wc -l)"
    echo "└────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # === MÉMOIRE ===
    echo "┌─ MÉMOIRE ──────────────────────────────────────────────────────┐"
    free -h | awk 'NR==2{print "│ RAM:  " $3 " / " $2 " (" int($3/$2*100) "%)"}'
    free -h | awk 'NR==3{print "│ Swap: " $3 " / " $2}'
    cat /proc/meminfo | grep "HugePages_Total" | awk '{print "│ HugePages: " $2}'
    echo "│ Cache: $(free -h | awk 'NR==2{print $6}')"
    echo "└────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # === GPU ===
    echo "┌─ GPU ──────────────────────────────────────────────────────────┐"
    if command -v intel_gpu_top &> /dev/null; then
        intel_gpu_top -s 1 -o - 2>/dev/null | head -n 5 | tail -n 3 | sed 's/^/│ /'
    else
        echo "│ intel_gpu_top non disponible"
    fi
    echo "│ Température: $(sensors 2>/dev/null | grep -i temp | head -n 1 | awk '{print $2}' || echo 'N/A')"
    echo "└────────────────────────────────────────────────────────────────┘"
    echo ""
    
    # === LUMVORAX ===
    echo "┌─ LUMVORAX ─────────────────────────────────────────────────────┐"
    if systemctl is-active --quiet lumvorax-manager.service; then
        echo "│ Status: ✓ ACTIF"
        echo "│ Uptime: $(systemctl show lumvorax-manager.service -p ActiveEnterTimestamp --value | xargs -I {} date -d {} +%s | xargs -I {} echo $(($(date +%s) - {})) | awk '{print int($1/3600) "h " int(($1%3600)/60) "m"}')"
    else
        echo "│ Status: ✗ INACTIF"
    fi
    echo "│ Logs: /var/log/lumvorax/"
    echo "└────────────────────────────────────────────────────────────────┘"
    echo ""
    echo "Appuyez sur Ctrl+C pour quitter"
    
    sleep 2
done
EOF

sudo chmod +x /opt/lumvorax/bin/lumvorax-dashboard.sh
```

### 7.2 Commandes Contrôle Système

```bash
# Créer alias dans /etc/profile.d/lumvorax.sh
sudo tee -a /etc/profile.d/lumvorax.sh > /dev/null << 'EOF'

# Alias LumVorax
alias lum-start='sudo systemctl start lumvorax-manager.service'
alias lum-stop='sudo systemctl stop lumvorax-manager.service'
alias lum-restart='sudo systemctl restart lumvorax-manager.service'
alias lum-status='sudo systemctl status lumvorax-*.service'
alias lum-logs='sudo journalctl -u lumvorax-manager.service -f'
alias lum-dashboard='/opt/lumvorax/bin/lumvorax-dashboard.sh'
alias lum-stats='lumvorax --stats'
EOF

source /etc/profile.d/lumvorax.sh
```

---

## ✅ ÉTAPE 8: TESTS DE VALIDATION COMPLÈTE

### 8.1 Test CPU

```bash
# Test gestion CPU
lumvorax --test-cpu --duration 60

# Vérifier affinité
lumvorax --show-cpu-affinity

# Benchmark SIMD
lumvorax --benchmark-simd
```

### 8.2 Test RAM

```bash
# Test allocation mémoire
lumvorax --test-memory --size 1G

# Test hugepages
lumvorax --test-hugepages

# Test zero-copy
lumvorax --test-zero-copy --iterations 1000000
```

### 8.3 Test GPU

```bash
# Test OpenCL
lumvorax --test-opencl --platform 0 --device 0

# Test DRM natif
lumvorax --test-drm

# Benchmark GPU
lumvorax --benchmark-gpu --duration 120
```

### 8.4 Test Intégration Complète

```bash
# Test système complet (CPU+RAM+GPU)
lumvorax --test-system-full --duration 300

# Stress test
lumvorax --stress-test --cpu --memory --gpu --duration 600
```

---

## 🔍 ÉTAPE 9: VÉRIFICATION GESTION SYSTÈME

### 9.1 Vérifier Gestion CPU

```bash
# Vérifier scheduler
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Vérifier affinité processus LumVorax
ps -eLo pid,tid,psr,comm | grep lum

# Vérifier utilisation SIMD
lumvorax --show-simd-usage
```

### 9.2 Vérifier Gestion RAM

```bash
# Vérifier hugepages utilisées
cat /proc/meminfo | grep -i huge

# Vérifier pool allocator
lumvorax --show-memory-pools

# Vérifier fragmentation
lumvorax --show-memory-fragmentation
```

### 9.3 Vérifier Gestion GPU

```bash
# Vérifier GPU actif
intel_gpu_top -l

# Vérifier OpenCL
clinfo | grep -A 5 "Device Name"

# Vérifier température
sensors | grep -i temp

# Logs GPU
sudo journalctl -u lumvorax-gpu-monitor.service -n 50
```

---

## 🎯 RÉSUMÉ: LUMVORAX GÈRE MAINTENANT TOUT

### ✅ Ce qui est Géré Automatiquement

**CPU**:
- ✓ Scheduler optimisé (performance governor)
- ✓ Affinité threads sur cœurs physiques
- ✓ SIMD automatique (AVX2/AVX-512)
- ✓ Turbo boost activé
- ✓ Monitoring temps réel

**RAM**:
- ✓ Pool allocator 2GB
- ✓ Zero-copy activé
- ✓ Hugepages 2MB (1024 pages = 2GB)
- ✓ Défragmentation auto (5 min)
- ✓ Limite 80% utilisation
- ✓ Tracking fuites mémoire

**GPU**:
- ✓ DRM natif i915
- ✓ OpenCL activé
- ✓ Level Zero activé
- ✓ Watchdog 5s
- ✓ Throttling auto si >80°C
- ✓ Monitoring température

**Système**:
- ✓ Services systemd actifs
- ✓ Démarrage automatique
- ✓ Recovery automatique
- ✓ Logging forensique
- ✓ Dashboard temps réel

### 📊 Commandes Quotidiennes

```bash
# Voir dashboard
lum-dashboard

# Voir status
lum-status

# Voir logs
lum-logs

# Voir stats
lum-stats

# Redémarrer si besoin
lum-restart
```

### ⚠️ Points Importants

1. **Reboot requis** après installation pour:
   - Hugepages
   - Permissions GPU
   - Configuration kernel

2. **Monitoring actif** 24/7:
   - CPU: 1 mesure/seconde
   - RAM: 1 mesure/seconde
   - GPU: 1 mesure/seconde

3. **Logs** dans:
   - `/var/log/lumvorax/`
   - `journalctl -u lumvorax-*`

4. **Désactivation** si besoin:
   ```bash
   sudo systemctl stop lumvorax-*.service
   sudo systemctl disable lumvorax-*.service
   ```

---

## 🚀 INSTALLATION RAPIDE - COMMANDES COMPLÈTES

```bash
# 1. Dépendances
sudo apt-get update && sudo apt-get install -y \
    build-essential gcc-12 g++-12 make cmake \
    libssl-dev libdrm-dev libnuma-dev \
    intel-opencl-icd ocl-icd-opencl-dev opencl-headers \
    intel-gpu-tools level-zero level-zero-dev \
    htop sysstat lm-sensors

# 2. Compilation
cd ~/LVX/lumvorax2
make clean
make all -j$(nproc) CFLAGS="-O3 -march=native -mavx2 -fopenmp" LDFLAGS="-lOpenCL -lze_loader -lnuma"

# 3. Installation
sudo mkdir -p /opt/lumvorax/{bin,lib,logs,config}
sudo cp bin/* /opt/lumvorax/bin/
sudo cp liblumvorax.so /opt/lumvorax/lib/
sudo ln -sf /opt/lumvorax/bin/lum_vorax_complete /usr/local/bin/lumvorax

# 4. Configuration
sudo cp INSTALLATION_COMPLETE_GESTION_SYSTEME.md /opt/lumvorax/
# Suivre ÉTAPE 4-6 pour kernel, GPU, services

# 5. Démarrage
sudo systemctl start lumvorax-manager.service
lum-dashboard

# 6. Reboot
sudo reboot
```

---

**INSTALLATION TERMINÉE !**

LumVorax gère maintenant **TOUT** votre système: CPU, RAM, GPU avec OpenCL.

Lancez `lum-dashboard` pour voir la gestion en temps réel !