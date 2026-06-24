#!/bin/bash
################################################################################
# LUMVORAX - VÉRIFICATION ENVIRONNEMENT PRE-INSTALLATION
# Conforme à PROTOCOLE_MASTER_LVX.md (PRE-EXEC-001)
################################################################################

set -e
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  LUMVORAX - VÉRIFICATION ENVIRONNEMENT                    ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}\n"

ERRORS=0

check() {
    if eval "$2" &>/dev/null; then
        echo -e "${GREEN}✅${NC} $1"
    else
        echo -e "${RED}❌${NC} $1"
        ((ERRORS++))
    fi
}

check_warn() {
    if eval "$2" &>/dev/null; then
        echo -e "${GREEN}✅${NC} $1"
    else
        echo -e "${YELLOW}⚠️${NC}  $1 (non critique)"
    fi
}

echo "=== SYSTÈME ==="
check "OS Linux" "[[ \$(uname -s) == 'Linux' ]]"
echo "   $(uname -a)"
check "CPU ≥4 cores" "[[ \$(nproc) -ge 4 ]]"
echo "   Cores: $(nproc)"
check "RAM ≥16 GB" "[[ \$(free -g | grep Mem | awk '{print \$2}') -ge 16 ]]"
echo "   RAM: $(free -h | grep Mem | awk '{print $2}')"
check "Espace disque ≥100 GB" "[[ \$(df -BG . | tail -1 | awk '{print \$4}' | sed 's/G//') -ge 100 ]]"
echo "   Libre: $(df -h . | tail -1 | awk '{print $4}')"

echo -e "\n=== COMPILATEURS ==="
check "GCC installé" "command -v gcc"
[[ $? -eq 0 ]] && echo "   $(gcc --version | head -1)"
check "G++ installé" "command -v g++"
check "Gfortran installé" "command -v gfortran"
check "Make installé" "command -v make"

echo -e "\n=== BIBLIOTHÈQUES ==="
check "OpenBLAS" "ls /usr/lib/x86_64-linux-gnu/libopenblas* 2>/dev/null"
check "LAPACK" "ls /usr/lib/x86_64-linux-gnu/liblapack* 2>/dev/null"

echo -e "\n=== MPI ==="
check "MPI installé" "command -v mpirun"
[[ $? -eq 0 ]] && echo "   $(mpirun --version 2>&1 | head -1)"
check "mpicc disponible" "command -v mpicc"
check "mpicxx disponible" "command -v mpicxx"

echo -e "\n=== MONITORING ==="
check_warn "turbostat" "command -v turbostat"
check_warn "mpstat" "command -v mpstat"
check_warn "sensors" "command -v sensors"
check_warn "intel_gpu_top" "command -v intel_gpu_top"

echo -e "\n=== GPU INTEL ==="
if lspci | grep -i "vga.*intel" &>/dev/null; then
    echo -e "${GREEN}✅${NC} GPU Intel détecté"
    lspci | grep -i "vga.*intel" | sed 's/^/   /'
else
    echo -e "${YELLOW}⚠️${NC}  GPU Intel non détecté (Level Zero Native peut ne pas fonctionner)"
fi

echo -e "\n=== OUTILS ==="
check "wget" "command -v wget"
check "curl" "command -v curl"
check "git" "command -v git"

echo -e "\n════════════════════════════════════════════════════════════"
if [[ $ERRORS -eq 0 ]]; then
    echo -e "${GREEN}✅ ENVIRONNEMENT PRÊT POUR INSTALLATION${NC}"
    echo -e "\nProchaine étape:"
    echo -e "  ./master_lvx/scripts/install_benchmarks.sh"
    exit 0
else
    echo -e "${RED}❌ $ERRORS ERREUR(S) DÉTECTÉE(S)${NC}"
    echo -e "\nInstallez les dépendances manquantes:"
    echo -e "  sudo apt update"
    echo -e "  sudo apt install -y build-essential gcc g++ gfortran make"
    echo -e "  sudo apt install -y libopenblas-dev liblapack-dev"
    echo -e "  sudo apt install -y openmpi-bin libopenmpi-dev"
    exit 1
fi
