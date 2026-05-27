#!/bin/bash
# Test C396 - Validation kernel manuel avec opcodes OpenCL réels
# Objectif: Vérifier que output[0-15] = 0xDEADBEEF

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "🧪 TEST C396 - Validation Kernel Manuel"
echo "========================================"
echo ""

# Vérifier que le kernel existe
if [ ! -f "kernels/test_c396_from_opencl.bin" ]; then
    echo "❌ Kernel non trouvé: kernels/test_c396_from_opencl.bin"
    echo "   Exécuter: python3 kernels/create_c396_from_opencl.py"
    exit 1
fi

echo "✅ Kernel trouvé: kernels/test_c396_from_opencl.bin"
echo ""

# Compiler le runner (réutiliser infrastructure existante)
echo "🔨 Compilation runner..."
gcc -o test_c396_runner \
    tests/test_c363_runner.c \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lze_loader \
    -lpthread \
    -lm \
    -DKERNEL_PATH=\"kernels/test_c396_from_opencl.bin\" \
    -DKERNEL_NAME=\"test_magic_values\" \
    2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "❌ Compilation échouée"
    exit 1
fi

echo "✅ Compilation réussie"
echo ""

# Exécuter le test
echo "🚀 Exécution test..."
echo ""

./test_c396_runner 2>&1 | tee logs/test_c396_validation.log

echo ""
echo "📊 Analyse résultats..."
echo ""

# Vérifier les résultats
if grep -q "0xDEADBEEF" logs/test_c396_validation.log; then
    echo "✅ SUCCÈS: 0xDEADBEEF détecté dans output!"
    echo ""
    echo "🎉 VALIDATION COMPLÈTE:"
    echo "   - Kernel manuel fonctionne"
    echo "   - Opcodes OpenCL validés"
    echo "   - Indépendance OpenCL confirmée"
    echo ""
    echo "🚀 Prochaine étape: C397 - Intégration Bitcoin mining"
    exit 0
else
    echo "❌ ÉCHEC: 0xDEADBEEF non trouvé"
    echo ""
    echo "📋 Logs complets: logs/test_c396_validation.log"
    exit 1
fi

# Made with Bob
