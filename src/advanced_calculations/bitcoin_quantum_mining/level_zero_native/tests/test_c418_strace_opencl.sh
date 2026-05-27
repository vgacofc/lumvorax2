#!/bin/bash
# C418 - CAPTURE BATCH BUFFER OPENCL VIA STRACE
# 
# Utilise strace pour intercepter les appels ioctl EXECBUFFER2
# et dumper le contenu exact du batch buffer OpenCL

cd "$(dirname "$0")/.."

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  C418 - CAPTURE BATCH BUFFER OPENCL VIA STRACE               ║"
echo "║  Interception des appels DRM_IOCTL_I915_GEM_EXECBUFFER2      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Compiler le test OpenCL simple
gcc -o test_c417 tests/test_c417_opencl_dump.c -I/usr/include/libdrm -lOpenCL

echo "=== EXÉCUTION AVEC STRACE ==="
echo "Capture de tous les ioctl DRM..."
echo ""

# Exécuter avec strace pour capturer les ioctl
strace -e trace=ioctl -v -s 4096 -o logs/forensic/c418_strace_opencl.log ./test_c417 2>&1 | head -30

echo ""
echo "=== ANALYSE DES LOGS STRACE ==="
echo ""

# Extraire les appels EXECBUFFER2
grep -A 50 "DRM_IOCTL_I915_GEM_EXECBUFFER2" logs/forensic/c418_strace_opencl.log | head -100

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Logs complets dans: logs/forensic/c418_strace_opencl.log    ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Analyser la taille du batch buffer
echo "=== STATISTIQUES BATCH BUFFER ==="
grep "batch_len" logs/forensic/c418_strace_opencl.log | head -5

echo ""
echo "Fichier log créé: logs/forensic/c418_strace_opencl.log"
echo "Taille: $(wc -l < logs/forensic/c418_strace_opencl.log) lignes"

# Made with Bob
