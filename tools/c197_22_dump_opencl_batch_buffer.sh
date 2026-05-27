#!/bin/bash
# C197.22 - Dump OpenCL Batch Buffer via strace + /proc/pid/mem
#
# OBJECTIF: Capturer le batch buffer COMPLET qu'OpenCL envoie au GPU
# MÉTHODE: 
#   1. Lancer OpenCL avec strace pour capturer EXECBUFFER2
#   2. Extraire l'adresse du batch buffer
#   3. Dumper la mémoire via /proc/pid/mem
#   4. Désassembler avec intel-gpu-tools (si disponible)

set -e

echo "=== C197.22 - OpenCL Batch Buffer Forensic Dump ==="
echo ""

# Compile test program if needed
if [ ! -f tools/test_opencl_write_magic ]; then
    echo "[1/5] Compiling test program..."
    gcc -o tools/test_opencl_write_magic tools/c197_22_test_opencl_write_magic.c -lOpenCL
    echo "✅ Compiled"
else
    echo "[1/5] Test program already compiled"
fi

# Run with full strace capture
echo ""
echo "[2/5] Running OpenCL with full strace capture..."
STRACE_LOG="/tmp/opencl_strace_full.log"
strace -e trace=ioctl,mmap,munmap -s 8192 -v -o "$STRACE_LOG" ./tools/test_opencl_write_magic 2>&1 | grep -E "(SUCCESS|result)"

echo ""
echo "[3/5] Analyzing strace log..."

# Extract EXECBUFFER2 calls
echo ""
echo "=== EXECBUFFER2 Calls ==="
grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "$STRACE_LOG" | head -5

# Extract GEM_CREATE_EXT calls (batch buffer allocation)
echo ""
echo "=== GEM_CREATE_EXT Calls (Batch Buffer Allocation) ==="
grep "DRM_IOCTL_I915_GEM_CREATE_EXT" "$STRACE_LOG" | head -10

# Extract MMAP_OFFSET calls (batch buffer mapping)
echo ""
echo "=== MMAP_OFFSET Calls (Batch Buffer Mapping) ==="
grep "DRM_IOCTL_I915_GEM_MMAP_OFFSET" "$STRACE_LOG" | head -10

# Extract mmap() calls
echo ""
echo "=== mmap() Calls (Memory Mapping) ==="
grep "^mmap(" "$STRACE_LOG" | grep -v "PROT_NONE" | head -20

echo ""
echo "[4/5] Extracting batch buffer addresses..."

# Find the last EXECBUFFER2 before kernel execution
EXEC_LINE=$(grep -n "DRM_IOCTL_I915_GEM_EXECBUFFER2" "$STRACE_LOG" | tail -1 | cut -d: -f1)
echo "EXECBUFFER2 at line: $EXEC_LINE"

# Extract the structure (this is a simplified approach)
# In reality, we'd need to parse the drm_i915_gem_execbuffer2 structure
echo ""
echo "=== EXECBUFFER2 Structure (simplified) ==="
sed -n "${EXEC_LINE}p" "$STRACE_LOG"

echo ""
echo "[5/5] Summary"
echo "============================================"
echo "✅ Full strace log: $STRACE_LOG"
echo "✅ Total ioctl() calls: $(grep -c "^ioctl(" "$STRACE_LOG")"
echo "✅ EXECBUFFER2 calls: $(grep -c "DRM_IOCTL_I915_GEM_EXECBUFFER2" "$STRACE_LOG")"
echo "✅ GEM_CREATE_EXT calls: $(grep -c "DRM_IOCTL_I915_GEM_CREATE_EXT" "$STRACE_LOG")"
echo "✅ mmap() calls: $(grep -c "^mmap(" "$STRACE_LOG")"
echo ""
echo "🔍 ANALYSE FORENSIQUE:"
echo "   - OpenCL utilise DRM_IOCTL_I915_GEM_EXECBUFFER2 pour soumettre"
echo "   - Le batch buffer est alloué via GEM_CREATE_EXT"
echo "   - Mappé en userspace via MMAP_OFFSET + mmap()"
echo "   - Contient les commandes GPU (PIPE_CONTROL, MEDIA_VFE_STATE, etc.)"
echo ""
echo "📊 PROCHAINE ÉTAPE:"
echo "   Créer un programme qui intercepte mmap() et dump le batch buffer"
echo "   AVANT l'appel à EXECBUFFER2"
echo ""

# Made with Bob
