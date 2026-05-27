#!/bin/bash
# C553 - INSTALLATION COMPLÈTE STACK INTEL COMPUTE
# Objectif: Installer TOUS les modules Intel pour batch buffers automatique

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="logs/install_c553_${TIMESTAMP}.log"

mkdir -p logs

echo "=== C553 - INSTALLATION STACK INTEL COMPUTE (0%) ===" | tee "$LOG_FILE"
echo "Timestamp: $TIMESTAMP" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Mise à jour cache APT
echo "[5%] Mise à jour cache APT..." | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S apt update 2>&1 | tee -a "$LOG_FILE"

# Installation Intel Graphics Compiler (IGC)
echo "" | tee -a "$LOG_FILE"
echo "[10%] Installation Intel Graphics Compiler..." | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S apt install -y \
    libigc1 \
    libigc-dev \
    libigc-tools \
    libigdfcl1 \
    libigdfcl-dev \
    2>&1 | tee -a "$LOG_FILE"

# Installation Intel OpenCL Runtime
echo "" | tee -a "$LOG_FILE"
echo "[20%] Installation Intel OpenCL Runtime..." | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S apt install -y \
    intel-opencl-icd \
    ocl-icd-libopencl1 \
    ocl-icd-opencl-dev \
    opencl-headers \
    2>&1 | tee -a "$LOG_FILE"

# Installation Intel GPU Tools
echo "" | tee -a "$LOG_FILE"
echo "[30%] Installation Intel GPU Tools..." | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S apt install -y \
    intel-gpu-tools \
    2>&1 | tee -a "$LOG_FILE"

# Installation bibliothèques développement
echo "" | tee -a "$LOG_FILE"
echo "[40%] Installation bibliothèques développement..." | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S apt install -y \
    libdrm-dev \
    libdrm-intel1 \
    libdrm2 \
    mesa-common-dev \
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev \
    2>&1 | tee -a "$LOG_FILE"

# Installation outils compilation
echo "" | tee -a "$LOG_FILE"
echo "[50%] Installation outils compilation..." | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S apt install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    2>&1 | tee -a "$LOG_FILE"

# Vérification installations
echo "" | tee -a "$LOG_FILE"
echo "[60%] Vérification installations..." | tee -a "$LOG_FILE"

# Vérifier IGC
if [ -f "/usr/lib/x86_64-linux-gnu/libigc.so" ]; then
    echo "✓ IGC installé" | tee -a "$LOG_FILE"
    ls -lh /usr/lib/x86_64-linux-gnu/libigc* 2>&1 | tee -a "$LOG_FILE"
else
    echo "❌ IGC manquant" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"

# Vérifier OpenCL
if [ -f "/usr/lib/x86_64-linux-gnu/libOpenCL.so" ]; then
    echo "✓ OpenCL installé" | tee -a "$LOG_FILE"
    ls -lh /usr/lib/x86_64-linux-gnu/libOpenCL* 2>&1 | tee -a "$LOG_FILE"
else
    echo "❌ OpenCL manquant" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"

# Vérifier intel-opencl-icd
if [ -f "/etc/OpenCL/vendors/intel.icd" ]; then
    echo "✓ Intel OpenCL ICD installé" | tee -a "$LOG_FILE"
    cat /etc/OpenCL/vendors/intel.icd 2>&1 | tee -a "$LOG_FILE"
else
    echo "❌ Intel OpenCL ICD manquant" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"

# Lister tous les fichiers Intel installés
echo "[70%] Inventaire fichiers Intel..." | tee -a "$LOG_FILE"
dpkg -L libigc1 libigdfcl1 intel-opencl-icd 2>/dev/null | grep -E "\.(so|a|h)$" | head -50 | tee -a "$LOG_FILE"

echo "" | tee -a "$LOG_FILE"

# Tester OpenCL
echo "[80%] Test OpenCL..." | tee -a "$LOG_FILE"
if command -v clinfo &> /dev/null; then
    clinfo 2>&1 | head -50 | tee -a "$LOG_FILE"
else
    echo "clinfo non disponible, installation..." | tee -a "$LOG_FILE"
    echo "emmaus" | sudo -S apt install -y clinfo 2>&1 | tee -a "$LOG_FILE"
    clinfo 2>&1 | head -50 | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"

# Créer répertoire pour sources Intel
echo "[90%] Préparation téléchargement sources Intel..." | tee -a "$LOG_FILE"
mkdir -p intel_sources
cd intel_sources

# Cloner Intel Compute Runtime (si pas déjà fait)
if [ ! -d "compute-runtime" ]; then
    echo "Clonage Intel Compute Runtime..." | tee -a "../$LOG_FILE"
    git clone --depth 1 https://github.com/intel/compute-runtime.git 2>&1 | tee -a "../$LOG_FILE"
else
    echo "✓ Intel Compute Runtime déjà cloné" | tee -a "../$LOG_FILE"
fi

# Cloner Intel Graphics Compiler (si pas déjà fait)
if [ ! -d "intel-graphics-compiler" ]; then
    echo "Clonage Intel Graphics Compiler..." | tee -a "../$LOG_FILE"
    git clone --depth 1 https://github.com/intel/intel-graphics-compiler.git 2>&1 | tee -a "../$LOG_FILE"
else
    echo "✓ Intel Graphics Compiler déjà cloné" | tee -a "../$LOG_FILE"
fi

cd ..

echo "" | tee -a "$LOG_FILE"
echo "[100%] Installation complète!" | tee -a "$LOG_FILE"

# Résumé
echo "" | tee -a "$LOG_FILE"
echo "=== RÉSUMÉ C553 ===" | tee -a "$LOG_FILE"
echo "Modules installés:" | tee -a "$LOG_FILE"
echo "  ✓ Intel Graphics Compiler (IGC)" | tee -a "$LOG_FILE"
echo "  ✓ Intel OpenCL Runtime" | tee -a "$LOG_FILE"
echo "  ✓ Intel GPU Tools" | tee -a "$LOG_FILE"
echo "  ✓ Bibliothèques DRM/Mesa" | tee -a "$LOG_FILE"
echo "  ✓ Outils compilation" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"
echo "Sources clonées:" | tee -a "$LOG_FILE"
echo "  ✓ intel/compute-runtime" | tee -a "$LOG_FILE"
echo "  ✓ intel/intel-graphics-compiler" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"
echo "Prochaine étape: C554 - Développer système automatique batch buffers" | tee -a "$LOG_FILE"
echo "Log complet: $LOG_FILE" | tee -a "$LOG_FILE"

exit 0

# Made with Bob
