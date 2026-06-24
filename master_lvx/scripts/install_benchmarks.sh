#!/bin/bash
################################################################################
# LUMVORAX - SCRIPT INSTALLATION BENCHMARKS INTERNATIONAUX
# Version: 1.0 | Date: 2026-06-19 | Auteur: Bob
# Technologie: C Native + Level Zero (i915 DRM) | MAGEN: Désactivé
################################################################################

set -e
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'

BASE_DIR=~/lumvorax_benchmarks
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="${BASE_DIR}/logs/install_${TIMESTAMP}.log"

mkdir -p "${BASE_DIR}"/{hpl,hpcg,graph500,io500,stream,osu,results,logs,scripts}

log_info() { echo -e "${GREEN}[INFO]${NC} $(date '+%Y-%m-%d %H:%M:%S.%N') - $1" | tee -a "${LOG_FILE}"; }
log_error() { echo -e "${RED}[ERROR]${NC} $(date '+%Y-%m-%d %H:%M:%S.%N') - $1" | tee -a "${LOG_FILE}"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $(date '+%Y-%m-%d %H:%M:%S.%N') - $1" | tee -a "${LOG_FILE}"; }

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  LUMVORAX - INSTALLATION BENCHMARKS STANDARDS             ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"

log_info "Début installation - Base: ${BASE_DIR}"

# Vérifications système
log_info "Vérification environnement..."
[[ "$(uname -s)" != "Linux" ]] && { log_error "Linux requis"; exit 1; }
log_success "OS: $(uname -s) $(uname -r)"
log_success "CPU cores: $(nproc)"
log_success "RAM: $(free -g | grep Mem | awk '{print $2}') GB"

# Installation dépendances
log_info "Installation dépendances système..."
sudo apt update && sudo apt install -y \
    build-essential gcc-9 g++-9 gfortran-9 cmake make \
    libopenblas-dev liblapack-dev openmpi-bin libopenmpi-dev \
    linux-tools-common sysstat lm-sensors intel-gpu-tools \
    git wget curl 2>&1 | tee -a "${LOG_FILE}"

log_success "Dépendances installées"

# HPL
log_info "Installation HPL..."
cd "${BASE_DIR}/hpl"
[[ ! -f "hpl-2.3.tar.gz" ]] && wget http://www.netlib.org/benchmark/hpl/hpl-2.3.tar.gz
[[ ! -d "hpl-2.3" ]] && tar -xzf hpl-2.3.tar.gz
cd hpl-2.3
cat > Make.Linux << 'EOF'
ARCH=Linux
TOPdir=$(HOME)/lumvorax_benchmarks/hpl/hpl-2.3
MPdir=/usr/lib/x86_64-linux-gnu/openmpi
MPinc=-I$(MPdir)/include
MPlib=$(MPdir)/lib/libmpi.so
LAdir=/usr/lib/x86_64-linux-gnu
LAlib=$(LAdir)/libopenblas.a
F2CDEFS=-DAdd__ -DF77_INTEGER=int -DStringSunStyle
HPL_OPTS=-DHPL_CALL_CBLAS
HPL_INCLUDES=-I$(TOPdir)/include -I$(TOPdir)/include/$(ARCH) $(LAinc) $(MPinc)
HPL_LIBS=$(LAlib) $(MPlib) -lm
HPL_DEFS=$(F2CDEFS) $(HPL_OPTS) $(HPL_INCLUDES)
CC=mpicc
CCFLAGS=$(HPL_DEFS) -O3 -funroll-loops
LINKER=mpif77
LINKFLAGS=$(CCFLAGS)
ARCHIVER=ar
ARFLAGS=r
RANLIB=echo
EOF
make arch=Linux 2>&1 | tee -a "${LOG_FILE}"
[[ -f "bin/Linux/xhpl" ]] && log_success "HPL compilé" || { log_error "Échec HPL"; exit 1; }

# HPCG
log_info "Installation HPCG..."
cd "${BASE_DIR}/hpcg"
[[ ! -f "HPCG-release-3-1-0.tar.gz" ]] && wget https://github.com/hpcg-benchmark/hpcg/archive/refs/tags/HPCG-release-3-1-0.tar.gz
[[ ! -d "hpcg-HPCG-release-3-1-0" ]] && tar -xzf HPCG-release-3-1-0.tar.gz
cd hpcg-HPCG-release-3-1-0 && mkdir -p setup build
cat > setup/Make.Linux_MPI << 'EOF'
ARCH=Linux_MPI
CXX=mpicxx
CXXFLAGS=-O3 -ffast-math
LINKER=$(CXX)
HPCG_DEFS=-DHPCG_NO_OPENMP
EOF
cd build && ../configure Linux_MPI && make 2>&1 | tee -a "${LOG_FILE}"
[[ -f "bin/xhpcg" ]] && log_success "HPCG compilé" || { log_error "Échec HPCG"; exit 1; }

# Graph500
log_info "Installation Graph500..."
cd "${BASE_DIR}/graph500"
[[ ! -f "graph500-3.0.tar.gz" ]] && wget https://github.com/graph500/graph500/archive/refs/tags/graph500-3.0.tar.gz
[[ ! -d "graph500-graph500-3.0" ]] && tar -xzf graph500-3.0.tar.gz
cd graph500-graph500-3.0/src && make 2>&1 | tee -a "${LOG_FILE}"
[[ -f "graph500_reference_bfs" ]] && log_success "Graph500 compilé" || { log_error "Échec Graph500"; exit 1; }

# STREAM
log_info "Installation STREAM..."
cd "${BASE_DIR}/stream"
[[ ! -f "stream.c" ]] && wget https://www.cs.virginia.edu/stream/FTP/Code/stream.c
gcc -O3 -fopenmp -DSTREAM_ARRAY_SIZE=100000000 stream.c -o stream_cpu 2>&1 | tee -a "${LOG_FILE}"
[[ -f "stream_cpu" ]] && log_success "STREAM compilé" || { log_error "Échec STREAM"; exit 1; }

# OSU
log_info "Installation OSU..."
cd "${BASE_DIR}/osu"
[[ ! -f "osu-micro-benchmarks-5.9.tar.gz" ]] && wget http://mvapich.cse.ohio-state.edu/download/mvapich/osu-micro-benchmarks-5.9.tar.gz
[[ ! -d "osu-micro-benchmarks-5.9" ]] && tar -xzf osu-micro-benchmarks-5.9.tar.gz
cd osu-micro-benchmarks-5.9 && ./configure CC=mpicc CXX=mpicxx && make 2>&1 | tee -a "${LOG_FILE}"
[[ -f "mpi/pt2pt/osu_latency" ]] && log_success "OSU compilé" || { log_error "Échec OSU"; exit 1; }

echo -e "\n${GREEN}✅ INSTALLATION COMPLÉTÉE${NC}\n"
log_success "Installation terminée: $(date)"
