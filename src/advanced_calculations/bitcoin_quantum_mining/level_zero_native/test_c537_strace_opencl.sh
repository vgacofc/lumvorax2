#!/bin/bash
# TEST C537 - Capture strace OpenCL pour analyser les buffers

set -e

echo "=== TEST C537 - CAPTURE STRACE OPENCL ==="
echo ""

cd "$(dirname "$0")"

# Compiler le test OpenCL simple
echo "Compilation test_c532_opencl_runner.c..."
gcc -o test_c532_opencl_runner \
    tests/test_c532_opencl_runner.c \
    -lOpenCL \
    -Wall -Wextra \
    || { echo "❌ Erreur compilation"; exit 1; }

echo "✓ Compilation réussie"
echo ""

# Exécuter avec strace pour capturer TOUS les ioctl
echo "Exécution avec strace (capture complète des ioctl)..."
echo ""

strace -f -e trace=ioctl -s 65536 -o /tmp/strace_opencl_c537.txt \
    ./test_c532_opencl_runner 2>&1 | tee logs/test_c537_opencl_output.log

echo ""
echo "=== ANALYSE STRACE ===

"

# Extraire les appels EXECBUFFER2
echo "Appels DRM_IOCTL_I915_GEM_EXECBUFFER2:"
grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" /tmp/strace_opencl_c537.txt | wc -l

echo ""
echo "Appels DRM_IOCTL_I915_GEM_CREATE:"
grep "DRM_IOCTL_I915_GEM_CREATE" /tmp/strace_opencl_c537.txt | wc -l

echo ""
echo "Appels DRM_IOCTL_I915_GEM_MMAP:"
grep "DRM_IOCTL_I915_GEM_MMAP" /tmp/strace_opencl_c537.txt | wc -l

echo ""
echo "=== EXTRACTION BUFFERS ===

"

# Parser le strace pour extraire les buffers
python3 << 'PYTHON_SCRIPT'
import re
import sys

print("Parsing strace output...")

with open('/tmp/strace_opencl_c537.txt', 'r') as f:
    content = f.read()

# Chercher les EXECBUFFER2 avec leurs buffers
execbuffer_pattern = r'ioctl\(\d+, DRM_IOCTL_I915_GEM_EXECBUFFER2.*?buffers_ptr=0x([0-9a-f]+)'
matches = re.findall(execbuffer_pattern, content, re.DOTALL)

print(f"Trouvé {len(matches)} appels EXECBUFFER2")

# Chercher les GEM_CREATE avec tailles
create_pattern = r'ioctl\(\d+, DRM_IOCTL_I915_GEM_CREATE.*?size=(\d+).*?handle=(\d+)'
creates = re.findall(create_pattern, content, re.DOTALL)

print(f"Trouvé {len(creates)} appels GEM_CREATE:")
for size, handle in creates[:20]:
    print(f"  Handle {handle}: {size} bytes")
    if int(size) == 32:
        print("    → Probablement IDRT")
    elif int(size) == 64:
        print("    → Probablement Surface State")
    elif int(size) == 320:
        print("    → Probablement Batch Buffer")

# Chercher les MMAP
mmap_pattern = r'ioctl\(\d+, DRM_IOCTL_I915_GEM_MMAP.*?handle=(\d+).*?size=(\d+)'
mmaps = re.findall(mmap_pattern, content, re.DOTALL)

print(f"\nTrouvé {len(mmaps)} appels GEM_MMAP:")
for handle, size in mmaps[:20]:
    print(f"  Handle {handle}: {size} bytes")

PYTHON_SCRIPT

echo ""
echo "=== FICHIERS GÉNÉRÉS ===

"
echo "/tmp/strace_opencl_c537.txt ($(wc -l < /tmp/strace_opencl_c537.txt) lignes)"
echo "logs/test_c537_opencl_output.log"

echo ""
echo "✓ Test C537 terminé"

# Made with Bob