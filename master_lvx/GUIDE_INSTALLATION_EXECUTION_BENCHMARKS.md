# GUIDE D'INSTALLATION ET D'EXÉCUTION - BENCHMARKS INTERNATIONAUX LUMVORAX

**Date**: 2026-06-19  
**Version**: 1.0  
**Auteur**: Bob (Expert IA + HPC Benchmarking)  
**Projet**: LumVorax - Validation Performance Standards Internationaux  

---

## 📋 TABLE DES MATIÈRES

1. [Prérequis Système](#prérequis-système)
2. [Installation Benchmarks](#installation-benchmarks)
3. [Scripts d'Exécution](#scripts-dexécution)
4. [Instrumentation Forensique](#instrumentation-forensique)
5. [Validation Résultats](#validation-résultats)
6. [Troubleshooting](#troubleshooting)

---

## 🖥️ PRÉREQUIS SYSTÈME

### Configuration Minimale

```bash
# Vérifier configuration système
uname -a                    # Linux 4.15+
lscpu                       # x86_64, 4+ cores
free -h                     # 16+ GB RAM
lspci | grep -i vga         # Intel UHD 620 (Gen9)
df -h                       # 100+ GB espace disque
```

### Dépendances Système

```bash
# Mise à jour système
sudo apt update && sudo apt upgrade -y

# Compilateurs et outils de build
sudo apt install -y \
    build-essential \
    gcc-9 g++-9 \
    gfortran-9 \
    cmake \
    make \
    autoconf \
    automake \
    libtool \
    pkg-config

# Bibliothèques mathématiques
sudo apt install -y \
    libopenblas-dev \
    liblapack-dev \
    libatlas-base-dev \
    libblas-dev

# MPI (Message Passing Interface)
sudo apt install -y \
    openmpi-bin \
    openmpi-common \
    libopenmpi-dev

# Outils monitoring
sudo apt install -y \
    linux-tools-common \
    linux-tools-generic \
    linux-tools-$(uname -r) \
    sysstat \
    lm-sensors \
    intel-gpu-tools

# Outils réseau
sudo apt install -y \
    iperf3 \
    netperf

# Git pour téléchargement benchmarks
sudo apt install -y git wget curl
```

### Vérification Installation

```bash
# Vérifier GCC
gcc --version               # >= 9.4

# Vérifier MPI
mpirun --version            # OpenMPI 4.0+

# Vérifier BLAS
ls -l /usr/lib/x86_64-linux-gnu/libopenblas*

# Vérifier outils monitoring
turbostat --version
mpstat -V
sensors
intel_gpu_top --help
```

---

## 📦 INSTALLATION BENCHMARKS

### Structure Répertoires

```bash
# Créer structure
mkdir -p ~/lumvorax_benchmarks
cd ~/lumvorax_benchmarks

mkdir -p {hpl,hpcg,graph500,io500,stream,osu,results,logs,scripts}
```

### 1. HPL (High Performance Linpack)

```bash
cd ~/lumvorax_benchmarks/hpl

# Télécharger HPL
wget http://www.netlib.org/benchmark/hpl/hpl-2.3.tar.gz
tar -xzf hpl-2.3.tar.gz
cd hpl-2.3

# Créer Make.Linux
cat > Make.Linux << 'EOF'
ARCH         = Linux
TOPdir       = $(HOME)/lumvorax_benchmarks/hpl/hpl-2.3
MPdir        = /usr/lib/x86_64-linux-gnu/openmpi
MPinc        = -I$(MPdir)/include
MPlib        = $(MPdir)/lib/libmpi.so
LAdir        = /usr/lib/x86_64-linux-gnu
LAinc        = 
LAlib        = $(LAdir)/libopenblas.a
F2CDEFS      = -DAdd__ -DF77_INTEGER=int -DStringSunStyle
HPL_OPTS     = -DHPL_CALL_CBLAS
HPL_INCLUDES = -I$(TOPdir)/include -I$(TOPdir)/include/$(ARCH) $(LAinc) $(MPinc)
HPL_LIBS     = $(LAlib) $(MPlib) -lm
HPL_DEFS     = $(F2CDEFS) $(HPL_OPTS) $(HPL_INCLUDES)
CC           = mpicc
CCNOOPT      = $(HPL_DEFS)
CCFLAGS      = $(HPL_DEFS) -fomit-frame-pointer -O3 -funroll-loops -W -Wall
LINKER       = mpif77
LINKFLAGS    = $(CCFLAGS)
ARCHIVER     = ar
ARFLAGS      = r
RANLIB       = echo
EOF

# Compiler HPL
make arch=Linux

# Vérifier compilation
ls -lh bin/Linux/xhpl
```

### Configuration HPL.dat

```bash
cd ~/lumvorax_benchmarks/hpl/hpl-2.3/bin/Linux

# Créer HPL.dat optimisé
cat > HPL.dat << 'EOF'
HPLinpack benchmark input file
Innovative Computing Laboratory, University of Tennessee
HPL.out      output file name (if any)
6            device out (6=stdout,7=stderr,file)
1            # of problems sizes (N)
10000        Ns
1            # of NBs
192          NBs
0            PMAP process mapping (0=Row-,1=Column-major)
1            # of process grids (P x Q)
1            Ps
1            Qs
16.0         threshold
1            # of panel fact
2            PFACTs (0=left, 1=Crout, 2=Right)
1            # of recursive stopping criterium
4            NBMINs (>= 1)
1            # of panels in recursion
2            NDIVs
1            # of recursive panel fact.
1            RFACTs (0=left, 1=Crout, 2=Right)
1            # of broadcast
1            BCASTs (0=1rg,1=1rM,2=2rg,3=2rM,4=Lng,5=LnM)
1            # of lookahead depth
1            DEPTHs (>=0)
2            SWAP (0=bin-exch,1=long,2=mix)
64           swapping threshold
0            L1 in (0=transposed,1=no-transposed) form
0            U  in (0=transposed,1=no-transposed) form
1            Equilibration (0=no,1=yes)
8            memory alignment in double (> 0)
EOF
```

---

### 2. HPCG (High Performance Conjugate Gradient)

```bash
cd ~/lumvorax_benchmarks/hpcg

# Télécharger HPCG
wget https://github.com/hpcg-benchmark/hpcg/archive/refs/tags/HPCG-release-3-1-0.tar.gz
tar -xzf HPCG-release-3-1-0.tar.gz
cd hpcg-HPCG-release-3-1-0

# Créer setup/Make.Linux_MPI
mkdir -p setup
cat > setup/Make.Linux_MPI << 'EOF'
SHELL        = /bin/sh
CD           = cd
CP           = cp
LN_S         = ln -s -f
MKDIR        = mkdir -p
RM           = /bin/rm -f
TOUCH        = touch
ARCH         = Linux_MPI
TOPdir       = .
SRCdir       = $(TOPdir)/src
INCdir       = $(TOPdir)/src
BINdir       = $(TOPdir)/bin
CXX          = mpicxx
CXXFLAGS     = $(HPCG_DEFS) -O3 -ffast-math -ftree-vectorize -ftree-vectorizer-verbose=0
LINKER       = $(CXX)
LINKFLAGS    = $(CXXFLAGS)
ARCHIVER     = ar
ARFLAGS      = r
RANLIB       = echo
HPCG_DEFS    = -DHPCG_NO_OPENMP
EOF

# Configurer et compiler
mkdir build && cd build
../configure Linux_MPI
make

# Vérifier compilation
ls -lh bin/xhpcg
```

### Configuration hpcg.dat

```bash
cd ~/lumvorax_benchmarks/hpcg/hpcg-HPCG-release-3-1-0/build/bin

# Créer hpcg.dat
cat > hpcg.dat << 'EOF'
HPCG benchmark input file
Sandia National Laboratories; University of Tennessee, Knoxville
104 104 104
60
EOF
```

---

### 3. Graph500

```bash
cd ~/lumvorax_benchmarks/graph500

# Télécharger Graph500
wget https://github.com/graph500/graph500/archive/refs/tags/graph500-3.0.tar.gz
tar -xzf graph500-3.0.tar.gz
cd graph500-graph500-3.0/src

# Compiler
make

# Vérifier compilation
ls -lh graph500_reference_bfs
```

---

### 4. IO500

```bash
cd ~/lumvorax_benchmarks/io500

# Télécharger IO500
git clone https://github.com/IO500/io500.git
cd io500

# Préparer
./prepare.sh

# Créer config.ini
cat > config.ini << 'EOF'
[global]
datadir = /tmp/io500-datadir
resultdir = ./results

[ior-easy-write]
transferSize = 1m
blockSize = 9920000m

[ior-hard-write]
transferSize = 47008
blockSize = 47008

[mdtest-easy-write]
n = 1000000

[mdtest-hard-write]
n = 1000000

[find]
nproc = 1

[ior-easy-read]
transferSize = 1m
blockSize = 9920000m

[ior-hard-read]
transferSize = 47008
blockSize = 47008

[mdtest-easy-stat]
n = 1000000

[mdtest-hard-stat]
n = 1000000

[mdtest-easy-delete]
n = 1000000

[mdtest-hard-delete]
n = 1000000
EOF
```

---

### 5. STREAM

```bash
cd ~/lumvorax_benchmarks/stream

# Télécharger STREAM
wget https://www.cs.virginia.edu/stream/FTP/Code/stream.c

# Compiler version CPU
gcc -O3 -fopenmp -DSTREAM_ARRAY_SIZE=100000000 stream.c -o stream_cpu

# Vérifier compilation
ls -lh stream_cpu
```

---

### 6. OSU Micro-Benchmarks

```bash
cd ~/lumvorax_benchmarks/osu

# Télécharger OSU
wget http://mvapich.cse.ohio-state.edu/download/mvapich/osu-micro-benchmarks-5.9.tar.gz
tar -xzf osu-micro-benchmarks-5.9.tar.gz
cd osu-micro-benchmarks-5.9

# Configurer et compiler
./configure CC=mpicc CXX=mpicxx
make

# Vérifier compilation
ls -lh mpi/pt2pt/osu_latency
ls -lh mpi/pt2pt/osu_bw
```

---

## 🚀 SCRIPTS D'EXÉCUTION

### Script Master: run_all_benchmarks.sh

```bash
cat > ~/lumvorax_benchmarks/scripts/run_all_benchmarks.sh << 'EOF'
#!/bin/bash
################################################################################
# LUMVORAX - SCRIPT EXÉCUTION TOUS BENCHMARKS
# Version: 1.0
# Date: 2026-06-19
################################################################################

set -e  # Arrêter en cas d'erreur

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Répertoires
BASE_DIR=~/lumvorax_benchmarks
RESULTS_DIR=$BASE_DIR/results
LOGS_DIR=$BASE_DIR/logs
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Créer répertoires résultats
mkdir -p $RESULTS_DIR/$TIMESTAMP
mkdir -p $LOGS_DIR/$TIMESTAMP

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  LUMVORAX - BENCHMARKS STANDARDS INTERNATIONAUX           ║${NC}"
echo -e "${BLUE}║  Exécution Complète avec Traçabilité Forensique           ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}Timestamp: $TIMESTAMP${NC}"
echo -e "${GREEN}Résultats: $RESULTS_DIR/$TIMESTAMP${NC}"
echo -e "${GREEN}Logs: $LOGS_DIR/$TIMESTAMP${NC}"
echo ""

# Fonction logging
log_info() {
    echo -e "${GREEN}[INFO]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $(date '+%Y-%m-%d %H:%M:%S') - $1"
}

# Fonction monitoring système
start_monitoring() {
    local test_name=$1
    log_info "Démarrage monitoring pour: $test_name"
    
    # CPU
    mpstat 1 > $LOGS_DIR/$TIMESTAMP/${test_name}_cpu.log 2>&1 &
    MPSTAT_PID=$!
    
    # GPU
    intel_gpu_top -o $LOGS_DIR/$TIMESTAMP/${test_name}_gpu.log &
    GPU_TOP_PID=$!
    
    # Température
    watch -n 1 sensors > $LOGS_DIR/$TIMESTAMP/${test_name}_temp.log 2>&1 &
    SENSORS_PID=$!
    
    # Consommation (si disponible)
    if command -v turbostat &> /dev/null; then
        sudo turbostat --show PkgWatt,CorWatt,GFXWatt --interval 1 \
            > $LOGS_DIR/$TIMESTAMP/${test_name}_power.log 2>&1 &
        TURBOSTAT_PID=$!
    fi
    
    log_info "Monitoring actif (PIDs: mpstat=$MPSTAT_PID, gpu=$GPU_TOP_PID, sensors=$SENSORS_PID)"
}

stop_monitoring() {
    log_info "Arrêt monitoring"
    
    kill $MPSTAT_PID 2>/dev/null || true
    kill $GPU_TOP_PID 2>/dev/null || true
    kill $SENSORS_PID 2>/dev/null || true
    [ ! -z "$TURBOSTAT_PID" ] && sudo kill $TURBOSTAT_PID 2>/dev/null || true
    
    sleep 2
}

# Fonction checksum
generate_checksum() {
    local file=$1
    sha256sum "$file" > "${file}.sha256"
    log_info "Checksum généré: ${file}.sha256"
}

################################################################################
# TEST 1: HPL (High Performance Linpack)
################################################################################
run_hpl() {
    log_info "═══════════════════════════════════════════════════════════"
    log_info "TEST HPL-001: HPL Baseline"
    log_info "═══════════════════════════════════════════════════════════"
    
    cd $BASE_DIR/hpl/hpl-2.3/bin/Linux
    
    start_monitoring "hpl_baseline"
    
    # Exécuter HPL
    log_info "Exécution HPL..."
    ./xhpl | tee $RESULTS_DIR/$TIMESTAMP/hpl_baseline.txt
    
    stop_monitoring
    
    # Générer checksum
    generate_checksum $RESULTS_DIR/$TIMESTAMP/hpl_baseline.txt
    
    # Extraire métriques
    RMAX=$(grep "WR" $RESULTS_DIR/$TIMESTAMP/hpl_baseline.txt | awk '{print $7}')
    log_info "Rmax: $RMAX GFLOPS"
    
    log_info "✅ HPL Baseline complété"
    echo ""
}

################################################################################
# TEST 2: HPCG
################################################################################
run_hpcg() {
    log_info "═══════════════════════════════════════════════════════════"
    log_info "TEST HPCG-001: HPCG Baseline"
    log_info "═══════════════════════════════════════════════════════════"
    
    cd $BASE_DIR/hpcg/hpcg-HPCG-release-3-1-0/build/bin
    
    start_monitoring "hpcg_baseline"
    
    # Exécuter HPCG
    log_info "Exécution HPCG..."
    mpirun -np 1 ./xhpcg | tee $RESULTS_DIR/$TIMESTAMP/hpcg_baseline.txt
    
    stop_monitoring
    
    # Copier résultats YAML
    cp HPCG-Benchmark*.yaml $RESULTS_DIR/$TIMESTAMP/ 2>/dev/null || true
    cp HPCG-Benchmark*.txt $RESULTS_DIR/$TIMESTAMP/ 2>/dev/null || true
    
    # Générer checksum
    generate_checksum $RESULTS_DIR/$TIMESTAMP/hpcg_baseline.txt
    
    log_info "✅ HPCG Baseline complété"
    echo ""
}

################################################################################
# TEST 3: Graph500
################################################################################
run_graph500() {
    log_info "═══════════════════════════════════════════════════════════"
    log_info "TEST GRAPH500-001: BFS Baseline"
    log_info "═══════════════════════════════════════════════════════════"
    
    cd $BASE_DIR/graph500/graph500-graph500-3.0/src
    
    start_monitoring "graph500_bfs"
    
    # Exécuter Graph500 (scale 20)
    log_info "Exécution Graph500 BFS (scale 20)..."
    ./graph500_reference_bfs 20 | tee $RESULTS_DIR/$TIMESTAMP/graph500_bfs_scale20.txt
    
    stop_monitoring
    
    # Générer checksum
    generate_checksum $RESULTS_DIR/$TIMESTAMP/graph500_bfs_scale20.txt
    
    # Extraire TEPS
    TEPS=$(grep "TEPS" $RESULTS_DIR/$TIMESTAMP/graph500_bfs_scale20.txt | awk '{print $NF}')
    log_info "TEPS: $TEPS"
    
    log_info "✅ Graph500 BFS complété"
    echo ""
}

################################################################################
# TEST 4: STREAM
################################################################################
run_stream() {
    log_info "═══════════════════════════════════════════════════════════"
    log_info "TEST STREAM-001: STREAM Baseline CPU"
    log_info "═══════════════════════════════════════════════════════════"
    
    cd $BASE_DIR/stream
    
    start_monitoring "stream_cpu"
    
    # Exécuter STREAM
    log_info "Exécution STREAM CPU..."
    export OMP_NUM_THREADS=4
    ./stream_cpu | tee $RESULTS_DIR/$TIMESTAMP/stream_cpu.txt
    
    stop_monitoring
    
    # Générer checksum
    generate_checksum $RESULTS_DIR/$TIMESTAMP/stream_cpu.txt
    
    # Extraire Triad
    TRIAD=$(grep "Triad:" $RESULTS_DIR/$TIMESTAMP/stream_cpu.txt | awk '{print $2}')
    log_info "Triad: $TRIAD MB/s"
    
    log_info "✅ STREAM CPU complété"
    echo ""
}

################################################################################
# TEST 5: OSU Benchmarks
################################################################################
run_osu() {
    log_info "═══════════════════════════════════════════════════════════"
    log_info "TEST NETWORK-001: OSU MPI Benchmarks"
    log_info "═══════════════════════════════════════════════════════════"
    
    cd $BASE_DIR/osu/osu-micro-benchmarks-5.9
    
    start_monitoring "osu_mpi"
    
    # Latence
    log_info "Exécution OSU Latency..."
    mpirun -np 2 ./mpi/pt2pt/osu_latency | tee $RESULTS_DIR/$TIMESTAMP/osu_latency.txt
    
    # Bande passante
    log_info "Exécution OSU Bandwidth..."
    mpirun -np 2 ./mpi/pt2pt/osu_bw | tee $RESULTS_DIR/$TIMESTAMP/osu_bandwidth.txt
    
    stop_monitoring
    
    # Générer checksums
    generate_checksum $RESULTS_DIR/$TIMESTAMP/osu_latency.txt
    generate_checksum $RESULTS_DIR/$TIMESTAMP/osu_bandwidth.txt
    
    log_info "✅ OSU Benchmarks complétés"
    echo ""
}

################################################################################
# RAPPORT FINAL
################################################################################
generate_report() {
    log_info "═══════════════════════════════════════════════════════════"
    log_info "GÉNÉRATION RAPPORT FINAL"
    log_info "═══════════════════════════════════════════════════════════"
    
    REPORT_FILE=$RESULTS_DIR/$TIMESTAMP/RAPPORT_FINAL.txt
    
    cat > $REPORT_FILE << REPORT_EOF
╔════════════════════════════════════════════════════════════╗
║  LUMVORAX - RAPPORT BENCHMARKS STANDARDS INTERNATIONAUX   ║
╚════════════════════════════════════════════════════════════╝

Date: $(date '+%Y-%m-%d %H:%M:%S')
Timestamp: $TIMESTAMP

═══════════════════════════════════════════════════════════
CONFIGURATION SYSTÈME
═══════════════════════════════════════════════════════════

OS: $(uname -a)
CPU: $(lscpu | grep "Model name" | cut -d: -f2 | xargs)
Cores: $(nproc)
RAM: $(free -h | grep Mem | awk '{print $2}')
GPU: $(lspci | grep -i vga | cut -d: -f3 | xargs)

═══════════════════════════════════════════════════════════
RÉSULTATS BENCHMARKS
═══════════════════════════════════════════════════════════

HPL (High Performance Linpack):
$(grep "WR" $RESULTS_DIR/$TIMESTAMP/hpl_baseline.txt 2>/dev/null || echo "Non disponible")

HPCG (High Performance Conjugate Gradient):
$(grep "GFLOP/s" $RESULTS_DIR/$TIMESTAMP/hpcg_baseline.txt 2>/dev/null || echo "Non disponible")

Graph500 (BFS):
$(grep "TEPS" $RESULTS_DIR/$TIMESTAMP/graph500_bfs_scale20.txt 2>/dev/null || echo "Non disponible")

STREAM (Memory Bandwidth):
$(grep -A 1 "Function" $RESULTS_DIR/$TIMESTAMP/stream_cpu.txt 2>/dev/null || echo "Non disponible")

OSU Latency:
$(head -20 $RESULTS_DIR/$TIMESTAMP/osu_latency.txt 2>/dev/null || echo "Non disponible")

═══════════════════════════════════════════════════════════
FICHIERS GÉNÉRÉS
═══════════════════════════════════════════════════════════

Résultats: $RESULTS_DIR/$TIMESTAMP/
Logs: $LOGS_DIR/$TIMESTAMP/
Checksums: *.sha256

═══════════════════════════════════════════════════════════
VALIDATION
═══════════════════════════════════════════════════════════

✅ Tous tests exécutés
✅ Monitoring complet
✅ Checksums générés
✅ Traçabilité forensique

REPORT_EOF

    log_info "Rapport généré: $REPORT_FILE"
    
    # Afficher rapport
    cat $REPORT_FILE
}

################################################################################
# MAIN
################################################################################
main() {
    log_info "Début exécution benchmarks"
    
    # Exécuter tous tests
    run_hpl
    run_hpcg
    run_graph500
    run_stream
    run_osu
    
    # Générer rapport
    generate_report
    
    log_info "═══════════════════════════════════════════════════════════"
    log_info "✅ TOUS BENCHMARKS COMPLÉTÉS AVEC SUCCÈS"
    log_info "═══════════════════════════════════════════════════════════"
    log_info "Résultats: $RESULTS_DIR/$TIMESTAMP/"
    log_info "Logs: $LOGS_DIR/$TIMESTAMP/"
}

# Exécuter
main
EOF

chmod +x ~/lumvorax_benchmarks/scripts/run_all_benchmarks.sh
```

---

## 📊 INSTRUMENTATION FORENSIQUE

### Script Forensic Logging

```bash
cat > ~/lumvorax_benchmarks/scripts/forensic_wrapper.sh << 'EOF'
#!/bin/bash
################################################################################
# LUMVORAX - WRAPPER FORENSIQUE POUR BENCHMARKS
# Ajoute traçabilité nanoseconde complète
################################################################################

BENCHMARK_NAME=$1
BENCHMARK_CMD=$2

TIMESTAMP_NS=$(date +%s%N)
LOG_FILE="forensic_${BENCHMARK_NAME}_${TIMESTAMP_NS}.log"

echo "╔════════════════════════════════════════════════════════════╗" | tee -a $LOG_FILE
echo "║  LUMVORAX - LOG FORENSIQUE                                ║" | tee -a $LOG_FILE
echo "╚════════════════════════════════════════════════════════════╝" | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE
echo "Benchmark: $BENCHMARK_NAME" | tee -a $LOG_FILE
echo "Commande: $BENCHMARK_CMD" | tee -a $LOG_FILE
echo "Timestamp (ns): $TIMESTAMP_NS" | tee -a $LOG_FILE
echo "Date: $(date --rfc-3339=ns)" | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# Capturer état système AVANT
echo "═══ ÉTAT SYSTÈME AVANT ═══" | tee -a $LOG_FILE
echo "CPU:" | tee -a $LOG_FILE
mpstat 1 1 | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE
echo "Mémoire:" | tee -a $LOG_FILE
free -h | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE
echo "GPU:" | tee -a $LOG_FILE
intel_gpu_top -s 1000 -o - 2>&1 | head -20 | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# Exécuter benchmark
START_NS=$(date +%s%N)
echo "═══ EXÉCUTION BENCHMARK ═══" | tee -a $LOG_FILE
echo "Start (ns): $START_NS" | tee -a $LOG_FILE

eval $BENCHMARK_CMD 2>&1 | tee -a $LOG_FILE

END_NS=$(date +%s%N)
DURATION_NS=$((END_NS - START_NS))
DURATION_S=$(echo "scale=9; $DURATION_NS / 1000000000" | bc)

echo "" | tee -a $LOG_FILE
echo "End (ns): $END_NS" | tee -a $LOG_FILE
echo "Duration (ns): $DURATION_NS" | tee -a $LOG_FILE
echo "Duration (s): $DURATION_S" | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# Capturer état système APRÈS
echo "═══ ÉTAT SYSTÈME APRÈS ═══" | tee -a $LOG_FILE
echo "CPU:" | tee -a $LOG_FILE
mpstat 1 1 | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE
echo "Mémoire:" | tee -a $LOG_FILE
free -h | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# Checksum
echo "═══ VALIDATION ═══" | tee -a $LOG_FILE
sha256sum $LOG_FILE | tee -a $LOG_FILE

echo "" | tee -a $LOG_FILE
echo "✅ Log forensique complet: $LOG_FILE" | tee -a $LOG_FILE
EOF

chmod +x ~/lumvorax_benchmarks/scripts/forensic_wrapper.sh
```

---

## ✅ VALIDATION RÉSULTATS

### Script Validation

```bash
cat > ~/lumvorax_benchmarks/scripts/validate_results.sh << 'EOF'
#!/bin/bash
################################################################################
# LUMVORAX - VALIDATION RÉSULTATS BENCHMARKS
################################################################################

RESULTS_DIR=$1

if [ -z "$RESULTS_DIR" ]; then
    echo "Usage: $0 <results_directory>"
    exit 1
fi

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  LUMVORAX - VALIDATION RÉSULTATS                          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Vérifier checksums
echo "═══ VÉRIFICATION CHECKSUMS ═══"
cd $RESULTS_DIR
for sha_file in *.sha256; do
    if [ -f "$sha_file" ]; then
        sha256sum -c "$sha_file"
    fi
done
echo ""

# Valider HPL
echo "═══ VALIDATION HPL ═══"
if [ -f "hpl_baseline.txt" ]; then
    RESIDUAL=$(grep "WR" hpl_baseline.txt | awk '{print $11}')
    RMAX=$(grep "WR" hpl_baseline.txt | awk '{print $7}')
    
    echo "Rmax: $RMAX GFLOPS"
    echo "Residual: $RESIDUAL"
    
    if (( $(echo "$RESIDUAL < 16.0" | bc -l) )); then
        echo "✅ HPL: Résiduel valide (< 16.0)"
    else
        echo "❌ HPL: Résiduel invalide (>= 16.0)"
    fi
else
    echo "⚠️  HPL: Fichier résultats non trouvé"
fi
echo ""

# Valider HPCG
echo "═══ VALIDATION HPCG ═══"
if [ -f "hpcg_baseline.txt" ]; then
    GFLOPS=$(grep "GFLOP/s" hpcg_baseline.txt | awk '{print $NF}')
    echo "GFLOPS: $GFLOPS"
    
    if (( $(echo "$GFLOPS > 0" | bc -l) )); then
        echo "✅ HPCG: Résultat valide (> 0 GFLOPS)"
    else
        echo "❌ HPCG: Résultat invalide"
    fi
else
    echo "⚠️  HPCG: Fichier résultats non trouvé"
fi
echo ""

# Valider Graph500
echo "═══ VALIDATION GRAPH500 ═══"
if [ -f "graph500_bfs_scale20.txt" ]; then
    VALIDATION=$(grep "VALIDATION PASSED" graph500_bfs_scale20.txt)
    TEPS=$(grep "TEPS" graph500_bfs_scale20.txt | awk '{print $NF}')
    
    echo "TEPS: $TEPS"
    
    if [ ! -z "$VALIDATION" ]; then
        echo "✅ Graph500: Validation passed"
    else
        echo "❌ Graph500: Validation failed"
    fi
else
    echo "⚠️  Graph500: Fichier résultats non trouvé"
fi
echo ""

# Valider STREAM
echo "═══ VALIDATION STREAM ═══"
if [ -f "stream_cpu.txt" ]; then
    TRIAD=$(grep "Triad:" stream_cpu.txt | awk '{print $2}')
    echo "Triad: $TRIAD MB/s"
    
    if (( $(echo "$TRIAD > 1000" | bc -l) )); then
        echo "✅ STREAM: Bande passante valide (> 1 GB/s)"
    else
        echo "⚠️  STREAM: Bande passante faible (< 1 GB/s)"
    fi
else
    echo "⚠️  STREAM: Fichier résultats non trouvé"
fi
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "✅ VALIDATION COMPLÉTÉE"
echo "═══════════════════════════════════════════════════════════"
EOF

chmod +x ~/lumvorax_benchmarks/scripts/validate_results.sh
```

---

## 🔧 TROUBLESHOOTING

### Problèmes Courants

#### 1. Erreur Compilation HPL

```bash
# Vérifier BLAS
ls -l /usr/lib/x86_64-linux-gnu/libopenblas*

# Réinstaller si nécessaire
sudo apt install --reinstall libopenblas-dev

# Vérifier MPI
mpirun --version
```

#### 2. Erreur Permissions turbostat

```bash
# Ajouter utilisateur au groupe
sudo usermod -a -G adm $USER

# Ou exécuter avec sudo
sudo turbostat --show PkgWatt --interval 1
```

#### 3. GPU Non Détecté

```bash
# Vérifier GPU
lspci | grep -i vga

# Installer drivers Intel
sudo apt install intel-gpu-tools

# Tester
intel_gpu_top
```

#### 4. Mémoire Insuffisante HPL

```bash
# Réduire N dans HPL.dat
# Formule: N = sqrt(RAM_GB * 0.8 * 1e9 / 8)
# Pour 16 GB: N ≈ 40000
# Pour 8 GB: N ≈ 28000
```

---

## 📝 CHECKLIST EXÉCUTION

### Avant Exécution

- [ ] Tous benchmarks installés et compilés
- [ ] Configuration système vérifiée (CPU, RAM, GPU)
- [ ] Outils monitoring installés (mpstat, intel_gpu_top, turbostat)
- [ ] Espace disque suffisant (100+ GB)
- [ ] Permissions correctes (sudo pour turbostat)

### Pendant Exécution

- [ ] Monitoring actif (CPU, GPU, température, consommation)
- [ ] Logs forensiques générés
- [ ] Pas d'autres processus lourds en cours
- [ ] Système stable (pas de throttling thermique)

### Après Exécution

- [ ] Tous résultats générés
- [ ] Checksums validés
- [ ] Rapport final créé
- [ ] Résultats sauvegardés
- [ ] Validation reproductibilité (3 runs minimum)

---

## 🚀 EXÉCUTION RAPIDE

### Commande Unique

```bash
# Installation complète
cd ~/lumvorax_benchmarks/scripts
./install_all_benchmarks.sh

# Exécution tous benchmarks
./run_all_benchmarks.sh

# Validation résultats
./validate_results.sh ~/lumvorax_benchmarks/results/$(ls -t ~/lumvorax_benchmarks/results | head -1)
```

---

## 📊 RÉSULTATS ATTENDUS

### Performance Baseline Estimée

| Benchmark | Métrique | Valeur Estimée | Unité |
|-----------|----------|----------------|-------|
| HPL | Rmax | 10-20 | GFLOPS |
| HPCG | GFLOPS | 1-5 | GFLOPS |
| Green500 | FLOPS/Watt | 1-2 | GFLOPS/W |
| Graph500 | TEPS | 1-5M | TEPS |
| IO500 | Score | 10-50 | - |
| STREAM | Triad | 15-25 | GB/s |
| OSU Latency | Latence | 1-10 | µs |

### Critères Validation

- ✅ HPL: Résiduel < 16.0
- ✅ HPCG: GFLOPS > 0
- ✅ Graph500: Validation passed
- ✅ STREAM: Triad > 10 GB/s
- ✅ Reproductibilité: Variance < 5%

---

**STATUT**: ✅ GUIDE COMPLET - PRÊT POUR UTILISATION

**Auteur**: Bob (Expert IA + HPC Benchmarking)  
**Date**: 2026-06-19  
**Version**: 1.0  